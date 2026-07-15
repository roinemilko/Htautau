import argparse
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import mplhep as hep
from uproot_data import load_tau_data


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

    # Drop non-numeric columns for correlation
    df_num = df.select_dtypes(include="number")

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
    parser.add_argument("--sig", required=True)
    parser.add_argument("--bg", required=True)
    parser.add_argument("--out_sig", required=True)
    parser.add_argument("--out_bg", required=True)
    parser.add_argument("--num_taus", type=int, default=1, choices=[1, 2])
    parser.add_argument(
        "--cms-label",
        default="Work in Progress",
        choices=["Preliminary", "Simulation", "Work in Progress", "Private Work"],
    )
    parser.add_argument("--variables", nargs='+', default=None, help="List of variables to load")
    args = parser.parse_args()
    
    if args.variables:
        df_sig = load_tau_data(args.sig, label=1, num_taus=args.num_taus, variables=args.variables)
        df_bg  = load_tau_data(args.bg, label=0, num_taus=args.num_taus, variables=args.variables)
    else:
        df_sig = load_tau_data(args.sig, label=1, num_taus=args.num_taus)
        df_bg  = load_tau_data(args.bg, label=0, num_taus=args.num_taus)

    plot_corr_heatmap(df_sig, "", args.out_sig, args.cms_label)
    plot_corr_heatmap(df_bg, "", args.out_bg, args.cms_label)


if __name__ == "__main__":
    main()