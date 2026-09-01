from sklearn.metrics import roc_curve, auc, roc_auc_score
import numpy as np

BG_STRING_DICT = {
    "TTto4Q": r"$tt \to qqqq$",
    "TTto2L2Nu": r"$tt \to \ell\ell\nu\nu$",
    "TTtoLNu2Q": r"$tt \to \ell \nu qq$",
    "DYto2Tau": "DY"
}

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

XSEC_DICT = {
    "TTto4Q": 419.7,         # NNLO+NNLL inclusive ttbar * BR(W->qq)^2
    "TTtoLNu2Q": 405.7,      # NNLO+NNLL inclusive ttbar * 2*BR(W->qq)*BR(W->lv)
    "TTto2L2Nu": 98,       # NNLO+NNLL inclusive ttbar * BR(W->lv)^2
    "DYto2Tau": 2125,      # NNLO DYJetsToLL M > 50 GeV
    
    
    "VBF": 0.02939443, 
    "ggF": 51.72
}


LUMI_FB = 137.0
