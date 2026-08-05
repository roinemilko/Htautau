import argparse
import pandas as pd
import numpy as np
import xgboost as xgb
import mplhep as hep
import matplotlib.pyplot as plt
from sklearn.model_selection import train_test_split
from sklearn.metrics import roc_curve, auc, confusion_matrix, ConfusionMatrixDisplay, roc_auc_score
from scipy.stats import bootstrap
from uproot_data import load_tau_data
from uproot_fat import load_fatjet_data



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
    else:
        raise ValueError("Unknown mode")

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--sig", required=True)
    parser.add_argument("--bg", required=True)
    parser.add_argument("--model", required=True)
    parser.add_argument("--out_roc", required=True)
    parser.add_argument("--out_feat", required=True)
    parser.add_argument("--out_conf", required=True)
    parser.add_argument("--out_auc_pt", required=True)
    parser.add_argument("--seed", type=int, default=100)
    parser.add_argument("--variables", nargs='+', default=None, help="List of variables to load")
    parser.add_argument("--num_taus", type=int, default=2, help="Number of taus per event")
    parser.add_argument("--mode", default="tau", choices=["Tau", "AK8", "AK15"], help="Object type to process")
    parser.add_argument("--use_subjets", action="store_true", help="Require 2 subjets and load subjet features (AK8/AK15 only)")
    args = parser.parse_args()

    hep.style.use("CMS")
    # Reconstruct the exact test set used during training  

    if args.mode == "Tau":
        if args.variables:
            vars = sorted(set(args.variables) | {"tau_phi"})
            df_sig = load_tau_data(args.sig, label=1, num_taus=args.num_taus, variables=vars)
            df_bg  = load_tau_data(args.bg, label=0, num_taus=args.num_taus, variables=vars)
        else:
            df_sig = load_tau_data(args.sig, label=1, num_taus=args.num_taus)
            df_bg  = load_tau_data(args.bg, label=0, num_taus=args.num_taus)

    if args.mode == "AK8" or args.mode == "AK15":
        if args.variables:
            df_sig = load_fatjet_data(args.sig, label=1, variables=args.variables, jet_type=args.mode, use_subjets=args.use_subjets)
            df_bg  = load_fatjet_data(args.bg, label=0, variables=args.variables, jet_type=args.mode, use_subjets=args.use_subjets)
        else:
            df_sig = load_fatjet_data(args.sig, label=1, jet_type=args.mode, use_subjets=args.use_subjets)
            df_bg  = load_fatjet_data(args.bg, label=0, jet_type=args.mode, use_subjets=args.use_subjets)

    df_all = pd.concat([df_sig, df_bg], ignore_index=True)

    X = df_all.drop(columns=['label'])

    df_test = df_all[df_all["event"] % 2 == 1].copy()
    X_test = df_test.drop(columns=['label'])
    y_test = df_test['label']

    bad_inf = X_test.columns[np.isinf(X_test).any()]
    bad_nan = X_test.columns[X_test.isna().any()]
    print("Columns with inf:", list(bad_inf))
    print("Columns with NaN:", list(bad_nan))
    print("Rows with inf:", np.isinf(X_test).any(axis=1).sum())
    print("Rows with NaN:", np.isnan(X_test).any(axis=1).sum())


    # Load trained model
    model = xgb.Booster()
    model.load_model(args.model)
    feat = model.feature_names
    X_pred = X_test[feat] if feat is not None else X_test
    dtest = xgb.DMatrix(X_pred, missing=np.inf)
    y_pred_prob = model.predict(dtest)

    # ROC Curve
    fig, ax = plt.subplots(figsize=(8, 7), dpi=150)
    fpr, tpr, _ = roc_curve(y_test, y_pred_prob)
    roc_auc = auc(fpr, tpr)
    ax.plot(fpr, tpr, label=f'AUC = {roc_auc:.8f}')
    ax.plot([0, 1], [0, 1], linestyle='--')
    ax.set_xlabel('Background efficiency')
    ax.set_ylabel('Signal efficiency')
    ax.legend()
    hep.cms.label("Work in Progress", data=False, rlabel=r"$H \to \tau\tau$ (125 GeV)", ax=ax, loc=0, fontsize=14)
    fig.tight_layout()
    fig.savefig(args.out_roc, bbox_inches="tight")
    plt.close()

    # Confusion Matrix (treshold at 0.5)
    fig, ax = plt.subplots(figsize=(7, 6), dpi=150)
    y_pred_class = (y_pred_prob > 0.5).astype(int)
    cm = confusion_matrix(y_test, y_pred_class)
    disp = ConfusionMatrixDisplay(confusion_matrix=cm, display_labels=['Background', 'Signal'])
    disp.plot(ax = ax, cmap='Blues', colorbar = False)
    hep.cms.label("Work in Progress", data=False, rlabel=r"$H \to \tau\tau$ (125 GeV)", ax=ax, loc=0, fontsize=14) 
    fig.tight_layout()
    fig.savefig(args.out_conf, bbox_inches="tight")
    plt.close()

    # Feature Importance
    scores = model.get_score(importance_type='gain')
    feature_names = list(X.columns)

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
    hep.cms.label("Work in Progress", data=False,
                  rlabel=r"$H \to \tau\tau$ (125 GeV)", ax=ax, loc=0, fontsize=12)
    fig.subplots_adjust(left=0.32, top=0.95, bottom=0.05, right=0.95)
    fig.savefig(args.out_feat, bbox_inches="tight")
    plt.close()


    # AUC vs. jet pt
    obj_pt = get_candidate_pt(X_test, args.mode, args.num_taus)
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
        
        y_true_bin = y_test[mask].values
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
                    random_state=args.seed,
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

if __name__ == "__main__":
    main()
