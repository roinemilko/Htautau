#!/bin/bash
# Checks that the tools every workflow depends on are reachabl and reports the grid-proxy status. 
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)/lib/common.sh"
try_source_lcg
cd "$REPO_ROOT"

fail_count=0

check_tool() {
    local tool="$1"
    local path
    path="$(command -v "$tool" 2>/dev/null)" || true
    if [[ -n "$path" ]]; then
        pass "$tool found: $path"
    else
        fail "$tool not found on PATH"
        fail_count=$((fail_count + 1))
    fi
}

for t in snakemake root python3 dasgoclient voms-proxy-info voms-proxy-init; do
    check_tool "$t"
done

LCG_SETUP="/cvmfs/sft.cern.ch/lcg/views/LCG_109/x86_64-el9-gcc15-opt/setup.sh"
if [[ -f "$LCG_SETUP" ]]; then
    pass "LCG_109 view present: $LCG_SETUP"
else
    warn "LCG_109 view not found at $LCG_SETUP" 
fi

if command -v voms-proxy-info >/dev/null 2>&1; then
    if voms-proxy-info --exists >/dev/null 2>&1; then
        timeleft="$(voms-proxy-info --timeleft 2>/dev/null || echo '?')"
        pass "grid proxy valid (timeleft: ${timeleft}s)"
    else
        warn "no valid grid proxy (voms-proxy-info --exists failed)."
        info "Run 'voms-proxy-init --voms cms"
    fi
fi

if [[ $fail_count -gt 0 ]]; then
    exit 1
fi
exit 0
