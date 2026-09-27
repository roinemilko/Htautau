#!/bin/bash
# Publishes results from plot/training workflows to one place (/RESULTS) for readability
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
RESULTS_DIR="$REPO_ROOT/RESULTS"
PLOTS_DIR="$REPO_ROOT/plot_workflow/plots"
TRAINING_DIR="$REPO_ROOT/training_workflow/results"

if [[ -d "$RESULTS_DIR" && -n "$(ls -A "$RESULTS_DIR" 2>/dev/null)" && ! -e "$RESULTS_DIR/MANIFEST.md" ]]; then
    echo "error: $RESULTS_DIR is non-empty and doesn't look like a previously published" >&2
    echo "RESULTS tree (no MANIFEST.md found). Move it aside before publishing." >&2
    exit 1
fi

rm -rf "$RESULTS_DIR"
mkdir -p "$RESULTS_DIR"

echo "==> Publishing plot_workflow outputs"
if [[ -d "$PLOTS_DIR" ]]; then
    for dataset_dir in "$PLOTS_DIR"/*/; do
        [[ -d "$dataset_dir" ]] || continue
        dataset="$(basename "$dataset_dir")"
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

echo "==> Publishing outputs"
if [[ -d "$TRAINING_DIR" ]]; then
    ln -s "$(realpath --relative-to="$RESULTS_DIR" "$TRAINING_DIR")" "$RESULTS_DIR/training"
else
    echo "    (skipped: $TRAINING_DIR not found - run training_workflow first)"
fi

cp "$REPO_ROOT/config.yaml" "$RESULTS_DIR/config.snapshot.yaml"

cat > "$RESULTS_DIR/MANIFEST.txt" <<EOF
# RESULTS

Generated: $(date -u +"%Y-%m-%dT%H:%M:%SZ")
Config used: config.snapshot.yaml

EOF

echo "==> Done."
