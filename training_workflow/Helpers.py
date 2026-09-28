from uproot_data import *
from uproot_fat import *
import uproot
from sklearn.metrics import roc_curve, auc, roc_auc_score
    
BG_STRING_DICT = {
    "TTto4Q": r"$tt \to qqqq$",
    "TTto2L2Nu": r"$tt \to \ell\ell\nu\nu$",
    "TTtoLNu2Q": r"$tt \to \ell \nu qq$",
    "DYto2Tau": r"DY \to \tau\tau"
}

LUMI_FB = 137.0

def parse_xsec_arg(raw):
    """Parse a 'Name=xsec[pb],Name2=xsec2[pb]' CLI string into {name: float}."""
    if not raw:
        return {}
    result = {}
    for pair in raw.split(","):
        pair = pair.strip()
        if not pair:
            continue
        key, _, value = pair.partition("=")
        result[key.strip()] = float(value)
    return result

def load_data(sig_path, bg_path, mode, args, sig_vars=None, bg_vars=None):
    """Caller for data loaders"""

    sig_paths = sig_path.split(',')
    bg_paths = bg_path.split(',')
    bg_xsec = getattr(args, "bg_xsec", None) or {}

    if mode == "Tau":
        df_sig = load_mixed_tau_data(
            sig_paths, label=1, num_taus=args.num_taus, variables=sig_vars, apply_weights=args.use_weights
        )
        df_bg = load_mixed_tau_data(
            bg_paths, label=0, num_taus=args.num_taus, variables=bg_vars, apply_weights=args.use_weights, xsec_dict=bg_xsec
        )
    else:
        df_sig = load_mixed_fatjet_data(
            sig_paths, label=1, jet_type=mode, use_subjets=args.use_subjets, variables=sig_vars, apply_weights=args.use_weights
        )
        df_bg = load_mixed_fatjet_data(
            paths=bg_paths, label=0, jet_type=mode, use_subjets=args.use_subjets, variables=bg_vars, apply_weights=args.use_weights, xsec_dict=bg_xsec
        )

    return df_sig, df_bg

def get_mode_names(str):
    modes_list = str.split("_")
    mode_names_list = [BG_STRING_DICT[i] for i in modes_list]
    return "+\n".join(mode_names_list)

def get_weighted_threshold(y_true, y_pred, weights, target_fpr):
    """Returns the cut when inference is done with weighted samples"""
    fpr, tpr, thresholds = roc_curve(y_true, y_pred, sample_weight=weights)
    idx = np.where(fpr <= target_fpr)[0][-1] if len(np.where(fpr <= target_fpr)[0]) > 0 else 0
    return thresholds[idx]

def safe_auc(y, p, w=None):
    if len(np.unique(y)) < 2:
        return np.nan
    if len(np.unique(p)) == 1:
        return 0.5
    return roc_auc_score(y, p, sample_weight=w)