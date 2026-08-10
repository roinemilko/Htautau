import uproot
import awkward as ak
import pandas as pd
import os
import numpy as np
from tqdm import tqdm
import hashlib

AK8_VARIABLES = [
    "fj_pt", "fj_eta", "fj_phi", "fj_mass", "fj_msoftdrop", "fj_rawFactor", "fj_area",
    "fj_chEmEF", "fj_chHEF", "fj_hfEmEF", "fj_hfHEF", "fj_neEmEF", "fj_neHEF", "fj_muEF",
    "fj_lsf3", "fj_nConstituents", "fj_chMultiplicity", "fj_neMultiplicity",
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
    "fj_Subjet_UParTAK4V1RegPtRawRes", "event"
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

def load_fatjet_data(file_path, label, jet_type="AK8", use_subjets=False, force_rebuild=False, variables=None):
    if variables is None:
        variables = AK8_VARIABLES if jet_type == "AK8" else AK15_VARIABLES

    base_name = os.path.basename(file_path).replace('.root', '')
    parent_dir = os.path.basename(os.path.dirname(file_path))

    config_string = f"{file_path}_{label}_{jet_type}_subjets{use_subjets}_{','.join(variables)}"
    config_hash = hashlib.md5(config_string.encode('utf-8')).hexdigest()[:8]

    cache_file = f"data_cache/{parent_dir}_{base_name}_cache_{jet_type}_{config_hash}.parquet"
    
    if not force_rebuild and os.path.exists(cache_file):
        return pd.read_parquet(cache_file)

    n_subjet_var = "fj_nMatchedSubjets" if jet_type == "AK8" else "ak15_nMatchedSubjets"

    read_vars = list(variables)
    if use_subjets and n_subjet_var not in read_vars:
        read_vars.append(n_subjet_var)

    if not use_subjets:
        variables = [var for var in variables if "_Subjet_" not in var]

    out_vars = []
    for var in variables:
        if "_Subjet_" in var and use_subjets:
            out_vars.extend([f"{var}_1", f"{var}_2"])
        else:
            out_vars.append(var)

    accumulated_data = {var: [] for var in out_vars}

    tree = uproot.open(f"{file_path}:Events")
    total_events = tree.num_entries
    tree.close()

    chunk_size = 250_000 
    total_chunks = (total_events // chunk_size) + 1

    iterator = uproot.iterate(
        f"{file_path}:Events",
        expressions=read_vars, 
        step_size=chunk_size,
        num_workers=6
    )

    for arrays in tqdm(iterator, total=total_chunks, desc=f"Reading {base_name} ({jet_type})", unit="chunk"):
        if use_subjets:
            # Require 2 subjets if subjets used
            mask = arrays[n_subjet_var] == 2
            arrays = arrays[mask]
                
        for var in variables:
            if "_Subjet_" in var and use_subjets:
                accumulated_data[f"{var}_1"].append(np.ravel(ak.to_numpy(arrays[var][:, 0])))
                accumulated_data[f"{var}_2"].append(np.ravel(ak.to_numpy(arrays[var][:, 1])))
            else:
                if var in out_vars:
                    accumulated_data[var].append(np.ravel(ak.to_numpy(arrays[var])))

    print("Merging arrays and building DataFrame...")
    final_dict = {var: np.concatenate(accumulated_data[var]) for var in out_vars}
    df = pd.DataFrame(final_dict)
    df["label"] = label
    
    n_before = len(df)
    df = df.replace([np.inf, -np.inf], np.nan).dropna()
    print(f"Dropped {n_before - len(df)} rows with NaN/inf ({100*(n_before-len(df))/n_before:.2f}%)")

    os.makedirs(os.path.dirname(cache_file), exist_ok=True)
    df.to_parquet(cache_file)
    
    return df

## Cross secions of signals in pb
XSEC_DICT = {
    "TTto4Q": 377.96,
    "TTtoLNu2Q": 365.34,
    "TTto2L2Nu": 88.29,
    "DYto2Tau": 6077.22,
}

LUMI_FB = 137.0

def load_mixed_fatjet_data(paths, label, jet_type="AK8", use_subjets=False, force_rebuild=False, variables=None):

    if variables is not None:
        variables = sorted(variables)


    paths_str = ",".join(sorted(paths))
    vars_str = ",".join(variables) if variables else "default"
    config_string = f"{paths_str}_{label}_{jet_type}_subjets{use_subjets}_{vars_str}"
    config_hash = hashlib.md5(config_string.encode('utf-8')).hexdigest()[:8]
    
    cache_file = f"data_cache/mixed_data_cache_{jet_type}_{config_hash}.parquet"

    if not force_rebuild and os.path.exists(cache_file):
        return pd.read_parquet(cache_file)

    dfs = []
    lumi_pb = LUMI_FB * 1000.0 

    for path in paths:
        bg_match = next((key for key in XSEC_DICT.keys() if key in path), None)        
        if bg_match:
            name = bg_match
            with uproot.open(f"{path}:Events") as raw_tree:
                n_gen = raw_tree.num_entries
                
            weight = (XSEC_DICT[name] * lumi_pb) / n_gen if n_gen > 0 else 1
            print(f"Sample {name}: N_gen={n_gen}, Weight={weight:.6e}")
            
        else:
            name = os.path.basename(path).replace(".root", "")
            weight = 1.0 
            print(f"Sample {name}: Weight={weight:.6e} (Unweighted)")

        df = load_fatjet_data(
            file_path=path,
            label=label,
            jet_type=jet_type,
            use_subjets=use_subjets,
            force_rebuild=force_rebuild,
            variables=variables
        )

        df["weight"] = weight
        df["process"] = name

        dfs.append(df)

    print("\nMerging all samples into mixed DataFrame...")
    df_mixed = pd.concat(dfs, ignore_index=True)
    
    os.makedirs(os.path.dirname(cache_file), exist_ok=True)
    df_mixed.to_parquet(cache_file)

    return df_mixed