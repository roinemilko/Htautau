# tests/

Diagnostic checks for this repository. Run everything with `/diagnose.sh` from root. Codes:

| Exit code | Meaning |
|---|---|
| 0 | PASS |
| 1 | FAIL  |
| 2 |  not applicable here (e.g. a tool isn't installed) |

## What's here

| File | Checks |
|---|---|
| `check_environment.sh` | Checks dependencies, LGC environment and voms proxy health |
| `check_config_yaml.sh` | `config.yaml` exists and is valid YAML |
| `check_memory.sh` | Reports system memory and the max `train_bdt`/`run_bdt_inference` concurrency it supports; fails if `training_workflow/run.sh`'s `bdt_slot` is set too high for it |
| `check_cpp_macros_compile.sh` | ROOT macros (skimmer + plotting) compile |
| `check_snakefile_dags.sh` | Unit test DAG by running `all` in each workflow against the  `config.yaml`  |
| `check_orchestrator.sh` | Regression tests for `reproduce.sh` / `publish_results.sh` |
| `check_data_presence.sh` |  reports what's currently produced on disk  |
| `test_python_units.py` | unit tests for the helper functions in `training_workflow`'s |


## Note
Run
```
source /cvmfs/sft.cern.ch/lcg/views/LCG_109/x86_64-el9-gcc15-opt/setup.sh
```
before running the test suite. I didn't make end-to-end validation since it takes forever and can't publish data. This suite is for diagnosing failures for someone who is not familiar w the codebase.
