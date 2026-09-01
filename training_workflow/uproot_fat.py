import uproot
import dask
dask.config.set({'dataframe.query-planning': False})
import dask_awkward as dak
import dask.dataframe as dd
import os
import numpy as np

AK8_VARIABLES = [
    "fj_pt", "fj_eta", "fj_phi", "fj_mass", "fj_msoftdrop", "fj_rawFactor", "fj_area",
    "fj_chEmEF", "fj_chHEF", "fj_hfEmEF", "fj_hfHEF", "fj_neEmEF", "fj_neHEF", "fj_muEF", "fj_lsf3",
    "fj_tau1", "fj_tau2", "fj_tau3", "fj_tau4", "fj_globalParT3_QCD", "fj_globalParT3_TopbWev",
    "fj_globalParT3_TopbWmv", "fj_globalParT3_TopbWq", "fj_globalParT3_TopbWqq", 
    "fj_globalParT3_TopbWtauhv", "fj_globalParT3_WvsQCD", "fj_globalParT3_XWW3q", 
    "fj_globalParT3_XWW4q", "fj_globalParT3_XWWqqev", "fj_globalParT3_XWWqqmv", 
    "fj_globalParT3_Xbb", "fj_globalParT3_Xcc", "fj_globalParT3_Xcs", "fj_globalParT3_Xqq", 
    "fj_globalParT3_Xtauhtaue", "fj_globalParT3_Xtauhtauh", "fj_globalParT3_Xtauhtaum", 
    "fj_globalParT3_massCorrGeneric", "fj_globalParT3_massCorrX2p", 
    "fj_globalParT3_withMassTopvsQCD", "fj_globalParT3_withMassWvsQCD", 
    "fj_globalParT3_withMassZvsQCD", "fj_Subjet_pt", "fj_Subjet_eta", 
    "fj_Subjet_phi", "fj_Subjet_mass", "fj_Subjet_area", "fj_Subjet_rawFactor", 
    "fj_Subjet_pt_rawFactorCorrected", "fj_Subjet_tau1", "fj_Subjet_tau2", "fj_Subjet_tau3", 
    "fj_Subjet_tau4", "fj_Subjet_n2b1", "fj_Subjet_n3b1", "fj_Subjet_btagDeepFlavB", 
    "fj_Subjet_btagUParTAK4B", "fj_Subjet_UParTAK4RegPtRawCorr", 
    "fj_Subjet_UParTAK4RegPtRawCorrNeutrino", "fj_Subjet_UParTAK4RegPtRawRes", 
    "fj_Subjet_UParTAK4V1RegPtRawCorr", "fj_Subjet_UParTAK4V1RegPtRawCorrNeutrino", 
    "fj_Subjet_UParTAK4V1RegPtRawRes", "event", "fj_nConstituents", "fj_chMultiplicity", "fj_neMultiplicity"
]

AK15_VARIABLES = [
    "ak15_pt", "ak15_eta", "ak15_phi", "ak15_mass", "ak15_msoftdrop", "ak15_rawFactor", 
    "ak15_area", "ak15_tau1", "ak15_tau2", "ak15_tau3", 
    "ak15_ParTv3_massCorrGeneric", "ak15_ParTv3_massCorrResonance", "ak15_ParTv3_massCorrX2p", 
    "ak15_ParTv3_probQCD", "ak15_ParTv3_probTopbWev", "ak15_ParTv3_probTopbWmv", 
    "ak15_ParTv3_probTopbWq", "ak15_ParTv3_probTopbWqq", "ak15_ParTv3_probTopbWtauhv", 
    "ak15_ParTv3_probXWW3q", "ak15_ParTv3_probXWW4q", "ak15_ParTv3_probXWWqqev", 
    "ak15_ParTv3_probXWWqqmv", "ak15_ParTv3_probXbb", "ak15_ParTv3_probXcc", 
    "ak15_ParTv3_probXcs", "ak15_ParTv3_probXqq", "ak15_ParTv3_probXtauhtaue", 
    "ak15_ParTv3_probXtauhtauh", "ak15_ParTv3_probXtauhtaum", "ak15_ParticleNetMD_mass", 
    "ak15_ParticleNetMD_probQCDb", "ak15_ParticleNetMD_probQCDbb", 
    "ak15_ParticleNetMD_probQCDc", "ak15_ParticleNetMD_probQCDcc", 
    "ak15_ParticleNetMD_probQCDothers", "ak15_ParticleNetMD_probXbb", 
    "ak15_ParticleNetMD_probXcc", "ak15_ParticleNetMD_probXqq", 
    "ak15_Subjet_pt", "ak15_Subjet_eta", "ak15_Subjet_phi", "ak15_Subjet_mass", 
    "ak15_Subjet_rawFactor", "ak15_Subjet_pt_rawFactorCorrected", "ak15_Subjet_area", "event"
]

