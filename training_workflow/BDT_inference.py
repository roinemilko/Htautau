import argparse
import pandas as pd
import numpy as np
import dask
import dask.dataframe as dd
from dask.distributed import Client
import xgboost as xgb
import mplhep as hep
import matplotlib.pyplot as plt
from sklearn.metrics import roc_curve, auc, confusion_matrix, ConfusionMatrixDisplay, roc_auc_score
from scipy.stats import bootstrap
from Helpers import *
import time
from datetime import datetime

def get_candidate_pt(df, mode, num_taus):
    if mode == "AK8":
        return df["fj_pt"].values
    elif mode == "AK15":
        return df["ak15_pt"].values
    elif mode == "Tau" and num_taus == 2:
        # Calculate di-tau vector sum pT
        px = df["tau_pt_1"] * np.cos(df["tau_phi_1"]) + df["tau_pt_2"] * np.cos(df["tau_phi_2"])
        py = df["tau_pt_1"] * np.sin(df["tau_phi_1"]) + df["tau_pt_2"] * np.sin(df["tau_phi_2"])
        return np.sqrt(px**2 + py**2).values
    elif mode == "Tau" and num_taus == 1:
        return df["tau_pt"].values
    else:
        raise ValueError(f"Unknown mode {mode}")

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--sig", required=True)
    parser.add_argument("--bg", required=True)
    parser.add_argument("--model", required=True)
    parser.add_argument("--out_parquet", required=True, help="Path to save the inference results (.parquet)")
    parser.add_argument("--out_roc", required=True)
    parser.add_argument("--out_feat", required=True)
    parser.add_argument("--out_conf", required=True)
    parser.add_argument("--out_auc_pt", required=True)
    parser.add_argument("--cms_label", default="Work in Progress")
    parser.add_argument("--variables", nargs='+', default=None, help="List of variables to load")
    parser.add_argument("--num_taus", type=int, default=2, help="Number of taus per event")
    parser.add_argument("--mode", default="tau", choices=["Tau", "AK8", "AK15"], help="Object type to process")
    parser.add_argument("--use_subjets", action="store_true", help="Require 2 subjets and load subjet features (AK8/AK15 only)")
    parser.add_argument("--use_weights", action="store_true", help="Apply cross-section weights to evaluation")
    parser.add_argument("--n_workers", default=4)
    args = parser.parse_args()

    hep.style.use("CMS")

    dask.config.set({
        'distributed.worker.memory.target': 0.60, 
        'distributed.worker.memory.spill': 0.70,
        'distributed.worker.memory.pause': 0.85, 
    })
    client = Client(n_workers=int(args.n_workers), threads_per_worker=4, memory_limit='5GB')
    print(f"Dask Dashboard: {client.dashboard_link}")

    print("Loading model...")
    model = xgb.Booster()
    model.load_model(args.model)
    features = model.feature_names
 
    req_cols = []
    if args.mode == "AK8": 
        req_cols = ["fj_pt"]
    elif args.mode == "AK15": 
        req_cols = ["ak15_pt"]
    elif args.mode == "Tau": 
        req_cols = ["tau_pt", "tau_phi"]


    sig_req_cols = req_cols + ["genH_pt", "event"]
    bg_req_cols = req_cols + ["event"]

    if features:
        base_vars = []
        for f in features:
            if f.endswith("_1") or f.endswith("_2"):
                base_vars.append(f[:-2])
            else:
                base_vars.append(f)
        
        sig_vars = sorted(set(base_vars) | set(sig_req_cols))
        bg_vars = sorted(set(base_vars) | set(bg_req_cols))
    else:
        sig_vars = None
        bg_vars = None

    print("Loading data...")
    df_sig, df_bg = load_data(
        sig_path=args.sig, 
        bg_path=args.bg, 
        mode=args.mode, 
        args=args, 
        sig_vars=sig_vars, 
        bg_vars=bg_vars
    )

    df_test_lazy = dd.concat([df_sig, df_bg])

    cols_to_drop = ["label", "event", "genH_pt", "weight", "process"]

    def process_partition(part):
        if part.empty:
            return pd.DataFrame(columns=["event", "label", "weight", "genH_pt", "pred", "obj_pt"])
            
        X = part[features] if features else part.drop(columns=[c for c in cols_to_drop if c in part.columns])
        
        out = pd.DataFrame()
        out["event"] = part["event"].values
        out["label"] = part["label"].values
        out["weight"] = part["weight"].values if "weight" in part.columns else 1.0
        out["genH_pt"] = part["genH_pt"].values if "genH_pt" in part.columns else np.nan
        out["pred"] = model.predict(xgb.DMatrix(X, missing=np.inf))
        out["obj_pt"] = get_candidate_pt(part, args.mode, args.num_taus)
        return out

    print("Running inference... ")
    df_eval = df_test_lazy.map_partitions(
        process_partition, 
        meta={"event": "i8", "label": "i4", "weight": "f4", "genH_pt": "f4", "pred": "f4", "obj_pt": "f4"}
    ).compute()
    
    if args.use_weights:
        print("Rescale weights for inference")
        import uproot
        
        with uproot.open(f"{args.sig}:Events") as raw_tree:
            n_gen_total = raw_tree["NRawEvents"].array(library="np", entry_stop=1)[0]
            
        lumi_pb = LUMI_FB * 1000

        if "MADGRAPH" in args.sig:
            sig_process = "VBF"
        elif "POWHEG" in args.sig:
            sig_process = "ggF"
        
        sig_physical_weight = (XSEC_DICT[sig_process] * lumi_pb) / n_gen_total
        
        df_eval.loc[df_eval["label"] == 1, "weight"] = sig_physical_weight
        print(f"Signal weight: {sig_physical_weight}")

    print(f"Saving to {args.out_parquet}...")
    df_eval.to_parquet(args.out_parquet)

    df_test = df_eval[df_eval["event"] % 2 == 1]
    y_test = df_eval["label"].values
    w_test = df_eval["weight"].values
    y_pred_prob = df_eval["pred"].values
    obj_pt = df_eval["obj_pt"].values

    print("Generating plots...")

    # ROC Curve
    fig, ax = plt.subplots(figsize=(8, 7), dpi=150)
    fpr, tpr, _ = roc_curve(y_test, y_pred_prob, sample_weight=w_test)
    roc_auc = auc(fpr, tpr)
    ax.plot(fpr, tpr, label=f'AUC = {roc_auc:.8f}')
    ax.plot([0, 1], [0, 1], linestyle='--')
    ax.set_xlabel('Background efficiency')
    ax.set_ylabel('Signal efficiency')
    ax.legend()
    hep.cms.label(args.cms_label, data=False, rlabel=r"$H \to \tau\tau$ (125 GeV)", ax=ax, loc=3, fontsize=14)
    fig.tight_layout()
    fig.savefig(args.out_roc, bbox_inches="tight")
    plt.close()

    # Confusion Matrix (treshold at 0.5)
    fig, ax = plt.subplots(figsize=(7, 6), dpi=150)
    y_pred_class = (y_pred_prob > 0.5).astype(int)
    cm = confusion_matrix(y_test, y_pred_class)
    disp = ConfusionMatrixDisplay(confusion_matrix=cm, display_labels=['Background', 'Signal'])
    disp.plot(ax = ax, cmap='Blues', colorbar = False)
    hep.cms.label(args.cms_label, data=False, rlabel=r"$H \to \tau\tau$ (125 GeV)", ax=ax, loc=3, fontsize=14) 
    fig.tight_layout()
    fig.savefig(args.out_conf, bbox_inches="tight")
    plt.close()

    # Feature Importance
    scores = model.get_score(importance_type='gain')
    feature_names = features if features else []

    def resolve_name(name):
        actual_name = name
        if actual_name.startswith("f") and actual_name[1:].isdigit():
            idx = int(actual_name[1:])
            if idx < len(feature_names):
                actual_name = feature_names[idx]

        prefixes = ["tau_", "fj_Subjet_", "ak15_Subjet_", "fj_", "ak15_"]
        
        for prefix in prefixes:
            if actual_name.startswith(prefix):
                return actual_name.replace(prefix, "", 1)
                
        return actual_name

    ranked = sorted(
        ((resolve_name(k), v) for k, v in scores.items()),
        key=lambda x: x[1],
        reverse=True,
    )
    names = [x[0] for x in ranked]
    values = [x[1] for x in ranked]
    n_feat = len(names)
    fig_h = max(12, n_feat * 0.45)
    fig, ax = plt.subplots(figsize=(10, fig_h), dpi=150)
    y = np.arange(n_feat)
    ax.barh(y, values, color="#3182bd", height=0.7)
    ax.set_yticks(y)
    ax.set_yticklabels(names, fontsize=9)
    ax.invert_yaxis()
    ax.set_xscale("log")
    ax.set_xlabel("Gain")
    ax.set_ylabel("Feature")
    ax.grid(axis="x", which="both", linestyle=":", alpha=0.4)
    hep.cms.label(args.cms_label, data=False,
                  rlabel=r"$H \to \tau\tau$ (125 GeV)", ax=ax, loc=0, fontsize=12)
    fig.subplots_adjust(left=0.32, top=0.95, bottom=0.05, right=0.95)
    fig.savefig(args.out_feat, bbox_inches="tight")
    plt.close()


    # AUC vs. jet pt
    pt_bins = [200, 250, 300, 350, 400, 450, 500, 550, 600, 650, 700, 750, 800, 850, 900, 950, 1000, 1100, 1200]
    bin_centers = []
    auc_scores = []
    auc_errs = []
    n_sig_list = []
    n_bg_list = []

    def safe_auc(y, p):
        if len(np.unique(y)) < 2:
            return np.nan
        if len(np.unique(p)) == 1:
            return 0.5
        return roc_auc_score(y, p)

    for i in range(len(pt_bins) - 1):
        pt_min = pt_bins[i]
        pt_max = pt_bins[i+1]
        
        mask = (obj_pt >= pt_min) & (obj_pt < pt_max)
        
        y_true_bin = y_test[mask]
        y_pred_bin = y_pred_prob[mask]

        n_sig = np.sum(y_true_bin == 1)
        n_bg = np.sum(y_true_bin == 0)

        n_sig_list.append(n_sig)
        n_bg_list.append(n_bg)
        bin_centers.append((pt_min + pt_max) / 2.0)

        if n_sig < 50 or n_bg < 50:
            auc_scores.append(np.nan)
            auc_errs.append(np.nan)
            continue

        if len(np.unique(y_true_bin)) >= 2:
            nominal_auc = roc_auc_score(y_true_bin, y_pred_bin)
            auc_scores.append(nominal_auc)

            try:
                res = bootstrap(
                    (y_true_bin, y_pred_bin), 
                    safe_auc, 
                    vectorized=False, 
                    paired=True, 
                    n_resamples=50, 
                    random_state=datetime.now(),
                    method='percentile'
                )
                auc_errs.append(res.standard_error)
            except ValueError:
                auc_errs.append(0.0)

        else:
            auc_scores.append(np.nan)
            auc_errs.append(np.nan)

    fig, (ax_auc, ax_yield) = plt.subplots(
        2, 1, figsize=(8, 8), sharex=True, 
        gridspec_kw={'height_ratios': [3, 1]}, dpi=150
    )

    x_err = [(bin_centers[i] - pt_bins[i]) for i in range(len(bin_centers))]

    ax_auc.errorbar(bin_centers, auc_scores, xerr=x_err, yerr=auc_errs, fmt='o-', color='#1f77b4', capsize=3, label=f"{args.mode}")
    ax_auc.set_ylabel("AUC")
    ax_auc.legend(loc='lower left')
    ax_auc.grid(axis='y', linestyle=':', alpha=0.7)
    hep.cms.label("Work in Progress", data=False, rlabel=r"$H \to \tau\tau$ (125 GeV)", ax=ax_auc, loc=0, fontsize=14)

    bin_widths = [pt_bins[i+1] - pt_bins[i] for i in range(len(pt_bins)-1)]
    ax_yield.bar(bin_centers, n_sig_list, width=bin_widths, alpha=0.5, label="Signal", color="blue")
    ax_yield.bar(bin_centers, n_bg_list, width=bin_widths, alpha=0.5, label="Background", color="red")

    ax_yield.set_yscale("log")
    ax_yield.set_xlabel(f"Reconstructed Object $p_T$ [GeV]")
    ax_yield.set_ylabel("Events")
    ax_yield.grid(axis='y', linestyle=':', alpha=0.7)
    ax_yield.legend(loc='upper right')

    fig.tight_layout()
    fig.savefig(args.out_auc_pt, bbox_inches="tight")
    plt.close()
    
    client.close()
    print("Done!")

if __name__ == "__main__":
    main()