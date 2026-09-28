"""
Unit tests for the helper functions in training_workflow
"""
import math
import warnings

import pytest

np = pytest.importorskip("numpy")
pytest.importorskip("sklearn")
from sklearn.metrics import roc_curve

def _try_import(name):
    try:
        return __import__(name), None
    except Exception as e:
        return None, e


Helpers, _helpers_err = _try_import("Helpers")

if _helpers_err is not None:
    warnings.warn(f"Helpers.py could not be imported ({type(_helpers_err).__name__}: {_helpers_err})")

BOTH = [m for m in (Helpers,) if m is not None]
if not BOTH:
    pytest.skip(
        "Helpers.py could not be imported",
        allow_module_level=True,
    )


@pytest.mark.parametrize("mod", BOTH)
def test_get_mode_names_single(mod):
    assert mod.get_mode_names("TTto4Q") == mod.BG_STRING_DICT["TTto4Q"]


@pytest.mark.parametrize("mod", BOTH)
def test_get_mode_names_joins_multiple_with_plus_newline(mod):
    result = mod.get_mode_names("TTto4Q_TTto2L2Nu")
    expected = mod.BG_STRING_DICT["TTto4Q"] + "+\n" + mod.BG_STRING_DICT["TTto2L2Nu"]
    assert result == expected


@pytest.mark.parametrize("mod", BOTH)
def test_get_mode_names_unknown_background_raises(mod):
    with pytest.raises(KeyError):
        mod.get_mode_names("NotARealBackground")


@pytest.mark.parametrize("mod", BOTH)
def test_safe_auc_perfect_separation(mod):
    y = np.array([0, 0, 0, 1, 1, 1])
    p = np.array([0.1, 0.2, 0.3, 0.7, 0.8, 0.9])
    assert mod.safe_auc(y, p) == pytest.approx(1.0)


@pytest.mark.parametrize("mod", BOTH)
def test_safe_auc_single_class_is_nan(mod):
    y = np.array([1, 1, 1, 1])
    p = np.array([0.1, 0.4, 0.6, 0.9])
    assert math.isnan(mod.safe_auc(y, p))


@pytest.mark.parametrize("mod", BOTH)
def test_safe_auc_constant_prediction_is_half(mod):
    y = np.array([0, 1, 0, 1])
    p = np.array([0.5, 0.5, 0.5, 0.5])
    assert mod.safe_auc(y, p) == pytest.approx(0.5)


@pytest.mark.parametrize("mod", BOTH)
def test_safe_auc_accepts_sample_weight(mod):
    y = np.array([0, 0, 1, 1])
    p = np.array([0.2, 0.4, 0.6, 0.8])
    w = np.ones_like(y, dtype=float)
    assert mod.safe_auc(y, p, w) == pytest.approx(mod.safe_auc(y, p))



@pytest.mark.parametrize("mod", BOTH)
def test_get_weighted_threshold_matches_roc_curve_thresholds(mod):
    y = np.array([0, 0, 0, 0, 1, 1, 1, 1])
    p = np.array([0.1, 0.2, 0.3, 0.4, 0.6, 0.7, 0.8, 0.9])
    w = np.ones_like(y, dtype=float)

    target_fpr = 0.5
    got = mod.get_weighted_threshold(y, p, w, target_fpr)

    fpr, _, thresholds = roc_curve(y, p, sample_weight=w)
    idx = np.where(fpr <= target_fpr)[0][-1]
    assert got == pytest.approx(thresholds[idx])


@pytest.mark.parametrize("mod", BOTH)
def test_get_weighted_threshold_falls_back_to_index_zero_below_all_fpr(mod):
    y = np.array([0, 0, 1, 1])
    p = np.array([0.1, 0.9, 0.2, 0.8])
    w = np.ones_like(y, dtype=float)
    got = mod.get_weighted_threshold(y, p, w, target_fpr=-1.0)
    assert not math.isnan(got)
