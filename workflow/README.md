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
| `RawPFMET_pt` | `matched[jet]Idx` |
| `RawPFMET_phi` | `nTauMatchedGood[jet]` |
| `PuppiMET_significance` | `nLastCopyTauFromHiggs` |
| `PuppiMET_sumEt` | `nHiggsTauMu` |
| `PuppiMET_sumPtUnclustered` | `nHiggsTauE` |
| `PuppiMET_pt` | `nHiggsTauHad` |
| `PuppiMET_phi` | 
| `dphi_[jet]_pfmet` |
| `dphi_[jet]_puppimet` |
| `[jet]_pt_minus_pfmet_pt` |
| `[jet]_pt_minus_puppimet_pt` | 
| `pfmet_over_[jet]_pt` | 
| `puppimet_over_[jet]_pt` | 
| `dR_[jet]_tau1` |
| `dR_[jet]_tau2` |
| `dR_[jet]_H` |
| `dR_tau1_tau2` |

Missing values are replaced with -999.8f (not robust, fails if critical features missing). 

## Guide
To just skim some data, run 
```
./run.sh skim
```
with optional config flags detailed below. To plot some features, run 
``` 
./run.sh plot
```
This will skim all three jets and plot their $m/p_t$ and $\eta$ side by side. The config flags are
| Config Flag | Default Value | Description |
| :--- | :--- | :--- |
| `exclude` | None | exclude jet types |
| `params` | `"X_mass/X_pt, X_eta"` | Variables to plot. Use `X` as a placeholder for jet prefixes. |
| 'normalize' | 'true' | Whether to normalise the histogram to [0,1] (to effectively compare distributions with uneven data)
| `data-dir` |  | Directory containing the raw NanoAOD `.root` files. |
| `plot-dir` |  | Destination directory path for plots. |
| `require-hadhad` | `"false"` | require jets matched to hadronic decay only |

For example, running
```
./run.sh plot --config params="X_mass/X_pt, abs(X_eta), abs(X_phi)"  plot-dir="/your/dir"
```
would first skim AK8 and AK15 jets from MC data and save something like this
<img width="985" height="462" alt="image" src="https://github.com/user-attachments/assets/41ab01c6-50e9-48f8-8bd6-29c56a8f75ab" />

to `Jet_FatJet_AK15_massoverpt_abs(eta)_plot.png` in `/your/dir`.
