import os
import argparse
import numpy as np
import dask
import dask.dataframe as dd
from dask.distributed import Client
import xgboost as xgb
import xgboost.dask as dxgb

from Helpers import *

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--sig", required=True)
    parser.add_argument("--bg", required=True, help="One or more background ROOT files")
    parser.add_argument("--out_model", required=True)
    parser.add_argument("--num_taus", type=int, default=1, choices=[1, 2], help="Use only leading tau or both")
    parser.add_argument("--variables", nargs='+', default=None, help="List of variables to load")
    parser.add_argument("--mode", default="tau", choices=["Tau", "AK8", "AK15"], help="Object type to process")
    parser.add_argument("--use_subjets", action="store_true", help="Require 2 subjets and load subjet features")
    parser.add_argument("--use_weights", action="store_true", help="Apply cross section weighting")
    parser.add_argument("--n_workers", type=int, default=4)
    args = parser.parse_args()


    dask.config.set({
        'distributed.worker.memory.target': 0.60, 
        'distributed.worker.memory.spill': 0.70,
        'distributed.worker.memory.pause': 0.85, 
    })
    client = Client(n_workers=int(args.n_workers), threads_per_worker=4, memory_limit='5GB')
    print(f"Dask client: {client.dashboard_link}")


    load_vars = list(set(args.variables + ["event"])) if args.variables else None

    sig_paths = args.sig.split(',')
    bg_paths = args.bg.split(',')

    df_sig, df_bg = load_data(
        sig_path=args.sig, 
        bg_path=args.bg, 
        mode=args.mode, 
        args=args, 
        sig_vars=load_vars, 
        bg_vars=load_vars
    )

    df_all = dd.concat([df_sig, df_bg])


    df_train = df_all[df_all["event"] % 2 == 0]


    sum_w_sig_lazy = df_train[df_train["label"] == 1]["weight"].sum()
    sum_w_bg_lazy = df_train[df_train["label"] == 0]["weight"].sum()
    sum_w_sig, sum_w_bg = dask.compute(sum_w_sig_lazy, sum_w_bg_lazy)

    if sum_w_sig > 0 and sum_w_bg > 0:
        scale_factor = sum_w_bg / sum_w_sig
        df_train["weight"] = df_train["weight"].where(df_train["label"] == 0, df_train["weight"] * scale_factor)

    cols_to_drop = ["label", "event", "genH_pt", "weight", "process"]
    X_train = df_train.drop(columns=[c for c in cols_to_drop if c in df_train.columns])
    y_train = df_train["label"]
    w_train = df_train["weight"]


    dtrain = dxgb.DaskDMatrix(client, X_train, y_train, weight=w_train)

    params = {
        'objective': 'binary:logistic',
        'eval_metric': 'auc',
        'max_depth': 3,
        'eta': 0.1,
        'tree_method': 'hist'
    }

    print("Training XGBoost...")
    output = dxgb.train(client, params, dtrain, num_boost_round=400)
    model = output['booster']
    model.save_model(args.out_model)
    print(f"Saved model to {args.out_model}")

    # Clean up
    client.close()

if __name__ == "__main__":
    main()
