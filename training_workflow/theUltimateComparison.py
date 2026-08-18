import argparse
import pandas as pd
import numpy as np
import xgboost as xgb
import mplhep as hep
import matplotlib.pyplot as plt
from sklearn.metrics import roc_curve, auc
import sys
import uproot
from uproot_fat import load_fatjet_data
from uproot_data import load_tau_data


BG_STRING_DICT = {
    "TTto4Q": r"$tt \to qqqq$",
    "TTto2L2Nu": r"$tt \to \ell\ell\nu\nu$",
    "TTtoLNu2Q": r"$tt \to \ell \nu qq$",
    "DYto2Tau": "DY"
}

def load_data(sig_path, bg_path, mode, args, variables=None):
    """Caller for data loaders"""
    base_vars = variables if variables is not None else []
    sig_req_vars = sorted(list(set(base_vars + ["event", "genH_pt"])))
    bg_req_vars = sorted(list(set(base_vars + ["event"])))

    if mode == "Tau":
        df_sig = load_tau_data(sig_path, label=1, num_taus=args.num_taus, variables=sig_req_vars)
        df_bg = load_tau_data(bg_path, label=0, num_taus=args.num_taus, variables=bg_req_vars)
    else:
        df_sig = load_fatjet_data(
            sig_path, label=1, jet_type=mode, use_subjets=args.use_subjets, variables=sig_req_vars
        )
        df_bg = load_fatjet_data(
            bg_path, label=0, jet_type=mode, use_subjets=args.use_subjets, variables=bg_req_vars
        )

    return df_sig, df_bg



