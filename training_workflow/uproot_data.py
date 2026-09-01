import uproot
import dask
dask.config.set({'dataframe.query-planning': False})
import dask_awkward as dak
import dask.dataframe as dd
import os
import numpy as np

def load_tau_data(file_path, label, event_offset, weight, process_name, num_taus=1, variables=None):

    if variables is None:
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
            "tau_rawUParTVSe", "tau_rawUParTVSjet", "tau_rawUParTVSmu"
        ]

    variables = list(variables)
    if "event" not in variables:
        variables.append("event")

    read_vars = sorted(variables)
    
    events = uproot.dask(f"{file_path}:Events", filter_name=read_vars, step_size=50_000)

    out_dict = {}

    for var in read_vars:
        if var in ["event", "genH_pt", "weight", "process", "label"]:
            out_dict[var] = events[var]
        elif num_taus == 1:
            out_dict[var] = events[var][:, 0]
        elif num_taus == 2:
            out_dict[f"{var}_1"] = events[var][:, 0]
            out_dict[f"{var}_2"] = events[var][:, 1]
        else:
            raise ValueError("num_taus has to be 1 or 2!")

    ak_record = dak.zip(out_dict)
    
    import dask
    import awkward as ak
    import dask.dataframe as dd

    def to_pandas_chunk(arr):
        """Converts a concrete awkward array chunk to a pandas chunk"""
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

def load_mixed_tau_data(paths, label, num_taus=1, variables=None, apply_weights=True):
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
                print(f"Sample {name}: Weight={weight:.6e} (Unweighted)")

        offset = PROCESS_OFFSETS.get(name, PROCESS_OFFSETS.get("jets", 0))

        df_lazy = load_tau_data(
            file_path=path, 
            label=label,
            event_offset=offset,
            weight=weight, 
            process_name=name,
            num_taus=num_taus, 
            variables=variables
        )
        
        all_dfs.append(df_lazy)
 
    return dd.concat(all_dfs) if all_dfs else None