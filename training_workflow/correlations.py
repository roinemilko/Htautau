import argparse
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import mplhep as hep
from Helpers import *


def plot_corr_heatmap(df, title, out_path, cms_label="Work in progress"):
    hep.style.use("CMS")

    plt.rcParams.update({
        "font.size": 10,
        "axes.titlesize": 11,
        "xtick.major.width": 0.6,
        "ytick.major.width": 0.6,
        "xtick.major.size": 3,
        "ytick.major.size": 3,
    })

    df_num = df.select_dtypes(include="number")
    if "weight" in df_num.columns:
        df_num = df_num.drop(columns=["weight"])

    fig, ax = plt.subplots(figsize=(10, 8), dpi=150)

    corr = df_num.corr()

    sns.heatmap(
        corr,
        ax=ax,
        cmap="RdBu_r",
        vmin=-1,
        vmax=1,
        center=0,
        square=True,
        linewidths=0.2,
        linecolor="white",
        cbar_kws={"shrink": 0.8, "label": "Correlation"},
        xticklabels=True,
        yticklabels=True,
    )

    ax.set_title(title, pad=20)
    ax.tick_params(axis="both", labelsize=8)
    plt.setp(ax.get_xticklabels(), rotation=45, ha="right")

    # CMS label
    hep.cms.label(
        label=cms_label,
        data=False,
        rlabel=r"$H \to \tau\tau$ (125 GeV)",
        ax=ax,
        loc=0
    )
    fig.tight_layout()
    fig.savefig(out_path, bbox_inches="tight")
    plt.close(fig)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--sig", required=True, help="Comma-separated list of signal root files")
    parser.add_argument("--bg", required=True, help="Comma-separated list of background root files")
    parser.add_argument("--out_sig", required=True)
    parser.add_argument("--out_bg", required=True)
    parser.add_argument("--num_taus", type=int, default=1, choices=[1, 2])
    parser.add_argument("--cms-label", dest="cms_label", default="Work in Progress")
    parser.add_argument("--variables", nargs='+', default=None, help="List of variables to load")
    parser.add_argument("--mode", default="tau", choices=["Tau", "AK8", "AK15"], help="Object type to process")
    parser.add_argument("--use_subjets", action="store_true", help="Require 2 subjets and load subjet features (AK8/AK15 only)")
    parser.add_argument("--use_weights", action="store_true", help="Calculate and use event weights")
    args = parser.parse_args()
    
    df_sig, df_bg = load_data(
        sig_path=args.sig,
        bg_path=args.bg,
        mode=args.mode,
        args=args,
        sig_vars=args.variables,
        bg_vars=args.variables
    )

    if not df_sig.empty:
        plot_corr_heatmap(df_sig, "", args.out_sig, args.cms_label)
    if not df_bg.empty:
        plot_corr_heatmap(df_bg, "", args.out_bg, args.cms_label)


if __name__ == "__main__":
    main()