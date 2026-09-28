#!/bin/bash
# Publishes results from plot/training workflows to one place (/RESULTS) for readability
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
RESULTS_DIR="$REPO_ROOT/RESULTS"
PLOTS_DIR="$REPO_ROOT/plot_workflow/plots"
TRAINING_DIR="$REPO_ROOT/training_workflow/results"

if [[ -d "$RESULTS_DIR" && -n "$(ls -A "$RESULTS_DIR" 2>/dev/null)" && ! -e "$RESULTS_DIR/MANIFEST.txt" ]]; then
    echo "error: $RESULTS_DIR is non-empty and doesn't look like a previously published" >&2
    echo "RESULTS tree (no MANIFEST.txt found)" >&2
    exit 1
fi

rm -rf "$RESULTS_DIR"
mkdir -p "$RESULTS_DIR"

declare -a DATASETS=()

echo "==> Publishing plot_workflow outputs"
if [[ -d "$PLOTS_DIR" ]]; then
    for dataset_dir in "$PLOTS_DIR"/*/; do
        [[ -d "$dataset_dir" ]] || continue
        dataset="$(basename "$dataset_dir")"
        DATASETS+=("$dataset")
        mkdir -p "$RESULTS_DIR/$dataset"
        for category in distributions efficiencies sanity_checks; do
            src="$dataset_dir$category"
            if [[ -d "$src" ]]; then
                ln -s "$(realpath --relative-to="$RESULTS_DIR/$dataset" "$src")" "$RESULTS_DIR/$dataset/$category"
            fi
        done
    done
else
    echo "    (skipped: $PLOTS_DIR not found - run plot_workflow first)"
fi

echo "==> Publishing training_workflow outputs"
if [[ -d "$TRAINING_DIR" ]]; then

    current_channel="$(grep -E '^training_channel:' "$REPO_ROOT/config.yaml" | head -1 | sed -E 's/^training_channel:[[:space:]]*"?([^"[:space:]]*)"?.*/\1/')"
    current_variables="$(grep -E '^variables:' "$REPO_ROOT/config.yaml" | head -1 | sed -E 's/^variables:[[:space:]]*"?([^"[:space:]]*)"?.*/\1/')"
    vars_prefix="${current_variables:+${current_variables}/}"
    found_any=0
    for dataset in "${DATASETS[@]}"; do
        sig_gen_dir=""
        if [[ -n "$current_channel" && -d "$TRAINING_DIR/${vars_prefix}${current_channel}/$dataset" ]]; then
            sig_gen_dir="$TRAINING_DIR/${vars_prefix}${current_channel}/$dataset"
        else
            mapfile -t candidates < <(find "$TRAINING_DIR" -type d -name "$dataset")
            if [[ ${#candidates[@]} -gt 0 ]]; then
                sig_gen_dir="${candidates[0]}"
                echo "    (note: config.yaml's current training_channel/variables don't match any" \
                     "result tree for '$dataset' - publishing the stale-looking $sig_gen_dir instead," \
                     "re-run training_workflow to refresh it)"
            fi
        fi
        [[ -z "$sig_gen_dir" ]] && continue

        mkdir -p "$RESULTS_DIR/$dataset"
        ln -s "$(realpath --relative-to="$RESULTS_DIR/$dataset" "$sig_gen_dir")" "$RESULTS_DIR/$dataset/training"
        found_any=1
    done
    if [[ "$found_any" -eq 0 ]]; then
        echo "    (skipped: no per-dataset results found under $TRAINING_DIR)"
    fi
else
    echo "    (skipped: $TRAINING_DIR not found - run training_workflow first)"
fi

cp "$REPO_ROOT/config.yaml" "$RESULTS_DIR/config.snapshot.yaml"

cat > "$RESULTS_DIR/MANIFEST.txt" <<EOF
# RESULTS

Generated: $(date -u +"%Y-%m-%dT%H:%M:%SZ")
Config used: config.snapshot.yaml


EOF

echo "==> Done. See $RESULTS_DIR/MANIFEST.md"
