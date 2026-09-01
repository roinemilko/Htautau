import argparse
import pandas as pd
import numpy as np
import mplhep as hep
import matplotlib.pyplot as plt
from sklearn.metrics import roc_curve, auc, roc_auc_score
from scipy.stats import bootstrap
import sys
import uproot
from plot_helpers import *
import gc

def main():
    parser = argparse.ArgumentParser(description="Tagging Efficiency vs True Higgs pT")
    parser.add_argument("--parquets", nargs="+", required=True)
    parser.add_argument("--bg_mode", required=True, help="Name of the bg mode for plot axes")
    parser.add_argument("--modes", nargs="+", required=True, choices=["Tau", "AK8", "AK15"])
    parser.add_argument("--names", nargs="+", required=True)
    parser.add_argument("--out_plot", required=True)
    parser.add_argument("--num_taus", type=int, default=2)
    parser.add_argument("--use_subjets", action="store_true")
    parser.add_argument("--fpr", type=float, default=0.01, help="Target Background Efficiency")
    parser.add_argument("--raw_sig", required=True, help="Path to RawEventInfo.root")
    parser.add_argument("--use_weights", action="store_true", help="Apply cross-section weights")
    parser.add_argument("--cms_label", default="Work in Progress")
    parser.add_argument("--use_all", action="store_true", required=True)
    args = parser.parse_args()

    hep.style.use("CMS")
    num_models = len(args.parquets)

    pt_bins = [
        200, 250, 300, 350, 400, 450, 500, 550, 600, 650, 700,
        750, 800, 850, 900, 950, 1000, 1100, 1200,
    ]
    bin_centers = [(pt_bins[i] + pt_bins[i + 1]) / 2.0 for i in range(len(pt_bins) - 1)]
    bin_widths = [pt_bins[i + 1] - pt_bins[i] for i in range(len(pt_bins) - 1)]
    x_err = [bin_centers[i] - pt_bins[i] for i in range(len(bin_centers))]
    
    fig_mat, (ax_eff_mat, ax_ratio_mat, ax_yield_mat) = plt.subplots(
        3, 1, figsize=(8, 10), sharex=True, gridspec_kw={"height_ratios": [3, 1, 1]}, dpi=150,
    )
    fig_abs, (ax_eff_abs, ax_yield_abs) = plt.subplots(
        2, 1, figsize=(8, 8), sharex=True, gridspec_kw={"height_ratios": [3, 1]}, dpi=150,
    )

    colors = ["#1f77b4", "#ff7f0e", "#2ca02c", "#d62728", "#9467bd"]
    markers = ["o", "s", "^", "D", "v"]

    print("Loading raw event info...")
    with uproot.open(f"{args.raw_sig}:Events") as raw_tree:
        raw_evts = raw_tree["event"].array(library="np")
        raw_pt = raw_tree["genH_pt_raw"].array(library="np")
        raw_genH_pt = raw_pt[raw_evts % 2 == 1] if not args.use_all else raw_pt

    n_gen_list = []
    for i in range(len(pt_bins) - 1):
        mask_raw = (raw_genH_pt >= pt_bins[i]) & (raw_genH_pt < pt_bins[i + 1])
        n_gen_list.append(np.sum(mask_raw))

    all_sig_yields_mat = []
    ref_effs_mat = None
    ref_errs_mat = None

    ax_inset = ax_eff_mat.inset_axes([0.10, 0.1, 0.45, 0.50])

    for j in range(num_models):
        print(f"Processing {args.names[j]}...")
        
        df_all = pd.read_parquet(args.parquets[j])

        df_test = df_all[df_all["event"] % 2 == 1] if not args.use_all else df_all
        del df_all
        gc.collect()
        
        y_test = df_test["label"].values
        w_test = df_test["weight"].values
        all_preds = df_test["pred"].values
        
        if "weight" in df_test.columns:
            print(f"model {j} sig weight: {df_test[y_test == 1]['weight'].iloc[0]}")
            print(f"model {j} bg weight: {df_test[y_test == 0]['weight'].iloc[0]}")

        threshold = get_weighted_threshold(y_test, all_preds, w_test, args.fpr)

        sig_preds = all_preds[y_test == 1]
        genH_pt = df_test.loc[y_test == 1, "genH_pt"].values

        sig_effs_mat, sig_errs_mat, n_sig_list_mat = [], [], []
        sig_effs_abs, sig_errs_abs = [], []
        
        for i in range(len(pt_bins) - 1):
            pt_min, pt_max = pt_bins[i], pt_bins[i + 1]
            
            mask = (genH_pt >= pt_min) & (genH_pt < pt_max)
            n_matched = np.sum(mask)
            n_generated = n_gen_list[i]
            n_sig_list_mat.append(n_matched)
                
            if n_matched < 50:
                sig_effs_mat.append(np.nan)
                sig_errs_mat.append(np.nan)
            else:
                bin_preds = sig_preds[mask]
                n_tagged = np.sum(bin_preds > threshold)
                eff_mat = n_tagged / n_matched
                err_mat = np.sqrt(eff_mat * (1 - eff_mat) / n_matched)
                sig_effs_mat.append(eff_mat)
                sig_errs_mat.append(err_mat)

            if n_generated < 50:
                sig_effs_abs.append(np.nan)
                sig_errs_abs.append(np.nan)
            else:
                bin_preds = sig_preds[mask]
                n_tagged = np.sum(bin_preds > threshold)
                eff_abs = n_tagged / n_generated
                err_abs = np.sqrt(eff_abs * (1 - eff_abs) / n_generated)
                sig_effs_abs.append(eff_abs)
                sig_errs_abs.append(err_abs)
            
        all_sig_yields_mat.append(n_sig_list_mat)
        global_min_eff = 1.0
        cut = threshold
        cut_str = None
        
        if cut > 0.999:
            cut_inv = 1 - threshold
            exp = int(np.floor(np.log10(cut_inv))) if cut_inv > 0 else -10
            mantissa = cut_inv / (10**exp) if cut_inv > 0 else 0
            if abs(mantissa - 1.0) < 1e-5:
                cut_inv_str = f"$10^{{{exp}}}$"
            else:
                cut_inv_str = f"${mantissa:.1f} \\times 10^{{{exp}}}$"
            cut_str = f"1 - {cut_inv_str}"
        else:
            cut_str = f"{cut:.3}"

        ax_eff_mat.errorbar(
            bin_centers, sig_effs_mat, xerr=x_err, yerr=sig_errs_mat,
            fmt=f"{markers[j % len(markers)]}-", color=colors[j % len(colors)],
            capsize=3, label=f"{args.names[j]} (Cut: {cut_str})",
        )

        ax_inset.errorbar(
            bin_centers, sig_effs_mat, xerr=x_err, yerr=sig_errs_mat,
            fmt=f"{markers[j % len(markers)]}-", color=colors[j % len(colors)],
            capsize=3
        )

        valid_effs = [e for e in sig_effs_mat if not np.isnan(e)]
        if valid_effs:
            global_min_eff = min(global_min_eff, min(valid_effs))
        
        ax_eff_abs.errorbar(
            bin_centers, sig_effs_abs, xerr=x_err, yerr=sig_errs_abs,
            fmt=f"{markers[j % len(markers)]}-", color=colors[j % len(colors)],
            capsize=3, label=f"{args.names[j]} (Cut: {cut_str})",
        )

        effs_arr = np.array(sig_effs_mat)
        errs_arr = np.array(sig_errs_mat)

        if j == 0:
            ref_effs_mat = effs_arr
            ref_errs_mat = errs_arr
            ax_ratio_mat.axhline(1.0, color='black', linestyle='--')
        else:
            with np.errstate(divide='ignore', invalid='ignore'):
                ratio_mat = effs_arr / ref_effs_mat
                ratio_err_mat = ratio_mat * np.sqrt(
                    (errs_arr / effs_arr)**2 + (ref_errs_mat / ref_effs_mat)**2
                )
            
            ax_ratio_mat.errorbar(
                bin_centers, ratio_mat, xerr=x_err, yerr=ratio_err_mat,
                fmt=f"{markers[j % len(markers)]}", color=colors[j % len(colors)],
                capsize=3
            )

        del df_test, all_preds, sig_preds, genH_pt, y_test, w_test
        gc.collect()

    ax_yield_abs.bar(bin_centers, n_gen_list, width=bin_widths, alpha=0.2, color="black", label="Total events")
    for j in range(num_models):
        ax_yield_mat.hist(
            bin_centers, bins=pt_bins, weights=all_sig_yields_mat[j],
            histtype="step", color=colors[j % len(colors)], linewidth=1.5,
            label=f"Matched {args.names[j]}"
        )
        
    for j in range(num_models):
        ax_yield_abs.hist(
            bin_centers, bins=pt_bins, weights=all_sig_yields_mat[j],
            histtype="step", color=colors[j % len(colors)], linewidth=1.5,
            label=f"Matched {args.names[j]}"
        )

    br = 1.0 / args.fpr
    br_str = f"{br:.0e}"

    ax_eff_mat.set_ylabel(f"Tagging Eff.")
    ax_eff_mat.legend(
        loc="center right", 
        title=f"Background Rejection {br_str}", 
        title_fontsize=14, 
        fontsize=12
    )
    ax_eff_mat.grid(axis="y", which="major", linestyle="-", alpha=0.7)
    ax_eff_mat.grid(axis="y", which="minor", linestyle=":", alpha=0.4)
    ax_eff_mat.grid(axis="x", linestyle=":", alpha=0.7)
    ax_eff_mat.set_ylim(0.0, 1.06)
    hep.cms.label(args.cms_label, data=False, rlabel="13.6 TeV", ax=ax_eff_mat, loc=0, fontsize=14)
    ax_eff_mat.set_title(rf"$H \to \tau\tau$ + {get_mode_names(args.bg_mode)}", loc="center", fontsize=14)

    if global_min_eff > 0.8:
        ax_inset.set_ylim(0.8, 1.01)
        ax_inset.set_xlim(200, 600.0) 
        ax_inset.grid(axis="both", linestyle=":", alpha=0.5)
        ax_inset.tick_params(axis='both', labelsize=10)
    else:
        ax_inset.remove()


    ax_ratio_mat.set_ylabel(f"/{args.names[0]}")
    ax_ratio_mat.grid(axis="y", which="major", linestyle="-", alpha=0.7)
    ax_ratio_mat.grid(axis="x", linestyle=":", alpha=0.7)

    ax_yield_mat.set_yscale("log")
    ax_yield_mat.set_xlabel(r"Higgs $p_T$ [GeV]")
    ax_yield_mat.set_ylabel("Events")
    ax_yield_mat.grid(axis="y", linestyle=":", alpha=0.7)
    ax_yield_mat.legend(loc="upper right", fontsize=8, ncol=2)
    fig_mat.tight_layout()
    out_name_mat = args.out_plot.replace(".png", f"_matched.png")
    fig_mat.savefig(out_name_mat, bbox_inches="tight")
    
    ax_eff_abs.set_ylabel(f"Reconstruction eff.@ {args.fpr*100:.1f}%")
    ax_eff_abs.legend(loc="best")
    ax_eff_abs.grid(axis="y", which="major", linestyle="-", alpha=0.7)
    ax_eff_abs.grid(axis="y", which="minor", linestyle=":", alpha=0.4)
    ax_eff_abs.grid(axis="x", linestyle=":", alpha=0.7)
    hep.cms.label(args.cms_label + "  " + rf"$\ \ \ H \to \tau\tau$ + {get_mode_names(args.bg_mode)}", data=False, rlabel="13.6 TeV", ax=ax_eff_mat, loc=0, fontsize=14)
    ax_yield_abs.set_yscale("log")
    ax_yield_abs.set_xlabel(r"Higgs $p_T$ [GeV]")
    ax_yield_abs.set_ylabel("Events")
    ax_yield_abs.grid(axis="y", linestyle=":", alpha=0.7)
    ax_yield_abs.legend(loc="upper right", fontsize=8, ncol=2)
    fig_abs.tight_layout()
    out_name_abs = args.out_plot.replace(".png", f"_absolute.png")
    fig_abs.savefig(out_name_abs, bbox_inches="tight")
    
    plt.close('all')
    gc.collect()
    print(f"Matched Tagging Efficiency plot saved to {out_name_mat}")
    print(f"Absolute Tagging Efficiency plot saved to {out_name_abs}")

if __name__ == "__main__":
    main()