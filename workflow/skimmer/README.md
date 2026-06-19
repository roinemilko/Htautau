## Skim/plot workflow 
Designed to easily skim Jets/FatJets/AK15 -jets from raw MC -simulated data and plot their differences. Skims the following features and labels, if available,
| Feature | Label (MC truth) |
| :--- | :--- |
| `[jet]_pt` | `target_mass` |
| `[jet]_eta` | `is_truth_ehad` |
| `[jet]_phi` | `is_truth_muhad` |
| `[jet]_mass` | `is_truth_hadhad` |
| `[jet]_rawFactor` | `matchedHiggsIdx` |
| `[jet]_mass_rawFactorCorrected` | `matchedTau1Idx` |
| `[jet]_msoftdrop` | `matchedTau2Idx` |
| `PFMET_pt` | `matched[jet]Idx` |
| `PFMET_phi` | `nTauMatchedGood[jet]` |
| `PFMET_significance` | `nLastCopyTauFromHiggs` |
| `PFMET_sumEt` | `nHiggsTauMu` |
| `PFMET_sumPtUnclustered` | `nHiggsTauE` |
| `PuppiMET_pt` | `nHiggsTauHad` |
| `PuppiMET_phi` | `dR_[jet]_tau1` |
| `dphi_[jet]_pfmet` | `dR_[jet]_tau2` |
| `dphi_[jet]_puppimet` | `dR_[jet]_H` |
| `[jet]_pt_minus_pfmet_pt` | `dR_tau1_tau2` |
| `[jet]_pt_minus_puppimet_pt` | |
| `pfmet_over_[jet]_pt` | |
| `puppimet_over_[jet]_pt` | |

Missing values are replaced with -999.8f (not robust, fails if critical features missing). 

## Guide
To just skim some data, run 
```
snakemake skim --cores all
```
with optional config flags detailed below. To plot some features, run 
``` 
snakemake plot --cores "all"
```
This will skim all three jets and plot their $m/p_t$ and $\eta$ side by side. The config flags are
| Config Flag | Default Value | Description |
| :--- | :--- | :--- |
| `exclude` | None | exclude jet types |
| `params` | `"X_mass/X_pt, X_eta"` | Variables to plot. Use `X` as a placeholder for jet prefixes. |
| `data-dir` |  | Directory containing the raw NanoAOD `.root` files. |
| `plot-dir` |  | Destination directory path for plots. |
| `require-hadhad` | `"n"` | require truth-level hadhad matching |

For example, running
```
snakemake plot --config exclude=AK4 params="X_mass, X_pt, X_eta" --cores all
```
would first skim AK8 and AK15 jets from MC data and save something like this
<img width="1448" height="456" alt="image" src="https://github.com/user-attachments/assets/5f7b7be2-bae2-456b-9396-eb5685176eac" />
to `FatJet_AK15_mass_pt_eta_plot.png`.
