source /cvmfs/sft.cern.ch/lcg/views/LCG_109/x86_64-el9-gcc15-opt/setup.sh
exec snakemake --latency-wait 120 --cores="all" "$@"
