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
    # training_workflow nests its output as results/<variables>/<training_channel>/<sig_generator>/...
    # - the sig_generator directory (e.g. MADGRAPH) is named after the same dataset plot_workflow
    # publishes under, so look it up by that name and link it into that dataset's RESULTS dir
    # instead of under a separate top-level one.
    #
    # A checkout can accumulate stale result trees from an earlier training_channel
    # or variables setting (e.g. leftover results/hadhad/MADGRAPH or
    # results/None/mix/MADGRAPH next to the current results/mix/MADGRAPH), so rather
    # than search-and-guess, mirror the Snakefile's own OUT_DIR_BASE formula
    # (results/<variables>/<training_channel>/<sig_generator>) from config.yaml to
    # land on the exact current directory deterministically.
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

cat > "$RESULTS_DIR/MANIFEST.md" <<EOF
# RESULTS

Generated: $(date -u +"%Y-%m-%dT%H:%M:%SZ")
Config used: config.snapshot.yaml (copy of config.yaml at publish time)

This tree is derived output, rebuilt from scratch every time
\`reproduce.sh\` or \`publish_results.sh\` runs. Every entry below is a
symlink back into the workflow that produced it - nothing here is
duplicated on disk.

- \`<dataset>/distributions/\`  -> plot_workflow/plots/<dataset>/distributions
- \`<dataset>/efficiencies/\`   -> plot_workflow/plots/<dataset>/efficiencies
- \`<dataset>/sanity_checks/\`  -> plot_workflow/plots/<dataset>/sanity_checks
- \`<dataset>/training/\`       -> training_workflow/results/.../<dataset>
  (the sig_generator trained on - only present for datasets training_workflow
  actually ran on; nested below by background-set/subjets, see
  training_workflow/README.md)

Intermediate skims (data_workflow/jets, data_workflow/background) are not
published here since they are large ROOT files, not final results.
EOF

echo "==> Done. See $RESULTS_DIR/MANIFEST.md"
