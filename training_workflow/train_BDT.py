import os
import hashlib
import argparse
import uproot
import pandas as pd
import xgboost as xgb
from sklearn.model_selection import train_test_split
from uproot_data import load_tau_data
from uproot_fat import load_fatjet_data

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--sig", required=True)
    parser.add_argument("--bg", nargs='+', required=True, help="One or more background ROOT files")
    parser.add_argument("--out_model", required=True)
    parser.add_argument("--seed", type=int, default=100)
    parser.add_argument("--num_taus", type=int, default=1, choices=[1, 2], help="Use only leading tau or both")
    parser.add_argument("--variables", nargs='+', default=None, help="List of variables to load")
    parser.add_argument("--mode", default="tau", choices=["Tau", "AK8", "AK15"], help="Object type to process")
    parser.add_argument("--use_subjets", action="store_true", help="Require 2 subjets and load subjet features (AK8/AK15 only)")
    args = parser.parse_args()

    load_vars = list(set(args.variables + ["event"])) if args.variables else None

    if args.mode == "Tau":


        if args.variables:
            df_sig = load_tau_data(args.sig, label=1, num_taus=args.num_taus, variables=args.variables)

            df_bg  = load_tau_data(args.bg[0], label=0, num_taus=args.num_taus, variables=args.variables)
        else:
            df_sig = load_tau_data(args.sig, label=1, num_taus=args.num_taus)
            df_bg  = load_tau_data(args.bg[0]   , label=0, num_taus=args.num_taus)

    if args.mode == "AK8" or args.mode == "AK15":
        if args.variables:
            df_sig = load_fatjet_data(args.sig, label=1, jet_type=args.mode, variables=args.variables, use_subjets=args.use_subjets)

            if len(args.bg) > 1:
                df_bg = load_mixed_fatjet_data(
                    paths=args.bg, 
                    label=0, 
                    jet_type=args.mode, 
                    use_subjets=args.use_subjets, 
                    variables=load_vars
                )
            else:
                df_bg = load_fatjet_data(
                file_path=args.bg[0], 
                label=0, 
                jet_type=args.mode, 
                use_subjets=args.use_subjets, 
                variables=load_vars
            )
        # TODO: implement this (check if this if statement even needed anymore)
        else:
            df_sig = load_fatjet_data(args.sig, label=1, jet_type=args.mode, use_subjets=args.use_subjets)
            df_bg  = load_fatjet_data(args.bg, label=0, jet_type=args.mode, use_subjets=args.use_subjets)

    df_all = pd.concat([df_sig, df_bg], ignore_index=True)


    df_train = df_all[df_all["event"] % 2 == 0].copy()
    cols_to_drop = ["label", "event", "genH_pt"]
    X_train = df_train.drop(columns=[c for c in cols_to_drop if c in df_train.columns])
    y_train = df_train['label']

    dtrain = xgb.DMatrix(X_train, label=y_train)

    params = {
        'objective': 'binary:logistic',
        'eval_metric': 'auc',
        'max_depth': 3,
        'eta': 0.1,
        'tree_method': 'hist'
    }

    print("Training XGBoost...")
    model = xgb.train(params, dtrain, num_boost_round=400)
    
    # Save the model
    model.save_model(args.out_model)

if __name__ == "__main__":
    main()
