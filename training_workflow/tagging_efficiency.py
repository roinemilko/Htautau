import argparse
import pandas as pd
import numpy as np
import xgboost as xgb
import mplhep as hep
import matplotlib.pyplot as plt
from sklearn.model_selection import train_test_split
import sys
from uproot_fat import load_fatjet_data
from uproot_data import load_tau_data
import uproot

BG_STRING_DICT = {
    "TTto4Q": r"$tt \to qqqq$",
    "TTto2L2Nu": r"$tt \to \ell\ell\nu\nu$",
    "TTtoLNu2Q": r"$tt \to \ell \nu qq$",
    "DYto2Tau": "DY"
}

def load_data(sig_path, bg_path, mode, args, variables=None):

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
    parser = argparse.ArgumentParser(description="Tagging Efficiency vs True Higgs pT")
    parser.add_argument("--sigs", nargs="+", required=True)
    parser.add_argument("--bgs", nargs="+", required=True)
    parser.add_argument("--bg_mode", required=True, help="Name of the bg mode for plot axes")
    parser.add_argument("--models", nargs="+", required=True)
    parser.add_argument("--modes", nargs="+", required=True, choices=["Tau", "AK8", "AK15"])
    parser.add_argument("--names", nargs="+", required=True)
    parser.add_argument("--out_plot", required=True)
    parser.add_argument("--seed", type=int, default=100)
    parser.add_argument("--num_taus", type=int, default=2)
    parser.add_argument("--use_subjets", action="store_true")
    parser.add_argument("--fpr", type=float, default=0.01, help="Target Background Efficiency")
    parser.add_argument("--raw_sig", required=True, help="Path to RawEventInfo.root")
    args = parser.parse_args()

    hep.style.use("CMS")
    num_models = len(args.models)

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

        raw_genH_pt = raw_pt[raw_evts % 2 == 1]

    n_gen_list = []
    for i in range(len(pt_bins) - 1):
        mask_raw = (raw_genH_pt >= pt_bins[i]) & (raw_genH_pt < pt_bins[i + 1])
        n_gen_list.append(np.sum(mask_raw))

    all_sig_yields_mat = []
    ref_effs_mat = None
    ref_errs_mat = None

    for j in range(num_models):
        print(f"Processing {args.names[j]} ({args.modes[j]})...")
        
        bst = xgb.Booster()
        bst.load_model(args.models[j])
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
            
        df_sig, df_bg = load_data(args.sigs[j], args.bgs[j], args.modes[j], args, variables=load_vars)

        test_sig = df_sig[df_sig["event"] % 2 == 1].copy()
        test_bg = df_bg[df_bg["event"] % 2 == 1].copy()

        # Set threshold form background
        X_bg = test_bg[features] if features else test_bg.drop(columns=["label"])
        d_bg = xgb.DMatrix(X_bg, missing=np.inf)
        bg_preds = bst.predict(d_bg)
        threshold = np.percentile(bg_preds, 100 * (1 - args.fpr))

        # Predictions
        X_sig = test_sig[features] if features else test_sig.drop(columns=["label", "genH_pt", "event"])
        d_sig = xgb.DMatrix(X_sig, missing=np.inf)
        sig_preds = bst.predict(d_sig)

        sig_effs_mat, sig_errs_mat, n_sig_list_mat = [], [], []
        sig_effs_abs, sig_errs_abs = [], []
        
        genH_pt = test_sig["genH_pt"].values
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


        cut = threshold
        cut_str = None
        
        if cut > 0.999:
            cut_inv = 1 - threshold
            exp = int(np.floor(np.log10(cut_inv)))
            mantissa = cut_inv / (10**exp)
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
    ax_eff_mat.legend(loc="best", title=f"Background Rejection {br_str}", title_fontsize=14)
    ax_eff_mat.grid(axis="y", which="major", linestyle="-", alpha=0.7)
    ax_eff_mat.grid(axis="y", which="minor", linestyle=":", alpha=0.4)
    ax_eff_mat.grid(axis="x", linestyle=":", alpha=0.7)
    hep.cms.label("Work in Progress", data=False, rlabel=rf"$H \to \tau\tau$ + {BG_STRING_DICT[args.bg_mode]}", ax=ax_eff_mat, loc=0, fontsize=14)

    ax_ratio_mat.set_ylabel(f"/{args.modes[0]}")
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
    
    ax_eff_abs.set_ylabel(f"Reconstruciton eff.@ {args.fpr*100:.1f}%")
    ax_eff_abs.legend(loc="best")
    ax_eff_abs.grid(axis="y", which="major", linestyle="-", alpha=0.7)
    ax_eff_abs.grid(axis="y", which="minor", linestyle=":", alpha=0.4)
    ax_eff_abs.grid(axis="x", linestyle=":", alpha=0.7)
    hep.cms.label("Work in Progress", data=False, rlabel=rf"$H \to \tau\tau$ + {BG_STRING_DICT[args.bg_mode]}", ax=ax_eff_abs, loc=0, fontsize=14)
    ax_yield_abs.set_yscale("log")
    ax_yield_abs.set_xlabel(r"Higgs $p_T$ [GeV]")
    ax_yield_abs.set_ylabel("Events")
    ax_yield_abs.grid(axis="y", linestyle=":", alpha=0.7)
    ax_yield_abs.legend(loc="upper right", fontsize=8, ncol=2)
    fig_abs.tight_layout()
    out_name_abs = args.out_plot.replace(".png", f"_absolute.png")
    fig_abs.savefig(out_name_abs, bbox_inches="tight")
    
    plt.close(fig_mat)
    plt.close(fig_abs)
    print(f"Matched Tagging Efficiency plot saved to {out_name_mat}")
    print(f"Absolute Tagging Efficiency plot saved to {out_name_abs}")

if __name__ == "__main__":
    main()