#!/bin/bash
# Smoke-tests every ROOT macro in the repo by loading it, catching C++
# syntax/type errors and missing-include problems cheaply and without
# needing any real data.
#
# data_workflow's skimmer macros are ACTUALLY COMPILED via ACLiC in
# production (data_workflow/Snakefile's compile/compile_bg rules), so they
# are checked the same way here. plot_workflow's macros are only ever
# CLING-interpreted in production (no '+' suffix in plot_workflow/Snakefile's
# shell: calls) - loading them with '.L' (no ACLiC) matches that. A clean
# load/compile here does not guarantee the macro succeeds when actually
# called with real arguments, but it does catch most real breakage
# (typos, missing headers, signature mismatches) before a long run gets to it.
set -uo pipefail
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)/lib/common.sh"
try_source_lcg
cd "$REPO_ROOT"

if ! command -v root >/dev/null 2>&1; then
    skip "root not found on PATH - cannot check macro compilation"
    exit 2
fi

fail_count=0

# ACLiC-compiles a macro, matching data_workflow's own compile/compile_bg rules.
compile_macro() {
    local file="$1" dir base out
    dir="$(dirname "$file")"
    base="$(basename "$file")"
    if out=$(cd "$dir" && root -l -b -q -e "int s = gSystem->CompileMacro(\"$base\", \"kf\"); gSystem->Exit(s == 1 ? 0 : 1);" 2>&1); then
        pass "$file compiles (ACLiC)"
    else
        fail "$file failed to compile:"
        echo "$out" | tail -25 | sed 's/^/     /' >&2
        fail_count=$((fail_count + 1))
    fi
}

# CLING-loads a macro without compiling it, matching plot_workflow's own usage.
load_macro() {
    local file="$1" dir base out
    dir="$(dirname "$file")"
    base="$(basename "$file")"
    if out=$(cd "$dir" && root -l -b -q -e ".L $base" 2>&1); then
        pass "$file loads (interpreted)"
    else
        fail "$file failed to load:"
        echo "$out" | tail -25 | sed 's/^/     /' >&2
        fail_count=$((fail_count + 1))
    fi
}

compile_macro "data_workflow/skimmer/BuildData.C"
compile_macro "data_workflow/skimmer/BuildBg.C"

shopt -s nullglob
for f in plot_workflow/*.C; do
    load_macro "$f"
done
shopt -u nullglob

if [[ $fail_count -gt 0 ]]; then
    exit 1
fi
exit 0
