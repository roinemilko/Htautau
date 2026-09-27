#!/bin/bash
# Purely informational: reports how far along this checkout is (which
# outputs already exist on disk). Never fails - a fresh checkout is expected
# to have nothing yet, that's not a problem with the repo.
set -uo pipefail
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)/lib/common.sh"
cd "$REPO_ROOT"

report_dir() {
    local label="$1" dir="$2" n
    if [[ -d "$dir" ]]; then
        n=$(find "$dir" -mindepth 1 -maxdepth 1 2>/dev/null | wc -l | tr -d ' ')
        pass "$label: present ($n entries) - $dir"
    else
        skip "$label: not produced yet - $dir"
    fi
}

report_dir "data_workflow signal skims"    "data_workflow/jets"
report_dir "data_workflow background skims" "data_workflow/background"
report_dir "plot_workflow plots"           "plot_workflow/plots"
report_dir "training_workflow results"     "training_workflow/results"
report_dir "published RESULTS/"            "RESULTS"

exit 0
