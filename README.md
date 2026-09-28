# Htautau

General-use repo for my 2026 CERN summer project.

## Abstract

At the Large Hadron Collider, proton-proton collisions of center-of-mass energies up
to 13.6TeV are measured and the underlying processes reconstructed as a way to test
predictions of the Standard Model (SM) and to probe new physics beyond it. The
studies of the Higgs boson are currently central to increasing the understanding of the
SM. Collision events in which a Higgs boson is created and subsequently decays into
two tau leptons are particularly interesting. They open a window for measuring the
various Higgs couplings, and consequently verifying their agreement with the SM.
However, the efficiency of reconstructing these events from a vast background remains
a bottleneck in the accuracy of measurements.

This thesis is a study on the feasibility of reconstructing 𝐻 → 𝜏+𝜏−-events in
the Compact Muon Solenoid (CMS)-experiment by using jets of an exceptionally
large radius parameter. These jets have potential to replace the traditional smaller
radius jets as a more efficient and reliable alternatives in boosted collisions. In such
highly energetic collision events, the created Higgs boson has a large momentum
perpendicular to the beamline, resulting in its decay products to be highly collimated
and thus difficult to detect. This thesis presents analyses of the reconstruction and
identification efficiencies, as well as approximations for the efficiencies of the full
reconstruction process. These analyses are conducted with Monte Carlo-simulated
samples of Higgs collision events decaying to two tau leptons, and of central particle
backgrounds.

The overall efficiency of the 𝐻 → 𝜏+𝜏− reconstruction process with anti-𝑘𝑇 jets of
radius parameter 𝑅 = 1.5 (AK15 jets) is found to be superior to traditional choises
in background rejections up to 3 · 105 and 4 · 103 against hadronic and leptonic
backgrounds respectively. The difference is significant, with an increase of up to 20%
in efficiency against hadronic 𝑡¯𝑡-background. These results suggest that AK15 jets
are a viable option for future CMS analyses.

## Quick start

```bash
git clone git@github.com:roinemilko/Htautau.git
cd Htautau
voms-proxy-init --voms cms 
./reproduce.sh --cores 20
```

Useful flags (`./reproduce.sh --help` for the full list):

| Flag | Effect |
|---|---|
| `--stages data,plots,training,publish` | run a subset of modules (default: all) |
| `--dry-run` | forward `-n` to every snakemake call and skip publishing |
| `-- <extra args>` | forwarded verbatim to every snakemake call |

If it falls apart you can try to find the root cause by running
```bash
./diagnose.sh              
```

```
RESULTS/
  <dataset>/
    distributions/
    efficiencies/         # all reco. efficiency related plots
    sanity_checks/        # the legacy clustering kinmatics / sanity checks
    training/              # BDTs and all tagging efficiency results for this dataset
                            # (only present for datasets training_workflow ran on, e.g. MADGRAPH)
  config.snapshot.yaml    # copy of config.yaml from runtime
  MANIFEST.txt             # result timestamp for safety, i.e. results won't be overwritten if this not present
```

## Repo layout

```
data_workflow/          # skim & matching
plot_workflow/          # distribution plots, efficiencies, sanity checks
training_workflow/      # BDT training, inference, comparison
config.yaml             # edit this to customize
reproduce.sh            # reproduce all of the analysis, then publish to RESULTS/
publish_results.sh      # (re)builds RESULTS/ from whatever's already been produced
tests/                  # unit tests
diagnose.sh             # script to help diagnose issues
RESULTS/
```

Each of the three workflow directories is also self-contained, (`run.sh` in each dir).


## config.yaml reference

Edit for customized runs.

### Datasets

The idea here is that you can set local or DAS paths to datasets in `_dirs` dictionaries and give shorthand names for them as keys for convinience. Choose a subset of these to use with the `data` and `bg-channels` flags. 

| Key | Default | Meaning |
|---|---|---|
| `data` | `"MADGRAPH, POWHEG"` | comma-separated signals  |
| `channel` | `"both"` | `"both"` \| `"hadhad_only"` \| `"inclusive_only"` |
| `exclude` | `""` | comma-separated jet types to skip entirely. Valid: `AK4`, `AK8`, `AK15`, `Tau` |
| `bg-channels` | `"DYto2Tau, TTto2L2Nu, TTto4Q, TTtoLNu2Q"` | comma-separated list of backgrounds |
| `max_files` | `150` | how many ROOT files per dataset |
| `signal_dirs` | `{}` | `name: path` for local datasets |
| `signal_das` | `{}` | `name: path` for DAS queries |
| `signal_xsec` | `{}` | `name: cross section (pb)` for signals |
| `bg_dirs` | `{}` |  |
| `bg_das` | `{}` |  |
| `bg_xsec` | `{}` | `name: cross section (pb)` for backgrounds |

### Plotting

| Key | Default | Meaning |
|---|---|---|
| `jet_params` | `"XX_mass, XX_pt, XX_mass/XX_pt, abs(XX_eta)"` | comma-seperated list of fields that you want distributions of |
| `subjet_params` | `"XX_Subjet_mass, XX_Subjet_pt, ..."` | same for subjets |
| `normalize` | `"true"` | dist. plots normalized to [0,1] |

### Training

Note: Training doesn't respect --cores N but instead has dedicated resource pools to use everything for one model at a time: there are 4 workes per model so total memory usage will be 4 * worker_memory_gb.

| Key | Default | Meaning |
|---|---|---|
| `sigs` | `["Tau", "fatJet", "AK15"]` | signals to train/compare |
| `training_channel` | `"mix"` | `"hadhad"` \| require hadronic decay? |
| `sig_generator` | `"MADGRAPH"` | single dataset to train on |
| `bg` | ["TTto4Q", "TTto2L2Nu", "TTtoLNu2Q", "DYto2Tau", "TTto2L2Nu, TTtoLNu2Q", "TTto4Q, TTto2L2Nu, TTtoLNu2Q, DYto2Tau"] | a list of comma-separated strings: what mixes of background to use for training |
| `num_taus` | `2` | `1` or `2` (1 is legacy so may crash) |
| `use_subjets` | `False` | use subjets for training |
| `cms_label` | `"Work in Progress"` |  |
| `compare_names` | jet type names | display names for jet types in plots, comma seperated list |
| `use_all` | `False` | use all of the data for results or unseen validation set |
| `variables` | `"greedy"` | you can define sets of fields to be used for training in `var_sets` dictionary below (empty = all available) |
| `fpr` | `[1.0]` | list of false-positive rates to plot (as **percent**) |
| `use_weights` | `False` | apply per-event weights (xsec-based) during training |
| `worker_memory_gb` | `5` | memory (GB) allocated per training worker |





