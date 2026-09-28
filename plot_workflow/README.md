## Analysis / plotting workflow

Semi-automatic analysis on top of `data_workflow`'s skims.

## Quick start

```bash
./run.sh all --cores N
```
or run a spesific rule by replacing 'all' w rule name.
## Rules

| Rule | Analysis |
|---|---|
| `jet_plots` | Distributions of jet properties from `jet_params`, signal vs one background at a time |
| `subjet_plots` | Same, for AK8/AK15 subjet properties from `subjet_params` |
| `jet_effs` | Natching efficiency vs Higgs pT, pileup, tau asymmetry, energy response, dR(tau,jet) |
| `subjet_effs` | Subjet matching efficiency vs genH pT, at loose (>=1 subjet) and tight (>=2 subjets)  |
| `pt_effs` | Standalone matching eff. plots against Higgs pt |
| `eff_diffs` |  eff.(inclusive - hadhad) vs genH pT, with thesis model overlay |
| `effs_channel` | Decay-channel breakdown of efficiency/response + decay-product energy fractions |
| `clustering_kinematics` | Clustering kinematics sanity checks (legacy stuff not interesting) |


Note the `exclude`d dataset/jet-type combination has to actually exist under
`data_workflow/jets/<dataset>/`.

## Result map
```
plots/<dataset>/
  distributions/   <- jet_plots, subjet_plots
  efficiencies/    <- jet_effs, subjet_effs, pt_effs, eff_diffs, effs_channel
  sanity_checks/   <- legacy clustering kinematics sanity checks
```
