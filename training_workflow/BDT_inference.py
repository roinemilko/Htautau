import argparse
import pandas as pd
import numpy as np
import xgboost as xgb
import mplhep as hep
import matplotlib.pyplot as plt
from sklearn.model_selection import train_test_split
from sklearn.metrics import roc_curve, auc, confusion_matrix, ConfusionMatrixDisplay
from uproot_data import load_tau_data



def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--sig", required=True)
    parser.add_argument("--bg", required=True)
    parser.add_argument("--model", required=True)
    parser.add_argument("--out_roc", required=True)
    parser.add_argument("--out_feat", required=True)
    parser.add_argument("--out_conf", required=True)
    parser.add_argument("--seed", type=int, default=100)
    parser.add_argument("--variables", nargs='+', default=None, help="List of variables to load")
    parser.add_argument("--num_taus", type=int, default=2, help="Number of taus per event")
    args = parser.parse_args()

    hep.style.use("CMS")

    # Reconstruct the exact test set used during training
    df_sig = load_tau_data(args.sig, label=1, num_taus=args.num_taus, variables=args.variables)
    df_bg = load_tau_data(args.bg, label=0, num_taus=args.num_taus, variables=args.variables)
    df_all = pd.concat([df_sig, df_bg], ignore_index=True)

    X = df_all.drop(columns=['label'])
    y = df_all['label']
    _, X_test, _, y_test = train_test_split(X, y, test_size=0.5, random_state=args.seed)

    bad_inf = X_test.columns[np.isinf(X_test).any()]
    bad_nan = X_test.columns[X_test.isna().any()]
    print("Columns with inf:", list(bad_inf))
    print("Columns with NaN:", list(bad_nan))
    print("Rows with inf:", np.isinf(X_test).any(axis=1).sum())
    print("Rows with NaN:", np.isnan(X_test).any(axis=1).sum())


    # Load trained model
    model = xgb.Booster()
    model.load_model(args.model)
    dtest = xgb.DMatrix(X_test, missing=np.inf)
    y_pred_prob = model.predict(dtest)

    # ROC Curve
    fig, ax = plt.subplots(figsize=(8, 7), dpi=150)
    fpr, tpr, _ = roc_curve(y_test, y_pred_prob)
    roc_auc = auc(fpr, tpr)
    ax.plot(fpr, tpr, label=f'AUC = {roc_auc:.3f}')
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
        if name in feature_names:
            return name.replace("tau_", "")
        if name.startswith("f") and name[1:].isdigit():
            idx = int(name[1:])
            if idx < len(feature_names):
                return feature_names[idx].replace("tau_", "")
        return name
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

if __name__ == "__main__":
    main()