def main():
    parser = argparse.ArgumentParser(description="Absolute Independent Evaluation of Multiple Models")
    parser.add_argument("--sigs", nargs="+", required=True, help="Signal ROOT file per model/mode")
    parser.add_argument("--bgs", nargs="+", required=True, help="Background ROOT file per model/mode")
    parser.add_argument("--models", nargs="+", required=True, help="Paths to trained XGBoost models")
    parser.add_argument("--modes", nargs="+", required=True, choices=["Tau", "AK8", "AK15"])
    parser.add_argument("--names", nargs="+", required=True, help="Labels for the plot legend")
    parser.add_argument("--out_plot", required=True, help="Output path for Global ROC plot")
    parser.add_argument("--out_rej_plot", default="results/abs_eval_rejection.png", help="Output path for Background Rejection plot")
    parser.add_argument("--raw_sig", required=True, help="Path to RawEventInfo.root for Signal")
    parser.add_argument("--raw_bg", required=False, help="Path to RawEventInfo.root for Background (Optional)")
    parser.add_argument("--bg_mode", required=True, help="Name of the bg mode for plot axes")
    parser.add_argument("--seed", type=int, default=100)
    parser.add_argument("--num_taus", type=int, default=2)
    parser.add_argument("--use_subjets", action="store_true")
    parser.add_argument("--use_all", action="store_true", help="Evaluate on all common events (skip 50/50 holdout)")
    args = parser.parse_args()

    if not (len(args.sigs) == len(args.bgs) == len(args.models) == len(args.modes) == len(args.names)):
        print("Error: --sigs, --bgs, --models, --modes, and --names must have the same length.")
        sys.exit(1)

    hep.style.use("CMS")
    num_models = len(args.models)

    print("Loading raw events...")
    with uproot.open(f"{args.raw_sig}:Events") as raw_tree:
        raw_sig_evts = raw_tree["event"].array(library="np")
        raw_sig_pt = raw_tree["genH_pt_raw"].array(library="np")

    test_sig_mask = (raw_sig_evts % 2 == 1) if not args.use_all else np.ones_like(raw_sig_evts, dtype=bool)
    global_sig_evts = raw_sig_evts[test_sig_mask]
    global_sig_pt = raw_sig_pt[test_sig_mask]

    with uproot.open(f"{args.raw_bg}:Events") as raw_tree:
        raw_bg_evts = raw_tree["event"].array(library="np")

    test_bg_mask = (raw_bg_evts % 2 == 1) if not args.use_all else np.ones_like(raw_bg_evts, dtype=bool)
    global_bg_evts = raw_bg_evts[test_bg_mask]

    loaded_data = []

    print("Running inference...")
    for i in range(num_models):
        bst = xgb.Booster()
        bst.load_model(args.models[i])
        features = bst.feature_names
        
        base_vars = []
        if features:
            for f in features:
                if f.endswith("_1") or f.endswith("_2"):
                    base_vars.append(f[:-2])
                else:
                    base_vars.append(f)
            load_vars = sorted(list(set(base_vars)))
        else:
            load_vars = None
            
        df_sig, df_bg = load_data(args.sigs[i], args.bgs[i], args.modes[i], args, variables=load_vars)

        df_sig = df_sig.drop_duplicates(subset=["event"]).set_index("event")
        df_bg = df_bg.drop_duplicates(subset=["event"]).set_index("event")

        if not args.use_all:
            df_sig = df_sig[df_sig.index % 2 == 1]
            df_bg = df_bg[df_bg.index % 2 == 1]

        X_sig = df_sig[features] if features else df_sig.drop(columns=["label", "genH_pt"], errors="ignore")
        d_sig = xgb.DMatrix(X_sig, missing=np.inf)
        df_sig[f"pred_{i}"] = bst.predict(d_sig)

        X_bg = df_bg[features] if features else df_bg.drop(columns=["label"], errors="ignore")
        d_bg = xgb.DMatrix(X_bg, missing=np.inf)
        df_bg[f"pred_{i}"] = bst.predict(d_bg)

        loaded_data.append({"sig": df_sig, "bg": df_bg})


    print("Mapping predictions to global event pool...")
    eval_sig = pd.DataFrame(index=global_sig_evts)
    eval_sig['label'] = 1
    eval_sig['genH_pt'] = global_sig_pt

    eval_bg = pd.DataFrame(index=global_bg_evts)
    eval_bg['label'] = 0
    eval_bg['genH_pt'] = np.nan

    for i in range(num_models):
        eval_sig[f"pred_{i}"] = -1.0
        eval_bg[f"pred_{i}"] = -1.0
        
        reco_sig = loaded_data[i]["sig"]
        reco_bg = loaded_data[i]["bg"]
        
        valid_sig = reco_sig.index.intersection(eval_sig.index)
        valid_bg = reco_bg.index.intersection(eval_bg.index)
        
        eval_sig.loc[valid_sig, f"pred_{i}"] = reco_sig.loc[valid_sig, f"pred_{i}"]
        eval_bg.loc[valid_bg, f"pred_{i}"] = reco_bg.loc[valid_bg, f"pred_{i}"]

    eval_df = pd.concat([eval_sig, eval_bg], ignore_index=True)

    colors = ["#1f77b4", "#ff7f0e", "#2ca02c", "#d62728", "#9467bd"]
    markers = ["o", "s", "^", "D", "v"]
    


    print("Generating plots...")
    fig_roc, ax_roc = plt.subplots(figsize=(8, 8), dpi=150)
    fig_rej, ax_rej = plt.subplots(figsize=(8, 8), dpi=150)

    y_true = eval_df["label"].values
    
    for i in range(num_models):
        y_pred = eval_df[f"pred_{i}"].values
        
        fpr_arr, tpr_arr, thresh_arr = roc_curve(y_true, y_pred)
        roc_auc = auc(fpr_arr, tpr_arr)
        
        
        ax_roc.plot(
            fpr_arr, tpr_arr, 
            color=colors[i % len(colors)], lw=2, 
            label=f"{args.names[i]} (AUC = {roc_auc:.4f})"
        )

        valid_idx = fpr_arr > 0 
        tpr_valid = tpr_arr[valid_idx]
        rej_valid = 1.0 / fpr_arr[valid_idx]
        keep_idx = rej_valid >= 10
        rej_valid = rej_valid[keep_idx]
        tpr_valid = tpr_valid[keep_idx]
        

        ax_rej.plot(
            tpr_valid, rej_valid, 
            color=colors[i % len(colors)], lw=2, 
            label=f"{args.names[i]}"
        )

    ax_roc.set_xlim([1e-4, 1.0])
    ax_roc.set_ylim([0.0, 1.05])
    ax_roc.set_xscale("log")
    ax_roc.set_xlabel("Absolute Background Efficiency")
    ax_roc.set_ylabel("Absolute Signal Efficiency")
    ax_roc.legend(loc="lower right")
    ax_roc.grid(axis="both", which="major", linestyle="-", alpha=0.7)
    ax_roc.grid(axis="both", which="minor", linestyle=":", alpha=0.4)
    hep.cms.label("Work in Progress", data=False, rlabel=rf"$H \to \tau\tau$ + {BG_STRING_DICT[args.bg_mode]}", ax=ax_roc, loc=0, fontsize=14)
    
    fig_roc.tight_layout()
    fig_roc.savefig(args.out_plot, bbox_inches="tight")
    plt.close(fig_roc)
    print(f"Global ROC plot saved to {args.out_plot}")

    ax_rej.set_yscale("log")
    ax_rej.set_xlabel("Absolute Signal Efficiency")
    ax_rej.set_ylabel("Background Rejection")
    ax_rej.legend(loc="upper right")
    ax_rej.grid(axis="both", which="major", linestyle="-", alpha=0.7)
    ax_rej.grid(axis="both", which="minor", linestyle=":", alpha=0.4)
    hep.cms.label("Work in Progress", data=False, rlabel=rf"$H \to \tau\tau$ + {BG_STRING_DICT[args.bg_mode]}", ax=ax_rej, loc=0, fontsize=14)

    fig_rej.tight_layout()
    fig_rej.savefig(args.out_rej_plot, bbox_inches="tight")
    plt.close(fig_rej)
    print(f"Background Rejection plot saved to {args.out_rej_plot}")


if __name__ == "__main__":
    main()