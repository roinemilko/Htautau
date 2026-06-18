source /cvmfs/sft.cern.ch/lcg/views/LCG_109/x86_64-el9-gcc15-opt/setup.sh
exec python3 -m snakemake --latency-wait 120 "$@"
