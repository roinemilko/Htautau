import argparse
import pandas as pd
import numpy as np
import xgboost as xgb
import mplhep as hep
import matplotlib.pyplot as plt
from sklearn.model_selection import train_test_split
from sklearn.metrics import roc_curve, auc, roc_auc_score
from scipy.stats import bootstrap
import sys
from uproot_data import load_tau_data
from uproot_fat import load_fatjet_data
import uproot

# TODO: Update with better values
XSEC_DICT = {
    "TTto4Q": 377.96,
    "TTtoLNu2Q": 365.34,
    "TTto2L2Nu": 88.29,
    "DYto2Tau": 6077.22,
    "VBFHHto2B2Tau": 0.0073,
}
LUMI_FB = 137.0

def get_weight(file_path):
    """Calculates the expected number of events w = (xsec * lumi) / N_gen"""
    lumi_pb = LUMI_FB * 1000.0
    xsec = next((v for k, v in XSEC_DICT.items() if k in file_path), 1.0)

    with uproot.open(f"{file_path}:Events") as tree:
        n_gen = tree.num_entries
    
    return (xsec * lumi_pb) / n_gen if n_gen > 0 else 1.0

def get_candidate_pt(df, mode):
    """Gets the reconstructed event p_T"""
    if mode == "AK8":
        return df["fj_pt"].values
    elif mode == "AK15":
        return df["ak15_pt"].values
    elif mode == "Tau":
        px = df["tau_pt_1"] * np.cos(df["tau_phi_1"]) + df["tau_pt_2"] * np.cos(df["tau_phi_2"])
        py = df["tau_pt_1"] * np.sin(df["tau_phi_1"]) + df["tau_pt_2"] * np.sin(df["tau_phi_2"])
        return np.sqrt(px**2 + py**2).values
    else:
        raise ValueError(f"Unknown mode: {mode}")

def load_data(sig_path, bg_path, mode, args, sig_vars=None, bg_vars=None):
    """Caller for data loaders"""
    if mode == "Tau":
        df_sig = load_tau_data(sig_path, label=1, num_taus=args.num_taus, variables=sig_vars)
        df_bg = load_tau_data(bg_path, label=0, num_taus=args.num_taus, variables=bg_vars)
    else:
        df_sig = load_fatjet_data(
            sig_path, label=1, jet_type=mode, use_subjets=args.use_subjets, variables=sig_vars
        )
        df_bg = load_fatjet_data(
            bg_path, label=0, jet_type=mode, use_subjets=args.use_subjets, variables=bg_vars
        )

    if args.use_weights:
        df_sig["weight"] = get_weight(sig_path)
        df_bg["weight"] = get_weight(bg_path)
    else:
        df_sig["weight"] = 1.0
        df_bg["weight"] = 1.0

    return df_sig, df_bg

def safe_auc(y, p, w=None):
    if len(np.unique(y)) < 2:
        return np.nan
    if len(np.unique(p)) == 1:
        return 0.5
    return roc_auc_score(y, p, sample_weight=w)

def get_weighted_threshold(y_true, y_pred, weights, target_fpr):
    """Returns the cut when inference is done with weighted samples"""
    fpr, tpr, thresholds = roc_curve(y_true, y_pred, sample_weight=weights)
    idx = np.where(fpr <= target_fpr)[0][-1] if len(np.where(fpr <= target_fpr)[0]) > 0 else 0
    return thresholds[idx]

