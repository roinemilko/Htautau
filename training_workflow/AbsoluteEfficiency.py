import argparse
import pandas as pd
import numpy as np
import mplhep as hep
import matplotlib.pyplot as plt
import sys
import uproot
from plot_helpers import *
import gc

def main():
    parser = argparse.ArgumentParser(description="Absolute Signal Efficiency Evaluation of Multiple Models")
    parser.add_argument("--parquets", nargs="+", required=True, help="Paths to cached Parquet predictions")
    parser.add_argument("--raw_sigs", nargs="+", required=True, help="Raw Signal ROOT files for denominator")
    parser.add_argument("--raw_bgs", nargs="+", required=True, help="Raw Background ROOT files for denominator")
    parser.add_argument("--modes", nargs="+", required=True, choices=["Tau", "AK8", "AK15"])
    parser.add_argument("--names", nargs="+", required=True, help="Labels for the plot legend")
    parser.add_argument("--out_plot", required=True, help="Output path for Absolute Sig. Eff. plot")
    parser.add_argument("--bg_mode", required=True, help="Name of the bg mode for plot axes")
    parser.add_argument("--num_taus", type=int, default=2)
    parser.add_argument("--use_subjets", action="store_true")
    parser.add_argument("--fpr", type=float, default=0.01, help="Target Background False Positive Rate")
    parser.add_argument("--use_all", action="store_true", help="Evaluate on all common events (skip 50/50 holdout)")
    parser.add_argument("--use_weights", action="store_true")
    parser.add_argument("--cms_label", default="Work in Progress")
    args = parser.parse_args()

    if not (len(args.parquets) == len(args.modes) == len(args.names)):
        print("Error: --parquets, --modes, and --names must have the same length.")
        sys.exit(1)

    hep.style.use("CMS")
    num_models = len(args.parquets)
    loaded_data = []

    print("Loading inferences...")
    for i in range(num_models):
        df_all = pd.read_parquet(args.parquets[i])
        
        print(f"First 5 rows of Parquet '{args.names[i]}'")
        print(df_all.head())
        
        if not args.use_all:
            df_all = df_all[df_all["event"] % 2 == 1].copy()

        df_sig = df_all[df_all["label"] == 1]
        df_bg = df_all[df_all["label"] == 0]
        
        loaded_data.append({"sig": df_sig, "bg": df_bg})
        del df_all
        gc.collect()

    sig_weight_per_event = loaded_data[0]["sig"]["weight"].iloc[0] if (args.use_weights and not loaded_data[0]["sig"].empty) else 1.0
    print(f"Extracted Signal Weight from Parquet: {sig_weight_per_event}")

    raw_sig_pts, raw_sig_weights = [], []
    total_raw_bg_w = 0.0

    raw_sigs_list = args.raw_sigs[0].split(',') if len(args.raw_sigs) == 1 and ',' in args.raw_sigs[0] else args.raw_sigs
    for p in raw_sigs_list:
        with uproot.open(f"{p}:Events") as tree:
            evts = tree["event"].array(library="np")
            pts = tree["genH_pt_raw"].array(library="np")
            
        mask = (evts % 2 == 1) if not args.use_all else np.ones_like(evts, dtype=bool)
        
        raw_sig_pts.extend(pts[mask])
        raw_sig_weights.extend([sig_weight_per_event] * np.sum(mask))


    raw_bgs_list = args.raw_bgs[0].split(',') if len(args.raw_bgs) == 1 and ',' in args.raw_bgs[0] else args.raw_bgs
    for p in raw_bgs_list:
        process = next((k for k in XSEC_DICT.keys() if k in p), "unknown")
        with uproot.open(f"{p}:Events") as tree:
            evts = tree["event"].array(library="np")
            n_gen = tree["NRawEvents"].array(library="np", entry_stop=1)[0]
            
        xsec = XSEC_DICT.get(process, XSEC_DICT.get("jets", 1.0))
        lumi_pb = LUMI_FB * 1000.0
        w = (xsec * lumi_pb) / n_gen if n_gen > 0 and args.use_weights else 1.0
        mask = (evts % 2 == 1) if not args.use_all else np.ones_like(evts, dtype=bool)
        total_raw_bg_w += np.sum(mask) * w

    raw_sig_pts = np.array(raw_sig_pts)
    raw_sig_weights = np.array(raw_sig_weights)

    pt_bins = [200, 250, 300, 350, 400, 450, 500, 550, 600, 650, 700, 750, 800, 850, 900, 950, 1000, 1100, 1200]
    bin_centers = [(pt_bins[i] + pt_bins[i + 1]) / 2.0 for i in range(len(pt_bins) - 1)]
    bin_widths = [pt_bins[i + 1] - pt_bins[i] for i in range(len(pt_bins) - 1)]
    x_err = [bin_centers[i] - pt_bins[i] for i in range(len(bin_centers))]

    n_sig_gen_list = []
    w_sig_gen_list = []
    for i in range(len(pt_bins) - 1):
        mask = (raw_sig_pts >= pt_bins[i]) & (raw_sig_pts < pt_bins[i+1])
        n_sig_gen_list.append(np.sum(mask))
        w_sig_gen_list.append(np.sum(raw_sig_weights[mask]))

    fig_eff, (ax_eff, ax_yield_eff) = plt.subplots(
        2, 1, figsize=(8, 8), sharex=True, gridspec_kw={"height_ratios": [3, 1]}, dpi=150
    )

    colors = ["#1f77b4", "#ff7f0e", "#2ca02c", "#d62728", "#9467bd"]
    markers = ["o", "s", "^", "D", "v"]

    for j in range(num_models):
        df_sig = loaded_data[j]["sig"]
        df_bg = loaded_data[j]["bg"]

        bg_preds = df_bg["pred"].values
        w_bg = df_bg["weight"].values

        target_passing_w = args.fpr * total_raw_bg_w

        sorted_idx = np.argsort(bg_preds)[::-1]
        sorted_preds = bg_preds[sorted_idx]
        sorted_weights = w_bg[sorted_idx]

        cum_w = np.cumsum(sorted_weights)
        cut_idx = np.searchsorted(cum_w, target_passing_w)

        if cut_idx < len(sorted_preds):
            threshold = sorted_preds[cut_idx]
        else:
            threshold = sorted_preds[-1] if len(sorted_preds) > 0 else 0.0

        sig_effs = [] 
        sig_errs = []

        for i in range(len(pt_bins) - 1):
            pt_min, pt_max = pt_bins[i], pt_bins[i + 1]
            n_generated_raw = n_sig_gen_list[i]
            w_generated = w_sig_gen_list[i]

            if n_generated_raw < 50:
                sig_effs.append(np.nan)
                sig_errs.append(np.nan)
            else:
                mask_reco = (df_sig["genH_pt"] >= pt_min) & (df_sig["genH_pt"] < pt_max)
                bin_sig = df_sig[mask_reco]
                
                w_passing = bin_sig.loc[bin_sig["pred"] > threshold, "weight"].sum()
                
                raw_eff = w_passing / w_generated if w_generated > 0 else 0.0
                
                eff = np.clip(raw_eff, 0.0, 1.0)
                variance = max(0.0, eff * (1.0 - eff))
                err = np.sqrt(variance / n_generated_raw) if n_generated_raw > 0 else 0.0
                
                sig_effs.append(eff)
                sig_errs.append(err)

        cut = threshold
        if cut > 0.999:
            cut_inv = 1 - threshold
            exp = int(np.floor(np.log10(cut_inv))) if cut_inv > 0 else -10
            mantissa = cut_inv / (10**exp) if cut_inv > 0 else 0
            cut_inv_str = f"$10^{{{exp}}}$" if abs(mantissa - 1.0) < 1e-5 else f"${mantissa:.1f} \\times 10^{{{exp}}}$"
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
    ax_eff.legend(loc="best", title=f"Background Rejection {br_str}", fontsize=13, title_fontsize=11)
    ax_eff.set_ylim(0.0, 1.06)
    ax_eff.grid(axis="y", which="major", linestyle="-", alpha=0.7)
    ax_eff.grid(axis="y", which="minor", linestyle=":", alpha=0.4)
    ax_eff.grid(axis="x", linestyle=":", alpha=0.7)
    hep.cms.label(args.cms_label, data=False, rlabel="13.6 TeV", ax=ax_eff, loc=0, fontsize=14)
    ax_eff.set_title(
        rf"$H \to \tau\tau$ + {get_mode_names(args.bg_mode)}", 
        x=0.62,
        fontsize=14
    )

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