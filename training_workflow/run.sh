source /cvmfs/sft.cern.ch/lcg/views/LCG_109/x86_64-el9-gcc15-opt/setup.sh

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd -P)"

# --resources caps concurrent BDT train/inference jobs to prevent OOM 
exec snakemake --configfile "$REPO_ROOT/config.yaml" --latency-wait 120 --resources bdt_slot=1 plot_slot=4 "$@"
    