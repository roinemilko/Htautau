import argparse
import pandas as pd
import xgboost as xgb
from sklearn.model_selection import train_test_split
from uproot_data import load_tau_data

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--sig", required=True)
    parser.add_argument("--bg", required=True)
    parser.add_argument("--out_model", required=True)
    parser.add_argument("--seed", type=int, default=100)
    parser.add_argument("--num_taus", type=int, default=1, choices=[1, 2], help="Use only leading tau or both")
    parser.add_argument("--variables", nargs='+', default=None, help="List of variables to load")
    args = parser.parse_args()

    if args.variables:
        df_sig = load_tau_data(args.sig, label=1, num_taus=args.num_taus, variables=args.variables)
        df_bg  = load_tau_data(args.bg, label=0, num_taus=args.num_taus, variables=args.variables)
    else:
        df_sig = load_tau_data(args.sig, label=1, num_taus=args.num_taus)
        df_bg  = load_tau_data(args.bg, label=0, num_taus=args.num_taus)

    df_all = pd.concat([df_sig, df_bg], ignore_index=True)

    X = df_all.drop(columns=['label'])
    y = df_all['label']

    X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.5, random_state=args.seed)

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
