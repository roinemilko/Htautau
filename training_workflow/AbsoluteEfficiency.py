import argparse
import pandas as pd
import numpy as np
import xgboost as xgb
import mplhep as hep
import matplotlib.pyplot as plt
from sklearn.metrics import roc_curve
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


def get_threshold(y_true, y_pred, target_fpr):
    """Returns the cut for a given False Positive Rate"""
    fpr, tpr, thresholds = roc_curve(y_true, y_pred)
    idx = np.where(fpr <= target_fpr)[0][-1] if len(np.where(fpr <= target_fpr)[0]) > 0 else 0
    return thresholds[idx]


def main():
    parser = argparse.ArgumentParser(description="Absolute Signal Efficiency Evaluation of Multiple Models")
    parser.add_argument("--sigs", nargs="+", required=True, help="Signal ROOT file per model/mode")
    parser.add_argument("--bgs", nargs="+", required=True, help="Background ROOT file per model/mode")
    parser.add_argument("--models", nargs="+", required=True, help="Paths to trained XGBoost models")
    parser.add_argument("--modes", nargs="+", required=True, choices=["Tau", "AK8", "AK15"])
    parser.add_argument("--names", nargs="+", required=True, help="Labels for the plot legend")
    parser.add_argument("--out_plot", required=True, help="Output path for Absolute Sig. Eff. plot")
    parser.add_argument("--raw_sig", required=True, help="Path to RawEventInfo.root for Signal")
    parser.add_argument("--raw_bg", required=True, help="Path to RawEventInfo.root for Background")
    parser.add_argument("--bg_mode", required=True, help="Name of the bg mode for plot axes")
    parser.add_argument("--seed", type=int, default=100)
    parser.add_argument("--num_taus", type=int, default=2)
    parser.add_argument("--use_subjets", action="store_true")
    parser.add_argument("--fpr", type=float, default=0.01, help="Target Background False Positive Rate")
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
    
    print("Calculating thresholds...")
    thresholds = {}
    y_true = eval_df["label"].values
    
    for i in range(num_models):
        y_pred = eval_df[f"pred_{i}"].values
        thresholds[i] = get_threshold(y_true, y_pred, args.fpr)

    print("Generating Total Efficiency plot...")
    pt_bins = [200, 250, 300, 350, 400, 450, 500, 550, 600, 650, 700, 750, 800, 850, 900, 950, 1000, 1100, 1200]
    bin_centers = [(pt_bins[i] + pt_bins[i + 1]) / 2.0 for i in range(len(pt_bins) - 1)]
    bin_widths = [pt_bins[i + 1] - pt_bins[i] for i in range(len(pt_bins) - 1)]
    x_err = [bin_centers[i] - pt_bins[i] for i in range(len(bin_centers))]
    
    sig_df = eval_df[eval_df["label"] == 1]
    n_sig_gen_list = []

    fig_eff, (ax_eff, ax_yield_eff) = plt.subplots(
        2, 1, figsize=(8, 8), sharex=True, gridspec_kw={"height_ratios": [3, 1]}, dpi=150
    )

    for j in range(num_models):
        sig_effs, sig_errs = [], []
        for i in range(len(pt_bins) - 1):
            pt_min, pt_max = pt_bins[i], pt_bins[i + 1]
            mask = (sig_df["genH_pt"] >= pt_min) & (sig_df["genH_pt"] < pt_max)
            bin_sig = sig_df[mask]
            
            n_generated = len(bin_sig)
            
            if j == 0:
                n_sig_gen_list.append(n_generated)

            if n_generated < 50:
                sig_effs.append(np.nan)
                sig_errs.append(np.nan)
            else:
                n_passing = np.sum(bin_sig[f"pred_{j}"] > thresholds[j])
                eff = n_passing / n_generated
                err = np.sqrt(eff * (1 - eff) / n_generated)
                
                sig_effs.append(eff)
                sig_errs.append(err)


        cut = thresholds[j]
        cut_str = None
        
        if cut > 0.999:
            cut_inv = 1 - thresholds[j]
            exp = int(np.floor(np.log10(cut_inv)))
            mantissa = cut_inv / (10**exp)
            if abs(mantissa - 1.0) < 1e-5:
                cut_inv_str = f"$10^{{{exp}}}$"
            else:
                cut_inv_str = f"${mantissa:.1f} \\times 10^{{{exp}}}$"
            cut_str = f"1 - {cut_inv_str}"
        else:
            cut_str = f"{cut:.3}"

        ax_eff.errorbar(
            bin_centers, sig_effs, xerr=x_err, yerr=sig_errs,
            fmt=f"{markers[j % len(markers)]}-", color=colors[j % len(colors)],
            capsize=3, label=f"{args.names[j]} (Cut: {cut_str})",
        )

    ax_yield_eff.bar(bin_centers, n_sig_gen_list, width=bin_widths, alpha=0.2, color="black", label="Total Generated")
    
    br = 1.0 / args.fpr
    br_str = f"{br:.0e}"




    ax_eff.set_ylabel(f"Total Eff.")
    ax_eff.legend(loc="best", title=f"Background Rejection {br_str}", title_fontsize=14)
    ax_eff.grid(axis="y", which="major", linestyle="-", alpha=0.7)
    ax_eff.grid(axis="y", which="minor", linestyle=":", alpha=0.4)
    ax_eff.grid(axis="x", linestyle=":", alpha=0.7)
    hep.cms.label("Work in Progress", data=False, rlabel=rf"$H \to \tau\tau$ + {BG_STRING_DICT[args.bg_mode]}", ax=ax_eff, loc=0, fontsize=14)
    
    ax_yield_eff.set_xlabel(r"Higgs $p_T$ [GeV]")
    ax_yield_eff.set_ylabel("Events")
    ax_yield_eff.grid(axis="y", linestyle=":", alpha=0.7)
    ax_yield_eff.legend(loc="upper right", fontsize=10)
    
    fig_eff.tight_layout()
    fig_eff.savefig(args.out_plot, bbox_inches="tight")
    plt.close(fig_eff)
    print(f"Absolute Efficiency plot saved to {args.out_plot}")

if __name__ == "__main__":
    main()
