import uproot
import awkward as ak
import pandas as pd
import os
import numpy as np
from tqdm import tqdm
import hashlib

def load_tau_data(file_path, label, num_taus=1, force_rebuild = False,
    variables = None
):

    if variables == None:
        variables = [
            "tau_pt", "tau_eta", "tau_phi", "tau_mass", "tau_charge",
            "tau_dxy", "tau_dz", "tau_ipLengthSig",
            "tau_chargedIso", "tau_neutralIso", "tau_rawIso", "tau_rawIsodR03", "tau_puCorr",
            "tau_rawDeepTauVSjet", "tau_rawDeepTauVSe", "tau_rawDeepTauVSmu",
            "tau_decayModePNet", "tau_probDM0PNet", "tau_probDM1PNet", "tau_probDM2PNet",
            "tau_probDM10PNet", "tau_probDM11PNet", "tau_ptCorrPNet", "tau_qConfPNet",
            "tau_rawPNetVSe", "tau_rawPNetVSjet", "tau_rawPNetVSmu",
            "tau_decayModeUParT", "tau_probDM0UParT", "tau_probDM1UParT", "tau_probDM2UParT",
            "tau_probDM10UParT", "tau_probDM11UParT", "tau_ptCorrUParT", "tau_qConfUParT",
            "tau_rawUParTVSe", "tau_rawUParTVSjet", "tau_rawUParTVSmu", "event"
        ]
    if "event" not in variables:
        variables.append("event")

    variables = sorted(variables)
    base_name = os.path.basename(file_path).replace('.root', '')
    parent_dir = os.path.basename(os.path.dirname(file_path))
    config_string = f"{file_path}_{label}_{num_taus}_{','.join(variables)}"
    config_hash = hashlib.md5(config_string.encode('utf-8')).hexdigest()[:8]
    cache_file = f"data_cache/{parent_dir}_{base_name}_cache_{num_taus}_{config_hash}.parquet"



    if not force_rebuild and os.path.exists(cache_file):
        return pd.read_parquet(cache_file)

    tree = uproot.open(f"{file_path}:Events")
    total_events = tree.num_entries
    tree.close()
  
    n_cores = 6    
    chunk_size = 250_000 
    total_chunks = (total_events // chunk_size) + 1
    

    if num_taus == 1:
        out_vars = variables
    elif num_taus == 2:
        out_vars = []
        for var in variables:
            if var == "event":
                out_vars.append("event")
            elif var == "genH_pt":
                out_vars.append("genH_pt")
            else:
                out_vars.extend([f"{var}_1", f"{var}_2"])
    else:
        raise ValueError("num_taus has to be 1 or 2!")

    accumulated_data = {var: [] for var in out_vars}
    
    iterator = uproot.iterate(
        f"{file_path}:Events",
        expressions=variables, 
        step_size=chunk_size,
        num_workers=n_cores
    )

    
    for arrays in tqdm(iterator, total=total_chunks, desc=f"Reading {base_name}", unit="chunk"):
        for var in variables:
            if var == "event":
                accumulated_data["event"].append(np.ravel(ak.to_numpy(arrays[var])))
                continue
            if var == "genH_pt":
                accumulated_data["genH_pt"].append(np.ravel(ak.to_numpy(arrays[var])))
                continue
            if num_taus == 1:
                accumulated_data[var].append(np.ravel(ak.to_numpy(arrays[var][:, 0])))
            elif num_taus == 2:
                accumulated_data[f"{var}_1"].append(np.ravel(ak.to_numpy(arrays[var][:, 0])))
                accumulated_data[f"{var}_2"].append(np.ravel(ak.to_numpy(arrays[var][:, 1])))
            else:
                raise ValueError(f"num_taus has to be 1 or 2!")
        



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

