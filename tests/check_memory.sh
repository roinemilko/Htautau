#!/bin/bash
# Check the available system memory and makes sure makes sure that there are not too many parallel BDT training

set -uo pipefail
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)/lib/common.sh"
cd "$REPO_ROOT"

MEMINFO=/proc/meminfo
if [[ ! -r "$MEMINFO" ]]; then
    skip "can't read $MEMINFO (not Linux, or no /proc) - can't estimate a memory budget here"
    exit 2
fi

mem_total_kb=$(awk '/^MemTotal:/{print $2}' "$MEMINFO")
mem_avail_kb=$(awk '/^MemAvailable:/{print $2}' "$MEMINFO")
if [[ -z "$mem_avail_kb" ]]; then
    mem_avail_kb="$mem_total_kb"
fi
if [[ -z "$mem_total_kb" ]]; then
    skip "couldn't parse MemTotal from $MEMINFO"
    exit 2
fi

mem_total_gb=$(awk -v kb="$mem_total_kb" 'BEGIN { printf "%.1f", kb / 1024 / 1024 }')
mem_avail_gb=$(awk -v kb="$mem_avail_kb" 'BEGIN { printf "%.1f", kb / 1024 / 1024 }')
info "${mem_avail_gb} GiB currently available"

threads=$(awk '
    /^rule train_bdt:/ { in_rule = 1 }
    in_rule && /threads:/ { gsub(/[^0-9]/, "", $0); print; exit }
' training_workflow/Snakefile)
threads="${threads:-4}"

worker_gb=$(grep -oE '^worker_memory_gb:[[:space:]]*[0-9]+' config.yaml | grep -oE '[0-9]+' | tail -1)
worker_gb="${worker_gb:-5}"

per_job_gb=$((threads * worker_gb))
info "currently ~${per_job_gb}GB per train_bdt/run_bdt_inference job)"

safety_factor="0.9"
usable_gb=$(awk -v gb="$mem_avail_gb" -v f="$safety_factor" 'BEGIN { printf "%.1f", gb * f }')
max_slots=$(awk -v usable="$usable_gb" -v per_job="$per_job_gb" 'BEGIN {
    v = int(usable / per_job)
    print (v < 0) ? 0 : v
}')

if [[ "$max_slots" -lt 1 ]]; then
    fail "${per_job_gb}GB train_bdt/run_bdt_inference job doesn't fit in" \
         "${usable_gb}GB usable (${safety_factor} x available memory)."
    info "if you hit an OOM error, lower config.yaml's max_files or" \
         "worker_memory_gb"
    exit 1
fi
pass "max concurrent train jobs on this machine: $max_slots" \
     "(${usable_gb}GB usable / ${per_job_gb}GB per job)"

configured_slot=$(grep -oE 'bdt_slot=[0-9]+' training_workflow/run.sh | head -1 | grep -oE '[0-9]+')
if [[ -z "$configured_slot" ]]; then
    warn "couldn't find a 'bdt_slot=N' default in training_workflow/run.sh"
    exit 0
fi
info "currently configed bdt_slot=${configured_slot}"

if [[ "$configured_slot" -gt "$max_slots" ]]; then
    fail "bdt_slot=${configured_slot} in training_workflow/run.sh requests up to" \
         "$((configured_slot * per_job_gb))GB at once, more than the ~${usable_gb}GB estimated usable" \
         "on this machine"
    info "lower  bdt_slot to $max_slots (or less) in training_workflow/run.sh's --resources flag, or" \
         "if you hit an OOM error, lower config.yaml's max_files or worker_memory_gb instead."
    exit 1
fi

pass "bdt_slot=${configured_slot} OK"
exit 0
