import argparse
import pandas as pd
import numpy as np
import mplhep as hep
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import sys
import uproot
import gc
from plot_helpers import *
from tqdm import tqdm

def main():
    parser = argparse.ArgumentParser(description="Animate Absolute Signal Efficiency")
    parser.add_argument("--parquets", nargs="+", required=True, help="Paths to cached Parquet predictions")
    parser.add_argument("--raw_sigs", nargs="+", required=True, help="Raw Signal ROOT files for denominator")
    parser.add_argument("--raw_bgs", nargs="+", required=True, help="Raw Background ROOT files for denominator")
    parser.add_argument("--modes", nargs="+", required=True, choices=["Tau", "AK8", "AK15"])
    parser.add_argument("--names", nargs="+", required=True, help="Labels for the plot legend")
    parser.add_argument("--out_plot", required=True, help="Output path (.gif)")
    parser.add_argument("--bg_mode", required=True, help="Name of the bg mode for plot axes")
    parser.add_argument("--num_taus", type=int, default=2)
    parser.add_argument("--use_subjets", action="store_true")
    parser.add_argument("--use_all", action="store_true", help="Evaluate on all common events")
    parser.add_argument("--frames", type=int, default=50, help="Number of frames for the animation")
    parser.add_argument("--use_weights", action="store_true", help="Apply cross-section weights")
    parser.add_argument("--cms_label", default="Work in Progress")
    args = parser.parse_args()

    if not (len(args.parquets) == len(args.modes) == len(args.names)):
        print("Error: --parquets, --modes, and --names must have the same length.")
        sys.exit(1)

    hep.style.use("CMS")
    num_models = len(args.parquets)
    colors = ["#1f77b4", "#ff7f0e", "#2ca02c", "#d62728", "#9467bd"]
    markers = ["o", "s", "^", "D", "v"]

    loaded_sig_data = [] 
    loaded_bg_data = []
    sig_weight_per_event = 1.0

    print("Loading inferences...")
    for j in range(num_models):
        df_all = pd.read_parquet(args.parquets[j])
        
        print(f"First 5 rows of Parquet '{args.names[j]}'")
        print(df_all.head())

        if not args.use_all:
            df_all = df_all[df_all["event"] % 2 == 1].copy()

        df_sig = df_all[df_all["label"] == 1]
        df_bg = df_all[df_all["label"] == 0]

        if j == 0 and args.use_weights and not df_sig.empty:
            sig_weight_per_event = df_sig["weight"].iloc[0]

        loaded_sig_data.append({
            "pt": df_sig["genH_pt"].values, 
            "pred": df_sig["pred"].values, 
            "weight": df_sig["weight"].values
        })
        loaded_bg_data.append({
            "pred": df_bg["pred"].values, 
            "weight": df_bg["weight"].values
        })
        
        del df_all, df_sig, df_bg
        gc.collect()

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
        
        lumi_pb = LUMI_FB * 1000.0
        xsec = XSEC_DICT.get(process, XSEC_DICT.get("jets", 1.0))
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

    print("Calculating thresholds per frame...")
    br_values = np.logspace(2, 7, num=args.frames)
    fpr_values = 1.0 / br_values
    
    thresholds_per_frame = []
    
    for frame_idx in tqdm(range(args.frames), desc="Thresholds"):
        current_fpr = fpr_values[frame_idx]
        target_passing_w = current_fpr * total_raw_bg_w
        frame_thresholds = {}
        
        for j in range(num_models):
            bg_preds = loaded_bg_data[j]["pred"]
            bg_weights = loaded_bg_data[j]["weight"]
            
            sorted_idx = np.argsort(bg_preds)[::-1]
            sorted_preds = bg_preds[sorted_idx]
            sorted_weights = bg_weights[sorted_idx]
            
            cum_w = np.cumsum(sorted_weights)
            cut_idx = np.searchsorted(cum_w, target_passing_w)
            
            if cut_idx < len(sorted_preds):
                frame_thresholds[j] = sorted_preds[cut_idx]
            else:
                frame_thresholds[j] = sorted_preds[-1] if len(sorted_preds) > 0 else 0.0
                
        thresholds_per_frame.append(frame_thresholds)

    del loaded_bg_data
    gc.collect()

    print("Computing efficiencies...")
    effs_per_frame = [[None] * args.frames for _ in range(num_models)]
    errs_per_frame = [[None] * args.frames for _ in range(num_models)]

    for frame_idx in tqdm(range(args.frames), desc="Efficiencies"):
        current_thresholds = thresholds_per_frame[frame_idx]
        for j in range(num_models):
            sig_pts = loaded_sig_data[j]["pt"]
            sig_preds = loaded_sig_data[j]["pred"]
            sig_weights = loaded_sig_data[j]["weight"]
            cut = current_thresholds[j]
            
            sig_effs, sig_errs = [], []
            for i in range(len(pt_bins) - 1):
                pt_min, pt_max = pt_bins[i], pt_bins[i + 1]
                n_generated_raw = n_sig_gen_list[i]
                w_generated = w_sig_gen_list[i]

                if n_generated_raw < 50:
                    sig_effs.append(np.nan)
                    sig_errs.append(np.nan)
                else:
                    mask_bin = (sig_pts >= pt_min) & (sig_pts < pt_max)
                    bin_preds = sig_preds[mask_bin]
                    bin_weights = sig_weights[mask_bin]
                    
                    w_passing = np.sum(bin_weights[bin_preds > cut])
                    raw_eff = w_passing / w_generated if w_generated > 0 else 0.0
                    
                    eff = np.clip(raw_eff, 0.0, 1.0)
                    variance = max(0.0, eff * (1.0 - eff))
                    err = np.sqrt(variance / n_generated_raw) if n_generated_raw > 0 else 0.0
                    
                    sig_effs.append(eff)
                    sig_errs.append(err)

            effs_per_frame[j][frame_idx] = sig_effs
            errs_per_frame[j][frame_idx] = sig_errs

    print("Setting up animation...")
    fig_eff, (ax_eff, ax_yield_eff) = plt.subplots(
        2, 1, figsize=(8, 8), sharex=True, gridspec_kw={"height_ratios": [3, 1]}, dpi=150
    )

    ax_yield_eff.bar(bin_centers, n_sig_gen_list, width=bin_widths, alpha=0.2, color="black", label="Total Generated")
    ax_yield_eff.set_xlabel(r"True Higgs $p_T$ [GeV]")
    ax_yield_eff.set_ylabel("Events")
    ax_yield_eff.grid(axis="y", linestyle=":", alpha=0.7)
    ax_yield_eff.legend(loc="upper right", fontsize=10)
    fig_eff.tight_layout()

    def update(frame_idx):
        ax_eff.clear()
        current_br = br_values[frame_idx]
        current_thresholds = thresholds_per_frame[frame_idx]
        
        exp = int(np.floor(np.log10(current_br)))
        mantissa = current_br / (10**exp)
        br_str = f"$10^{{{exp}}}$" if abs(mantissa - 1.0) < 1e-5 else f"${mantissa:.1f} \\times 10^{{{exp}}}$"

        for j in range(num_models):
            cut = current_thresholds[j]
            sig_effs = effs_per_frame[j][frame_idx]
            sig_errs = errs_per_frame[j][frame_idx]

            if cut > 0.999:
                cut_inv = 1 - cut
                c_exp = int(np.floor(np.log10(cut_inv))) if cut_inv > 0 else -10
                c_mantissa = cut_inv / (10**c_exp) if cut_inv > 0 else 0
                cut_inv_str = f"$10^{{{c_exp}}}$" if abs(c_mantissa - 1.0) < 1e-5 else f"${c_mantissa:.1f} \\times 10^{{{c_exp}}}$"
                cut_str = f"1 - {cut_inv_str}"
            else:
                cut_str = f"{cut:.3}"

            ax_eff.errorbar(
                bin_centers, sig_effs, xerr=x_err, yerr=sig_errs,
                fmt=f"{markers[j % len(markers)]}-", color=colors[j % len(colors)],
                capsize=3, label=f"{args.names[j]} (Cut: {cut_str})",
            )

        ax_eff.set_ylim([0.0, 1.05])
        ax_eff.set_ylabel("Total Eff.")
        ax_eff.legend(loc="lower right", title=f"Background Rejection = {br_str}", title_fontsize=14)
        ax_eff.grid(axis="y", which="major", linestyle="-", alpha=0.7)
        ax_eff.grid(axis="y", which="minor", linestyle=":", alpha=0.4)
        ax_eff.grid(axis="x", linestyle=":", alpha=0.7)
        hep.cms.label(args.cms_label, data=False, rlabel="13.6 TeV", ax=ax_eff, loc=0, fontsize=14)
        ax_eff.set_title(
            rf"$H \to \tau\tau$ + {get_mode_names(args.bg_mode)}", 
            x=0.62,
            fontsize=14
        )

    print(f"Animating {args.frames} frames...")
    anim = animation.FuncAnimation(fig_eff, update, frames=args.frames, interval=200)
    anim.save(args.out_plot, writer='pillow', fps=5)
    print(f"Animation saved to {args.out_plot}")
    plt.close(fig_eff)


    if "AK15" not in args.modes:
        print("'AK15' is not in the provided modes. Skipping crossover plot.")
    elif len(args.modes) < 2:
        print("Notice: No other modes provided to compare AK15 against. Skipping crossover plot.")
    else:
        print("Generating Crossover Plot...")
        idx_ak15 = args.modes.index("AK15")
        competitor_indices = [i for i, mode in enumerate(args.modes) if mode != "AK15"]

        crossover_pts_dict = {c: [] for c in competitor_indices}
        crossover_errs_dict = {c: [] for c in competitor_indices}

        num_bins = len(bin_centers)

        for frame_idx in range(args.frames):
            
            for c in competitor_indices: 
                crossover = np.nan
                crossover_err = np.nan
                comp_mode = args.modes[c]

                for b in range(num_bins):
                    eff_ak15 = effs_per_frame[idx_ak15][frame_idx][b]
                    ec = effs_per_frame[c][frame_idx][b]
                    
                    if not (np.isnan(eff_ak15) or np.isnan(ec)):

                        if comp_mode == "AK8":
                            condition_met = ec > eff_ak15
                        else:
                            condition_met = eff_ak15 > ec

                        if condition_met:
                            if b < num_bins - 1:
                                eff_ak15_next = effs_per_frame[idx_ak15][frame_idx][b+1]
                                ec_next = effs_per_frame[c][frame_idx][b+1]
                                
                                if not (np.isnan(eff_ak15_next) or np.isnan(ec_next)):
                                    if comp_mode == "AK8":
                                        condition_next = ec_next > eff_ak15_next
                                    else:
                                        condition_next = eff_ak15_next > ec_next
                                        
                                    if condition_next:
                                        crossover = bin_centers[b]
                                        crossover_err = (pt_bins[b+1] - pt_bins[b]) / 2.0
                                        break
                            else:
                                crossover = bin_centers[b]
                                crossover_err = (pt_bins[b+1] - pt_bins[b]) / 2.0
                                break

                crossover_pts_dict[c].append(crossover)
                crossover_errs_dict[c].append(crossover_err)

        fig_cross, ax_cross = plt.subplots(figsize=(8, 6), dpi=150)

        for c in competitor_indices:
            comp_name = args.names[c]
            comp_mode = args.modes[c]
            c_color = colors[c % len(colors)]
            c_marker = markers[c % len(markers)]
            
            plot_label = f"{comp_name} > AK15" if comp_mode == "AK8" else f"AK15 > {comp_name}"

            ax_cross.errorbar(
                br_values, 
                crossover_pts_dict[c], 
                yerr=crossover_errs_dict[c],
                marker=c_marker, 
                color=c_color, 
                lw=2, 
                capsize=3,
                label=plot_label
            )
        
        ax_cross.set_xscale("log")
        ax_cross.set_xlabel("Background Rejection")
        ax_cross.set_ylabel(r"Higgs $p_T$ [GeV]")
        ax_cross.legend(loc="upper left")
        
        ax_cross.grid(True, which="major", linestyle="-", alpha=0.7)
        ax_cross.grid(True, which="minor", linestyle=":", alpha=0.4)
        hep.cms.label(args.cms_label, data=False, rlabel=rf"$H \to \tau\tau$ + {get_mode_names(args.bg_mode)}", ax=ax_cross, loc=0, fontsize=14)

        cross_out = args.out_plot.rsplit('.', 1)[0] + "_crossover.png"
        fig_cross.tight_layout()
        fig_cross.savefig(cross_out)
        print(f"Crossover plot saved to {cross_out}")
        plt.close(fig_cross)

if __name__ == "__main__":
    main()