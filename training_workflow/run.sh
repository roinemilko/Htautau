source /cvmfs/sft.cern.ch/lcg/views/LCG_109/x86_64-el9-gcc15-opt/setup.sh

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd -P)"

# Cap concurrent BDT train/inference jobs (each spins up its own ~20GB dask
# LocalCluster) and concurrent plotting jobs (each loads full inference
# parquets/ROOT trees into memory) independently of --cores, so a high core
# count doesn't OOM the machine. Placed before "$@" so an explicit --resources
# on the command line still wins.
exec snakemake --configfile "$REPO_ROOT/config.yaml" --latency-wait 120 --resources bdt_slot=1 plot_slot=4 "$@"
    