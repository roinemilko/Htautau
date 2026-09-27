## Training BDT
### Goal
The idea of this code is to provide a workflow for training signal-vs-background tagging for identifying Tau objects and $H\to \tau\tau$ events against $tt$ -background or DY production. These are the biggest expected background and the most difficult background in boosted $H\to\tau\tau$ -analyses. The Boosted Desicion tree itself is very simple and uses ParticleNet, ParT and DeepTau taggers as inputs. The main purpose is to evaluate the tagging performance of AK4/AK8/AK15 jets and subjets independently of taggers and variable distributions.
### Usage
The workflow consists of the following steps:
```mermaid
flowchart LR
A[Skimmed & matched signal]
B[Skimmed background]
A-->C[Preprocessing & correlation]
B-->C
C-->D[Model training]
D-->F[Model inference]
```
which are modular and can be run independently. Like everything else this runs in [LCG 109](https://lcginfo.cern.ch/release/109/). The following inference is provided:
| Inference | Rule |
|----------|:-------------:|
| ROC -curves, confusion matrices at 0.5 and feature gains | bdt_inference (note: this also trains the models and computes the inference)|
|$\epsilon_{\text{sig.}}=\frac{\text{tagged jets}\cap \text{matched AK4}\cap \text{matched AK8} \cap \text{matched AK15}}{\text{matched AK4}\cap \text{matched AK8}\cap\text{matched AK15}}$  vs. Higgs $p_T$| compare_bdts
|$\epsilon_{\text{tag.}}=\frac{\text{tagged jets}}{\text{matched jets}}$ vs Higgs $p_t$|compare_tagging_effs|
Total eff. of reco. process by
- Start with full set of generated events
- If signal event is unmatched, manually set tagger score to 0
- Signal efficiency
```bash
./run.sh --config #optional flags
```
with optional flags listed here:compare_tagging_effs
| Flag (set to preset value) | Usage |
|----------|:-------------:|
| sig="TauHadHad" | Specify which signal to use. Add own datasets to SIGNAL_DATA_DIRS in the snakefile. | 
| bg="TTto4Q" | Specify which background(s) to use. Add ownn datasets to BG_DATA_DIRS in the snakefile. |
| num_taus=1 | 1 or 2: Train on single jets or matched jet pairs (for AK4 and subjets).  |
|seed=100 |the seed used for train/test splitting of the dataset (for reproducibility) |
|cms_label="Work in Progress"| for the visual representation of the results
|variables = '[]' (all available will be used)| List of params to train on. Note: for now you when ntaus = 2 you have to have this variable for both jets. Fix incoming. 

Only spesific parts of the pipeline can also be run with the self-explatonary rules plot_correlations, train_bdt and bdt_inference (if a previous result is missing it is also run).    