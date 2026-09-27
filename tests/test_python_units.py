"""
Unit tests for the helper functions in training_workflow
"""
import math
import warnings

import pytest

np = pytest.importorskip("numpy")
pytest.importorskip("sklearn")
from sklearn.metrics import roc_curve  # noqa: E402

def _try_import(name):
    try:
        return __import__(name), None
    except Exception as e:  # noqa: BLE001 - intentionally broad, see below
        return None, e


# plot_helpers.py only needs numpy/sklearn, but Helpers.py pulls in
# uproot_data.py/uproot_fat.py (dask, uproot, ...) - a real environment
# problem there (e.g. dask.dataframe's legacy API being removed) must not
# take plot_helpers.py's tests down with it. importorskip only catches
# ImportError, not arbitrary exceptions raised mid-import, so this imports
# each module independently and turns any failure into a clear, isolated skip.
Helpers, _helpers_err = _try_import("Helpers")
plot_helpers, _plot_helpers_err = _try_import("plot_helpers")

for _mod_name, _err in (("Helpers", _helpers_err), ("plot_helpers", _plot_helpers_err)):
    if _err is not None:
        warnings.warn(
            f"{_mod_name}.py could not be imported ({type(_err).__name__}: {_err}) - "
            "its tests are skipped. This is an environment/code issue in that module "
            "(see tests/README.md), not a problem with these tests."
        )

BOTH = [m for m in (Helpers, plot_helpers) if m is not None]
if not BOTH:
    pytest.skip(
        "neither Helpers.py nor plot_helpers.py could be imported - see warnings above",
        allow_module_level=True,
    )

needs_both = pytest.mark.skipif(
    Helpers is None or plot_helpers is None,
    reason="needs both Helpers.py and plot_helpers.py importable",
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


@needs_both
def test_get_mode_names_identical_between_helpers_and_plot_helpers():
    for bg_key in ("TTto4Q", "TTto2L2Nu", "TTto4Q_TTto2L2Nu"):
        assert Helpers.get_mode_names(bg_key) == plot_helpers.get_mode_names(bg_key)


@pytest.mark.xfail(reason=(
    "known divergence: Helpers.BG_STRING_DICT['DYto2Tau'] is "
    r"'DY \to \tau\tau' (missing the $...$ wrapping the other three entries "
    "have) while plot_helpers.BG_STRING_DICT['DYto2Tau'] is plain 'DY' - "
    "same key, two different label conventions depending which module a "
    "script happens to import from."
), strict=False)
@needs_both
def test_bg_string_dict_dyto2tau_label_matches_across_modules():
    assert Helpers.BG_STRING_DICT["DYto2Tau"] == plot_helpers.BG_STRING_DICT["DYto2Tau"]


@pytest.mark.xfail(reason=(
    "known divergence: Helpers.XSEC_DICT['DYto2Tau'] = 2219 vs "
    "plot_helpers.XSEC_DICT['DYto2Tau'] = 2125 (uproot_fat.py has its own "
    "third copy, also 2125) - a ~4.4% cross-section difference depending "
    "which module a script imports XSEC_DICT from."
), strict=False)
@needs_both
def test_xsec_dict_dyto2tau_matches_across_modules():
    assert Helpers.XSEC_DICT["DYto2Tau"] == plot_helpers.XSEC_DICT["DYto2Tau"]



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