def load_fatjet_data(file_path, label, event_offset, 
                     weight, process_name, jet_type="AK8", use_subjets=False, variables=None):

    if variables is None:
        variables = AK8_VARIABLES if jet_type == "AK8" else AK15_VARIABLES

    n_subjet_var = "fj_nMatchedSubjets" if jet_type == "AK8" else "ak15_nMatchedSubjets"

    variables = list(variables)
    if "event" not in variables:
        variables.append("event")
    
    if use_subjets and n_subjet_var not in variables:
        variables.append(n_subjet_var)

    if not use_subjets:
        variables = [var for var in variables if "_Subjet_" not in var]

    variables = sorted(list(set(variables)))

    events = uproot.dask(f"{file_path}:Events", filter_name=variables, step_size=50_000)

    if use_subjets:
        events = events[events[n_subjet_var] == 2]

    out_dict = {}
    

    if not use_subjets:
        variables = [v for v in variables if "_Subjet_" not in v]

    for var in variables:
        if "_Subjet_" in var and use_subjets:
            out_dict[f"{var}_1"] = events[var][:, 0]
            out_dict[f"{var}_2"] = events[var][:, 1]
        else:
            out_dict[var] = events[var]

    ak_record = dak.zip(out_dict)

    import dask
    import awkward as ak
    import dask.dataframe as dd

    def to_pandas_chunk(arr):
        return ak.to_dataframe(arr).reset_index(drop=True)

    delayed_pd_chunks = [dask.delayed(to_pandas_chunk)(chunk) for chunk in ak_record.to_delayed()]
    df = dd.from_delayed(delayed_pd_chunks)

    df["label"] = label
    df["weight"] = weight
    df["process"] = process_name

    float_cols = [c for c in df.columns if df.dtypes[c] == "float64"]
    if float_cols:
        df[float_cols] = df[float_cols].astype(np.float32)

    df = df.replace([np.inf, -np.inf], np.nan).dropna()
    df["event"] = df["event"].astype(np.int64) + event_offset
    return df

XSEC_DICT = {
    "TTto4Q": 419.7,         # NNLO+NNLL inclusive ttbar * BR(W->qq)^2
    "TTtoLNu2Q": 405.7,      # NNLO+NNLL inclusive ttbar * 2*BR(W->qq)*BR(W->lv)
    "TTto2L2Nu": 98,       # NNLO+NNLL inclusive ttbar * BR(W->lv)^2
    "DYto2Tau": 2125,      # NNLO DYJetsToLL M > 50 GeV
    
    
    "VBF": 0.02939443, 
    "ggF": 51.72
}

LUMI_FB = 137.0

PROCESS_OFFSETS = {
    "jets": 0,
    "TTto4Q": 100_000_000,
    "TTtoLNu2Q": 200_000_000,
    "TTto2L2Nu": 300_000_000,
    "DYto2Tau": 400_000_000,
}

def load_mixed_fatjet_data(paths, label, jet_type="AK8", use_subjets=False, variables=None, apply_weights=True):

    if variables is not None:
        variables = sorted(variables)

    lumi_pb = LUMI_FB * 1000.0 
    all_dfs = []
    

    for path in paths:
        name = os.path.basename(path).replace(".root", "")
        weight = 1.0

        if apply_weights:
            bg_match = next((key for key in XSEC_DICT.keys() if key in path), None)        
            if bg_match:
                name = bg_match
                with uproot.open(f"{path}:Events") as raw_tree:
                    n_gen = raw_tree["NRawEvents"].array(library="np", entry_stop=1)[0]
                weight = (XSEC_DICT[name] * lumi_pb) / n_gen if n_gen > 0 else 1.0
                print(f"Sample {name}: N_gen={n_gen}, Weight={weight:.6e}")
            else:
                print(f"Sample {name}: Weight={weight:.6e}")

        offset = PROCESS_OFFSETS.get(name, PROCESS_OFFSETS.get("jets", 0))

        df_lazy = load_fatjet_data(
            file_path=path, 
            label=label,
            event_offset=offset,
            weight=weight, 
            process_name=name, 
            jet_type=jet_type, 
            use_subjets=use_subjets, 
            variables=variables
        )
        
        all_dfs.append(df_lazy)

    return dd.concat(all_dfs) if all_dfs else None