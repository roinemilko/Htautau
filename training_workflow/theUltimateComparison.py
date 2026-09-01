import argparse
import pandas as pd
import numpy as np
import mplhep as hep
import matplotlib.pyplot as plt
from sklearn.metrics import roc_curve, auc
import sys
import uproot
from plot_helpers import *
import gc

def main():
    parser = argparse.ArgumentParser(description="Absolute Independent Evaluation of Multiple Models")
    parser.add_argument("--parquets", nargs="+", required=True, help="Paths to cached Parquet predictions")
    parser.add_argument("--raw_sigs", nargs="+", required=True, help="Raw Signal ROOT files for denominator")
    parser.add_argument("--raw_bgs", nargs="+", required=True, help="Raw Background ROOT files for denominator")
    parser.add_argument("--modes", nargs="+", required=True, choices=["Tau", "AK8", "AK15"])
    parser.add_argument("--names", nargs="+", required=True, help="Labels for the plot legend")
    parser.add_argument("--out_plot", required=True, help="Output path for Global ROC plot")
    parser.add_argument("--out_rej_plot", default="results/abs_eval_rejection.png", help="Output path for Background Rejection plot")
    parser.add_argument("--bg_mode", required=True, help="Name of the bg mode for plot axes")
    parser.add_argument("--num_taus", type=int, default=2)
    parser.add_argument("--use_subjets", action="store_true")
    parser.add_argument("--use_all", action="store_true", help="Evaluate on all common events (skip 50/50 holdout)")
    parser.add_argument("--use_weights", action="store_true", help="Apply cross-section weights to evaluation")
    parser.add_argument("--cms_label", default="Work in Progress")
    args = parser.parse_args()

    if not (len(args.parquets) == len(args.modes) == len(args.names)):
        print("Error: --parquets, --modes, and --names must have the same length.")
        sys.exit(1)

    hep.style.use("CMS")
    num_models = len(args.parquets)

    loaded_data = []

    print("Loading inferences")
    for i in range(num_models):
        df_all = pd.read_parquet(args.parquets[i])

        if not args.use_all:
            df_all = df_all[df_all["event"] % 2 == 1].copy()

        df_sig = df_all[df_all["label"] == 1].copy()
        df_bg = df_all[df_all["label"] == 0].copy()

        df_sig[f"pred_{i}"] = df_sig["pred"]
        df_bg[f"pred_{i}"] = df_bg["pred"]

        loaded_data.append({"sig": df_sig, "bg": df_bg})
        del df_all
        gc.collect()

    sig_weight_per_event = loaded_data[0]["sig"]["weight"].iloc[0] if not loaded_data[0]["sig"].empty else 1.0
    total_raw_sig_w = 0.0
    
    for p in args.raw_sigs:
        with uproot.open(f"{p}:Events") as tree:
            evts = tree["event"].array(library="np")
            mask = (evts % 2 == 1) if not args.use_all else np.ones_like(evts, dtype=bool)
            total_raw_sig_w += np.sum(mask) * sig_weight_per_event

    total_raw_bg_w = 0.0
    raw_bgs_list = args.raw_bgs[0].split(',') if len(args.raw_bgs) == 1 and ',' in args.raw_bgs[0] else args.raw_bgs
    
    for p in raw_bgs_list:
        process = next((k for k in XSEC_DICT.keys() if k in p), "unknown")
        with uproot.open(f"{p}:Events") as tree:
            evts = tree["event"].array(library="np")
            n_gen = tree["NRawEvents"].array(library="np", entry_stop=1)[0]
        
        lumi_pb = LUMI_FB * 1000.0
        xsec = XSEC_DICT.get(process, XSEC_DICT.get("jets", 1.0))
        w = (xsec * lumi_pb) / n_gen if n_gen > 0 and args.use_weights else 1.0
        
        mask = (evts % 2 == 1) if not args.use_all else np.ones_like(evts, dtype=bool)
        total_raw_bg_w += np.sum(mask) * w


    colors = ["#1f77b4", "#ff7f0e", "#2ca02c", "#d62728", "#9467bd"]
    markers = ["o", "s", "^", "D", "v"]

    print("Generating plots...")

    fig_roc, ax_roc = plt.subplots(figsize=(8, 8), dpi=150)
    fig_rej, ax_rej = plt.subplots(figsize=(8, 8), dpi=150)
    
    for i in range(num_models):
        df_sig = loaded_data[i]["sig"]
        df_bg = loaded_data[i]["bg"]

        sig_preds = df_sig[f"pred_{i}"].values
        bg_preds = df_bg[f"pred_{i}"].values

        w_sig = df_sig["weight"].values if "weight" in df_sig.columns else np.ones(len(df_sig))
        w_bg = df_bg["weight"].values if "weight" in df_bg.columns else np.ones(len(df_bg))

        reco_sig_w = np.sum(w_sig)
        reco_bg_w = np.sum(w_bg)
        
        missed_sig_w = max(0.0, total_raw_sig_w - reco_sig_w)
        missed_bg_w = max(0.0, total_raw_bg_w - reco_bg_w)

        print(f"[{args.names[i]}] MISSED SIG W: {missed_sig_w:.2f} || MISSED BG W: {missed_bg_w:.2f}")

        y_true_combined = np.concatenate([np.ones(len(sig_preds)), np.zeros(len(bg_preds)), [1, 0]])
        y_pred_combined = np.concatenate([sig_preds, bg_preds, [0.0, 0.0]])
        w_combined = np.concatenate([w_sig, w_bg, [missed_sig_w, missed_bg_w]])

        fpr_arr, tpr_arr, thresh_arr = roc_curve(y_true_combined, y_pred_combined, sample_weight=w_combined)
        roc_auc = auc(fpr_arr, tpr_arr)

        real_cuts = thresh_arr > -0.5
        fpr_arr = fpr_arr[real_cuts]
        tpr_arr = tpr_arr[real_cuts]
        thresh_arr = thresh_arr[real_cuts]

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
    hep.cms.label(args.cms_label, data=False, rlabel="13.6 TeV", ax=ax_roc, loc=0, fontsize=14)
    ax_roc.set_title(rf"$H \to \tau\tau$ + {get_mode_names(args.bg_mode)}", loc="center", fontsize=14)
    
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
    hep.cms.label(args.cms_label, data=False, rlabel="13.6 TeV", ax=ax_rej, loc=0, fontsize=14)
    ax_rej.set_title(rf"$H \to \tau\tau$ + {get_mode_names(args.bg_mode)}", loc="center", fontsize=14)

    fig_rej.tight_layout()
    fig_rej.savefig(args.out_rej_plot, bbox_inches="tight")
    plt.close(fig_rej)
    print(f"Background Rejection plot saved to {args.out_rej_plot}")


if __name__ == "__main__":
    main()