def main():
    parser = argparse.ArgumentParser(
        description="Intersection-method fair evaluation of multiple models"
    )
    parser.add_argument("--sigs", nargs="+", required=True, help="Signal ROOT file per model/mode")
    parser.add_argument("--bgs", nargs="+", required=True, help="Background ROOT file per model/mode")
    parser.add_argument("--models", nargs="+", required=True, help="Paths to trained XGBoost models")
    parser.add_argument("--modes", nargs="+", required=True, choices=["Tau", "AK8", "AK15"])
    parser.add_argument("--names", nargs="+", required=True, help="Labels for the plot legend")
    parser.add_argument("--out_plot", required=True, help="Output path for AUC vs pT plot")
    parser.add_argument("--out_eff_plot", required=True, help="Output path for sig. eff. plot")
    parser.add_argument("--seed", type=int, default=100)
    parser.add_argument("--num_taus", type=int, default=2)
    parser.add_argument("--use_subjets", action="store_true")
    parser.add_argument("--fpr", type=float, default=0.01, help="Target Background False Positive Rate")
    parser.add_argument("--use_weights", action="store_true", help="Apply physical expected yields")
    parser.add_argument(
        "--use_all",
        action="store_true",
        help="Evaluate on all common events (skip 50/50 holdout)",
    )
    args = parser.parse_args()

    if not (
        len(args.sigs)
        == len(args.bgs)
        == len(args.models)
        == len(args.modes)
        == len(args.names)
    ):
        print("Error: --sigs, --bgs, --models, --modes, and --names must have the same length.")
        sys.exit(1)

    hep.style.use("CMS")
    num_models = len(args.models)

    sig_dfs = []
    bg_dfs = []
    models = []
    expected_features = []
    for i in range(num_models):
        print(f"Loading model and data for {args.names[i]} ({args.modes[i]})...")
        
        bst = xgb.Booster()
        bst.load_model(args.models[i])
        models.append(bst)
        
        features = bst.feature_names
        expected_features.append(features)
        
        req_cols = []
        if args.modes[i] == "AK8": 
            req_cols = ["fj_pt"]
        elif args.modes[i] == "AK15": 
            req_cols = ["ak15_pt"]
        elif args.modes[i] == "Tau": 
            req_cols = ["tau_pt", "tau_phi"]

        sig_req_cols = req_cols + ["genH_pt", "event"]
        bg_req_cols = req_cols + ["event"]
            
        load_vars = load_vars = sorted(set(features) | set(req_cols)) if features else None
        
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

        df_sig, df_bg = load_data(
            args.sigs[i], args.bgs[i], args.modes[i], args, sig_vars=sig_vars, bg_vars=bg_vars
        )

        df_sig = df_sig.set_index("event")  
        df_bg = df_bg.set_index("event")

        n_dup_sig = int(df_sig.index.duplicated().sum())
        n_dup_bg = int(df_bg.index.duplicated().sum())
        n_unique_sig = df_sig.index.nunique()
        n_unique_bg = df_bg.index.nunique()

        print(
            f"{args.names[i]} ({args.modes[i]}): "
            f"sig rows={len(df_sig)}, unique={n_unique_sig}, dup_rows={n_dup_sig}; "
            f"bg rows={len(df_bg)}, unique={n_unique_bg}, dup_rows={n_dup_bg}"
        )

        overlap = df_sig.index.intersection(df_bg.index)
        print(f"Duplicate event ID:s in sg/bg: {len(overlap)}")

        if n_dup_sig > 0:
            df_sig = df_sig[~df_sig.index.duplicated(keep="first")]

        if n_dup_bg > 0:
            df_bg = df_bg[~df_bg.index.duplicated(keep="first")]

        df_sig.index = pd.MultiIndex.from_arrays(
            [["sig"] * len(df_sig), df_sig.index], names=["sample", "event"]
        )
        df_bg.index = pd.MultiIndex.from_arrays(
            [["bg"] * len(df_bg), df_bg.index], names=["sample", "event"]
        )

        sig_dfs.append(df_sig)
        bg_dfs.append(df_bg)

    print("Building intersection of events")
    common_sig = sig_dfs[0].index
    common_bg = bg_dfs[0].index
    for i in range(num_models):
        common_sig = common_sig.intersection(sig_dfs[i].index)
        common_bg = common_bg.intersection(bg_dfs[i].index)

    print(f"Common signal events: {len(common_sig)}")
    print(f"Common background events: {len(common_bg)}")

    if args.use_all:
        test_sig_idx = common_sig
        test_bg_idx = common_bg
    else:
        test_sig_idx = common_sig[common_sig.get_level_values("event") % 2 == 1]
        test_bg_idx = common_bg[common_bg.get_level_values("event") % 2 == 1]

    test_idx = test_sig_idx.union(test_bg_idx)

    eval_df = pd.DataFrame(index=test_idx)
    eval_df["label"] = 0
    eval_df.loc[test_sig_idx, "label"] = 1
    
    eval_df["weight"] = 1.0
    eval_df.loc[test_sig_idx, "weight"] = sig_dfs[0].loc[test_sig_idx, "weight"].values
    eval_df.loc[test_bg_idx, "weight"] = bg_dfs[0].loc[test_bg_idx, "weight"].values

    print("Running inference...")
    for i in range(num_models):
        df_all = pd.concat([sig_dfs[i], bg_dfs[i]])
        df_test = df_all.loc[test_idx]
        
        if i == 0:
            eval_df["genH_pt"] = df_test["genH_pt"].values

        feat_names = expected_features[i]
        if feat_names is None:
            X_test = df_test.drop(columns=["label"])
        else:
            X_test = df_test[feat_names]
        dtest = xgb.DMatrix(X_test, missing=np.inf)
        preds = models[i].predict(dtest)
        
        pts = get_candidate_pt(df_test, args.modes[i])
        eval_df[f"pred_{i}"] = preds
        eval_df[f"pt_{i}"] = pts
    
    pt_cols = [f"pt_{i}" for i in range(num_models)]
    eval_df["ref_pt"] = eval_df[pt_cols].mean(axis=1)
    print(f"Final common test set size: {len(eval_df)} events.")

    if num_models > 1:
        print("Making reconstructed pT difference plot...")
        eval_df["max_pt_diff"] = eval_df[pt_cols].max(axis=1) - eval_df[pt_cols].min(axis=1)
        
        fig_pt, ax_pt = plt.subplots(figsize=(8, 6), dpi=150)
        bins_pt = np.linspace(0, 300, 60)
        
        ax_pt.hist(
            eval_df[eval_df["label"] == 1]["max_pt_diff"], 
            weights=eval_df[eval_df["label"] == 1]["weight"],
            bins=bins_pt, histtype="step", color="blue", label="Signal", linewidth=2
        )
        ax_pt.hist(
            eval_df[eval_df["label"] == 0]["max_pt_diff"], 
            weights=eval_df[eval_df["label"] == 0]["weight"],
            bins=bins_pt, histtype="step", color="red", label="Background", linewidth=2
        )
        
        ax_pt.set_xlabel(r"Maximum $\Delta p_T$ between models [GeV]")
        y_label_pt = "Expected Yield" if args.use_weights else "Events"
        ax_pt.set_ylabel(y_label_pt)
        ax_pt.legend(loc="best")
        hep.cms.label("Work in Progress", data=False, rlabel=r"$H \to \tau\tau$ (125 GeV)", ax=ax_pt, loc=0, fontsize=14)
        
        out_pt_plot = args.out_plot.replace(".png", "_pt_diff.png")
        fig_pt.tight_layout()
        fig_pt.savefig(out_pt_plot, bbox_inches="tight")
        plt.close(fig_pt)
        print(f"saved to {out_pt_plot}")


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
                        random_state=args.seed,
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
        "Work in Progress",
        data=False,
        rlabel=r"$H \to \tau\tau + tt \to qqqq$",
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
        n_sig_list_eff.append(n_matched_weighted)

        for j in range(num_models):
            if n_matched_weighted < 50:
                sig_effs[j].append(np.nan)
                sig_errs[j].append(np.nan)
            else:
                passing_weights = bin_sig.loc[bin_sig[f"pred_{j}"] > thresholds[j], "weight"].sum()
                eff = passing_weights / n_matched_weighted
                err = np.sqrt(eff * (1 - eff) / n_matched_unweighted)
                
                sig_effs[j].append(eff)
                sig_errs[j].append(err)

    fig_eff, (ax_eff, ax_yield_eff) = plt.subplots(
        2, 1, figsize=(8, 8), sharex=True,
        gridspec_kw={"height_ratios": [3, 1]}, dpi=150,
    )

    for j in range(num_models):
        ax_eff.errorbar(
            bin_centers,
            sig_effs[j],
            xerr=x_err,
            yerr=sig_errs[j],
            fmt=f"{markers[j % len(markers)]}-",
            color=colors[j % len(colors)],
            capsize=3,
            label=f"{args.names[j]} (Cut: {thresholds[j]:.3f})",
        )
        
    br = 1.0 / args.fpr
    br_str = f"{br:.0e}"
        
    ax_eff.set_ylabel(f"Signal eff.")
    ax_eff.legend(loc="best", title=f"Background Rejection {br_str}", title_fontsize=14)
    ax_eff.grid(axis="y", which="major", linestyle="-", alpha=0.7)
    ax_eff.grid(axis="y", which="minor", linestyle=":", alpha=0.4)
    ax_eff.grid(axis="x", linestyle=":", alpha=0.7)
    hep.cms.label(
        "Work in Progress", data=False, rlabel=r"$H \to \tau\tau$ (125 GeV)",
        ax=ax_eff, loc=0, fontsize=14,
    )

    ax_yield_eff.bar(bin_centers, n_sig_list_eff, width=bin_widths, alpha=0.5, label="Signal intersection", color="blue")
    ax_yield_eff.set_yscale("log")
    ax_yield_eff.set_xlabel("Higgs $p_T$ [GeV]")
    ax_yield_eff.set_ylabel("Events")
    ax_yield_eff.grid(axis="y", linestyle=":", alpha=0.7)
    ax_yield_eff.legend(loc="upper right", fontsize=10)
    fig_eff.tight_layout()
    fig_eff.savefig(args.out_eff_plot, bbox_inches="tight")
    plt.close(fig_eff)
    print(f"saved to {args.out_eff_plot}")


if __name__ == "__main__":
    main()