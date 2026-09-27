#!/bin/bash

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"

STAGES="data,plots,training,publish"
CORES=""
DRY_RUN=false
EXTRA_ARGS=()

usage() {
    cat <<EOF
Usage: $(basename "$0") [options] [-- extra snakemake args]

Runs the full Htautau analysis pipeline against $REPO_ROOT/config.yaml.

Options:
  --stages LIST   Comma-separated subset of: data,plots,training,publish
                  (default: $STAGES)
  --cores N       Forwarded as --cores N to every snakemake invocation
  --dry-run       Forward -n to snakemake and skip the publish stage
  -h, --help      Show this help
  --              Add args to be forwarded verbatim to snakemake calls
                  (e.g. --n --forceall, --rerun-incomplete)
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --stages)
            STAGES="$2"
            shift 2
            ;;
        --cores)
            CORES="$2"
            shift 2
            ;;
        --dry-run)
            DRY_RUN=true
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        --)
            shift
            EXTRA_ARGS=("$@")
            break
            ;;
        *)
            echo "Unknown argument: $1" >&2
            usage
            exit 1
            ;;
    esac
done

if $DRY_RUN; then
    EXTRA_ARGS+=("-n")
fi

SNAKEMAKE_ARGS=()
if [[ -n "$CORES" ]]; then
    SNAKEMAKE_ARGS+=(--cores "$CORES")
fi
SNAKEMAKE_ARGS+=("${EXTRA_ARGS[@]}")

if [[ ! -f "$REPO_ROOT/config.yaml" ]]; then
    echo "error: $REPO_ROOT/config.yaml not found." >&2
    exit 1
fi

IFS=',' read -ra STAGE_LIST <<< "$STAGES"

known_stage() {
    case "$1" in
        data|plots|training|publish) return 0 ;;
        *) return 1 ;;
    esac
}

for s in "${STAGE_LIST[@]}"; do
    if ! known_stage "$s"; then
        echo "error: unknown stage '$s' in --stages (valid: data, plots, training, publish)" >&2
        exit 1
    fi
done

want_stage() {
    local target="$1" s
    for s in "${STAGE_LIST[@]}"; do
        if [[ "$s" == "$target" ]]; then
            return 0
        fi
    done
    return 1
}

run_snakemake() {
    local dir="$1" target="$2"
    echo "==> [$dir] snakemake ${SNAKEMAKE_ARGS[*]} $target"
    ( cd "$REPO_ROOT/$dir" && ./run.sh "${SNAKEMAKE_ARGS[@]}" "$target" )
}

if want_stage data; then
    if ! voms-proxy-info --exists >/dev/null 2>&1; then
        echo "error: no valid grid proxy found (voms-proxy-info --exists failed)." >&2
        echo "Run 'voms-proxy-init --voms cms' before using the data stage." >&2
        exit 1
    fi
fi

if want_stage data; then
    run_snakemake data_workflow skim
    run_snakemake data_workflow skim_bg
fi

if want_stage plots; then
    run_snakemake plot_workflow all
fi

if want_stage training; then
    run_snakemake training_workflow all
fi

if want_stage publish; then
    if $DRY_RUN; then
        echo "==> Skipping publish stage (--dry-run)"
    else
        "$REPO_ROOT/publish_results.sh"
    fi
fi
