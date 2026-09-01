import argparse
import pandas as pd
import numpy as np
import mplhep as hep
import matplotlib.pyplot as plt
from sklearn.metrics import roc_curve, auc, roc_auc_score
from scipy.stats import bootstrap
import sys
from plot_helpers import *
import gc
from datetime import datetime

def main():
    parser = argparse.ArgumentParser(
        description="Intersection-method fair evaluation of multiple models from Parquet"
    )
    parser.add_argument("--parquets", nargs="+", required=True, help="List of Parquet inference results")
    parser.add_argument("--modes", nargs="+", required=True, choices=["Tau", "AK8", "AK15"])
    parser.add_argument("--names", nargs="+", required=True, help="Labels for the plot legend")
    parser.add_argument("--bg_mode", required=True, help="Name of the bg mode for plot axes")
    parser.add_argument("--out_plot", required=True, help="Output path for AUC vs pT plot")
    parser.add_argument("--out_eff_plot", required=True, help="Output path for sig. eff. plot")
    parser.add_argument("--num_taus", type=int, default=2)
    parser.add_argument("--use_subjets", action="store_true")
    parser.add_argument("--cms_label", default="Work in Progress")
    parser.add_argument("--fpr", type=float, default=0.01, help="Target Background False Positive Rate")
    parser.add_argument("--use_weights", action="store_true", help="Apply physical expected yields")
    parser.add_argument(
        "--use_all",
        action="store_true",
        help="Evaluate on all common events (skip 50/50 holdout)",
    )
    args = parser.parse_args()

    if not (len(args.parquets) == len(args.modes) == len(args.names)):
        print("Error: --parquets, --modes, and --names must have the same length.")
        sys.exit(1)

    hep.style.use("CMS")
    num_models = len(args.parquets)

    df_evals = []

    for i in range(num_models):
        print(f"Loading inference for {args.names[i]}...")

        df_computed = pd.read_parquet(args.parquets[i])

        if not args.use_all:
            df_computed = df_computed[df_computed["event"] % 2 == 1]

        df_computed["sample"] = np.where(df_computed["label"] == 1, "sig", "bg")
        df_computed = df_computed.set_index(["sample", "event"])

        n_dup = int(df_computed.index.duplicated().sum())
        print(f"Evaluated rows={len(df_computed)}, unique={len(df_computed) - n_dup}, dup_rows={n_dup}")

        if n_dup > 0:
            df_computed = df_computed[~df_computed.index.duplicated(keep="first")]

        df_evals.append(df_computed)
        gc.collect()

    print("Building intersection")
    common_idx = df_evals[0].index
    for i in range(1, num_models):
        common_idx = common_idx.intersection(df_evals[i].index)

    print(f"Common signal events: {sum(common_idx.get_level_values('sample') == 'sig')}")
    print(f"Common background events: {sum(common_idx.get_level_values('sample') == 'bg')}")

    eval_df = pd.DataFrame(index=common_idx)
    eval_df["label"] = np.where(common_idx.get_level_values("sample") == "sig", 1, 0)
    eval_df["weight"] = df_evals[0].loc[common_idx, "weight"].values
    eval_df["genH_pt"] = df_evals[0].loc[common_idx, "genH_pt"].values

    sig_weight_val = eval_df.loc[eval_df["label"] == 1, "weight"].iloc[0] if not eval_df[eval_df["label"] == 1].empty else "N/A"
    bg_weight_val = eval_df.loc[eval_df["label"] == 0, "weight"].iloc[0] if not eval_df[eval_df["label"] == 0].empty else "N/A"
    print(f"Signal weight applied: {sig_weight_val}")
    print(f"Background weight applied: {bg_weight_val}")

    for i in range(num_models):
        eval_df[f"pred_{i}"] = df_evals[i].loc[common_idx, "pred"].values
        eval_df[f"pt_{i}"] = df_evals[i].loc[common_idx, "obj_pt"].values

    del df_evals
    gc.collect()

    pt_cols = [f"pt_{i}" for i in range(num_models)]
    eval_df["ref_pt"] = eval_df[pt_cols].mean(axis=1)
    print(f"Final common test set size: {len(eval_df)} events.")

    pt_bins = [
        200, 250, 300, 350, 400, 450, 500, 550, 600, 650, 700,
        750, 800, 850, 900, 950, 1000, 1100, 1200,
    ]

    bin_centers = [(pt_bins[i] + pt_bins[i + 1]) / 2.0 for i in range(len(pt_bins) - 1)]
    x_err = [bin_centers[i] - pt_bins[i] for i in range(len(bin_centers))]
    
    n_sig_list = []
    n_bg_list = []
    model_errors = {i: [] for i in range(num_models)}
    model_aucs = {i: [] for i in range(num_models)}

    print("Making AUC vs. reconstructed eff. plot...-")

    for i in range(len(pt_bins) - 1):
        pt_min, pt_max = pt_bins[i], pt_bins[i + 1]
        mask = (eval_df["ref_pt"] >= pt_min) & (eval_df["ref_pt"] < pt_max)
        bin_df = eval_df[mask]
        y_true_bin = bin_df["label"].values
        w_bin = bin_df["weight"].values

        n_sig_list.append(np.sum(y_true_bin == 1))
        n_bg_list.append(np.sum(y_true_bin == 0))

        for j in range(num_models):
            y_pred_bin = bin_df[f"pred_{j}"].values
            if np.sum(y_true_bin == 1) < 50 or np.sum(y_true_bin == 0) < 50:  
                model_aucs[j].append(np.nan)
                model_errors[j].append(np.nan)
                continue
            if len(np.unique(y_true_bin)) >= 2:
                nominal_auc = safe_auc(y_true_bin, y_pred_bin, w = w_bin)
                model_aucs[j].append(nominal_auc)
                try:
                    res = bootstrap(
                        (y_true_bin, y_pred_bin, w_bin),
                        safe_auc,
                        vectorized=False,
                        paired=True,
                        n_resamples=50,
                        random_state=datetime.now(),
                        method="percentile",
                    )
                    model_errors[j].append(res.standard_error)
                except ValueError:
                    model_errors[j].append(0.0)
            else:
                model_aucs[j].append(np.nan)
                model_errors[j].append(np.nan)

    fig, (ax_auc, ax_ratio, ax_yield) = plt.subplots(
        3, 1, figsize=(8, 10), sharex=True,
        gridspec_kw={"height_ratios": [3, 1, 1]}, dpi=150,
    )
    colors = ["#1f77b4", "#ff7f0e", "#2ca02c", "#d62728", "#9467bd"]
    markers = ["o", "s", "^", "D", "v"]

    for j in range(num_models):
        ax_auc.errorbar(
            bin_centers,
            model_aucs[j],
            xerr=x_err,
            yerr=model_errors[j],
            fmt=f"{markers[j % len(markers)]}-",
            color=colors[j % len(colors)],
            capsize=3,
            label=args.names[j],
        )

        if j != 0:
            auc_j = np.array(model_aucs[j])
            auc_0 = np.array(model_aucs[0])
            err_j = np.array(model_errors[j])
            err_0 = np.array(model_errors[0])

            with np.errstate(divide='ignore', invalid='ignore'):
                y_ratio = auc_j / auc_0
                y_err_ratio = y_ratio * np.sqrt((err_j / auc_j)**2 + (err_0 / auc_0)**2)

            ax_ratio.errorbar(
                    bin_centers,
                    y_ratio,
                    xerr=x_err,
                    yerr=y_err_ratio,
                    fmt=f"{markers[j % len(markers)]}-",
                    color=colors[j % len(colors)],
                    capsize=3,
                )

    ax_auc.set_ylabel("AUC")
    ax_auc.set_yscale("log")
    ax_auc.legend(loc="best")
    ax_auc.grid(axis="y", which="major", linestyle="-", alpha=0.7)
    ax_auc.grid(axis="y", which="minor", linestyle=":", alpha=0.4)
    ax_auc.grid(axis="x", linestyle=":", alpha=0.7)
    hep.cms.label(
        args.cms_label,
        data=False,
        rlabel=rf"$H \to \tau\tau$ + {get_mode_names(args.bg_mode)}",
        ax=ax_auc,
        loc=0,
        fontsize=14,
    )

    ax_ratio.axhline(1.0, color="black", linestyle="--", alpha=0.5)
    
    ax_ratio.set_ylabel(f"Ratio to {args.names[0]}") 
    ax_ratio.grid(axis="y", linestyle=":", alpha=0.7)
    ax_ratio.grid(axis="x", linestyle=":", alpha=0.7)
    
    ax_ratio.ticklabel_format(axis='y', useOffset=False)

    bin_widths = [pt_bins[i + 1] - pt_bins[i] for i in range(len(pt_bins) - 1)]
    ax_yield.bar(bin_centers, n_sig_list, width=bin_widths, alpha=0.5, label="Signal", color="blue")
    ax_yield.bar(bin_centers, n_bg_list, width=bin_widths, alpha=0.5, label="Background", color="red")

    ax_yield.set_yscale("log")
    ax_yield.set_xlabel("Average Reconstructed Object $p_T$ [GeV]")
    ax_yield.set_ylabel("Total Events")
    ax_yield.grid(axis="y", linestyle=":", alpha=0.7)
    ax_yield.legend(loc="upper right", fontsize=10)
    fig.tight_layout()
    fig.savefig(args.out_plot, bbox_inches="tight")
    plt.close()
    print(f"saved to {args.out_plot}")



    print(f"Making signal eff. vs. genHpt plot  at {args.fpr * 100:.1f}%...")

    thresholds = {}
    bg_df = eval_df[eval_df["label"] == 0]
    for j in range(num_models):
        thresholds[j] = get_weighted_threshold(
            y_true=eval_df["label"].values, 
            y_pred=eval_df[f"pred_{j}"].values, 
            weights=eval_df["weight"].values, 
            target_fpr=args.fpr
        )

    sig_effs = {j: [] for j in range(num_models)}
    sig_errs = {j: [] for j in range(num_models)}
    n_sig_list_eff = []

    sig_df = eval_df[eval_df["label"] == 1]

    for i in range(len(pt_bins) - 1):
        pt_min, pt_max = pt_bins[i], pt_bins[i + 1]
        mask = (sig_df["genH_pt"] >= pt_min) & (sig_df["genH_pt"] < pt_max)
        bin_sig = sig_df[mask]

        n_matched_unweighted = len(bin_sig)
        n_matched_weighted = bin_sig["weight"].sum()
        n_sig_list_eff.append(n_matched_unweighted)

        for j in range(num_models):
            if n_matched_unweighted < 50:
                sig_effs[j].append(np.nan)
                sig_errs[j].append(np.nan)
            else:
                passing_weights = bin_sig.loc[bin_sig[f"pred_{j}"] > thresholds[j], "weight"].sum()
                
                raw_eff = passing_weights / n_matched_weighted
                eff = np.clip(raw_eff, 0.0, 1.0)
                
                variance = max(0.0, eff * (1.0 - eff))
                err = np.sqrt(variance / n_matched_unweighted)
                
                sig_effs[j].append(eff)
                sig_errs[j].append(err)

    fig_eff, (ax_eff, ax_eff_rat, ax_yield_eff) = plt.subplots(
        3, 1, figsize=(8, 8), sharex=True,
        gridspec_kw={"height_ratios": [3, 1, 1]}, dpi=150,
    )

    ax_inset = ax_eff.inset_axes([0.10, 0.10, 0.42, 0.50])
    global_min_eff = 1.0

    for j in range(num_models):

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
            bin_centers,
            sig_effs[j],
            xerr=x_err,
            yerr=sig_errs[j],
            fmt=f"{markers[j % len(markers)]}-",
            color=colors[j % len(colors)],
            capsize=3,
            label=f"{args.names[j]} (Cut: {cut_str})",
        )

        ax_inset.errorbar(
            bin_centers,
            sig_effs[j],
            xerr=x_err,
            yerr=sig_errs[j],
            fmt=f"{markers[j % len(markers)]}-",
            color=colors[j % len(colors)],
            capsize=3,
        )

        valid_effs = [e for e in sig_effs[j] if not np.isnan(e)]
        if valid_effs:
            global_min_eff = min(global_min_eff, min(valid_effs))

        if j != 0:
            eff_j = np.array(sig_effs[j])
            eff_0 = np.array(sig_effs[0])
            err_j = np.array(sig_errs[j])
            err_0 = np.array(sig_errs[0])

            with np.errstate(divide='ignore', invalid='ignore'):
                y_ratio = eff_j / eff_0
                y_err_ratio = y_ratio * np.sqrt((err_j / eff_j)**2 + (err_0 / eff_0)**2)

            ax_eff_rat.errorbar(
                    bin_centers,
                    y_ratio,
                    xerr=x_err,
                    yerr=y_err_ratio,
                    fmt=f"{markers[j % len(markers)]}-",
                    color=colors[j % len(colors)],
                    capsize=3,
                )
        
    br = 1.0 / args.fpr
    br_str = f"{br:.0e}"
        
    ax_eff.set_ylabel(f"Signal eff.")
    ax_eff.legend(
        loc="center right", 
        title=f"Background Rejection {br_str}", 
        title_fontsize=14, 
        fontsize=12
    )
    ax_eff.grid(axis="y", which="major", linestyle="-", alpha=0.7)
    ax_eff.grid(axis="y", which="minor", linestyle=":", alpha=0.4)
    ax_eff.grid(axis="x", linestyle=":", alpha=0.7)
    ax_eff.set_ylim(0.0, 1.06)
    ax_eff.set_title(rf"$H \to \tau\tau$ + {get_mode_names(args.bg_mode)}", loc="center", fontsize=14)
    hep.cms.label(
        args.cms_label, data=False, rlabel="13.6 TeV",
        ax=ax_eff, loc=0, fontsize=14,
    )

    if global_min_eff > 0.75:
        ax_inset.set_ylim(0.70, 1.05)
        ax_inset.set_xlim(200, 500.0) 
        ax_inset.grid(axis="both", linestyle=":", alpha=0.5)
        ax_inset.tick_params(axis='both', labelsize=10)
    else:
        ax_inset.remove()

    
    ax_eff_rat.axhline(1.0, color="black", linestyle="--", alpha=0.5)
    
    ax_eff_rat.set_ylabel(f"/{args.names[0]}") 
    ax_eff_rat.grid(axis="y", linestyle=":", alpha=0.7)
    ax_eff_rat.grid(axis="x", linestyle=":", alpha=0.7)

    ax_yield_eff.bar(bin_centers, n_sig_list_eff, width=bin_widths, alpha=0.5, label="Signal intersection", color="blue")
    ax_yield_eff.set_yscale("log")
    ax_yield_eff.set_xlabel("Higgs $p_T$ [GeV]")
    ax_yield_eff.set_ylabel("Events")
    ax_yield_eff.grid(axis="y", linestyle=":", alpha=0.7)
    ax_yield_eff.legend(loc="upper right", fontsize=10)
    fig_eff.tight_layout()
    fig_eff.savefig(args.out_eff_plot, bbox_inches="tight")
    

    plt.close("all")
    gc.collect()
    print(f"savecleard to {args.out_eff_plot}")


if __name__ == "__main__":
    main()