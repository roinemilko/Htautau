source /cvmfs/sft.cern.ch/lcg/views/LCG_109/x86_64-el9-gcc15-opt/setup.sh

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd -P)"
export X509_USER_PROXY=$(voms-proxy-info -path)
exec snakemake --configfile "$REPO_ROOT/config.yaml" --latency-wait 120 "$@"