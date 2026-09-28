#!/bin/bash
# Dry-runs the 'all' target of every workflow against config.yaml.
set -uo pipefail
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)/lib/common.sh"
try_source_lcg
cd "$REPO_ROOT"

if ! command -v snakemake >/dev/null 2>&1; then
    skip "snakemake not found on PATH - cannot check workflow DAGs"
    exit 2
fi

fail_count=0

check_dag() {
    local workflow="$1" target="$2" out
    if out=$(cd "$REPO_ROOT/$workflow" && snakemake --configfile ../config.yaml -n "$target" 2>&1); then
        pass "$workflow: '$target' DAG resolves (config + inputs are consistent)"
    else
        fail "$workflow: '$target' DAG failed to resolve:"
        (echo "$out" | grep -A6 -E "WorkflowError|Error in rule|MissingInputException" | head -30) | sed 's/^/     /' >&2
        fail_count=$((fail_count + 1))
    fi
}

check_dag data_workflow all
check_dag plot_workflow all
check_dag training_workflow all

if [[ $fail_count -gt 0 ]]; then
    exit 1
fi
exit 0
