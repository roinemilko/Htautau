#!/bin/bash
#   0 = pass, 1 = fail, 2 = skip (not applicable here, e.g. a tool isn't
#   installed).
set -uo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
TESTS_DIR="$REPO_ROOT/tests"
# shellcheck source=tests/lib/common.sh
source "$TESTS_DIR/lib/common.sh"

usage() {
    cat <<EOF
Usage: $(basename "$0") [--only NAME] [-h]

Runs every check in tests/. Each one is also runnable standalone, e.g.:
  bash tests/check_environment.sh

Options:
  --only NAME   Run just one check - a tests/<NAME> filename, with or
                without its extension (e.g. --only check_config_yaml)
  -h, --help    Show this help
EOF
}

ONLY=""
while [[ $# -gt 0 ]]; do
    case "$1" in
        --only) ONLY="$2"; shift 2 ;;
        -h|--help) usage; exit 0 ;;
        *) echo "Unknown argument: $1" >&2; usage; exit 1 ;;
    esac
done

SHELL_CHECKS=(
    check_environment.sh
    check_config_yaml.sh
    check_cpp_macros_compile.sh
    check_snakefile_dags.sh
    check_orchestrator.sh
    check_data_presence.sh
)
PYTEST_FILES=(
    test_python_units.py
)

n_pass=0
n_fail=0
n_skip=0
FAILED_NAMES=()

matches_only() {
    local name="$1"
    [[ -z "$ONLY" ]] && return 0
    [[ "$name" == "$ONLY" || "$name" == "$ONLY.sh" || "$name" == "$ONLY.py" ]]
}

run_shell_check() {
    local name="$1" code
    matches_only "$name" || return 0
    echo
    echo "──── $name ────"
    bash "$TESTS_DIR/$name"
    code=$?
    if [[ $code -eq 0 ]]; then
        n_pass=$((n_pass + 1))
    elif [[ $code -eq 2 ]]; then
        n_skip=$((n_skip + 1))
    else
        n_fail=$((n_fail + 1))
        FAILED_NAMES+=("$name")
    fi
}

run_pytest() {
    local any_match=false
    for f in "${PYTEST_FILES[@]}"; do
        matches_only "$f" && any_match=true
    done
    $any_match || return 0

    echo
    echo "──── python unit tests (pytest) ────"

    local python=python3
    command -v "$python" >/dev/null 2>&1 || python=python
    try_source_lcg

    if ! "$python" -c "import pytest" >/dev/null 2>&1; then
        echo "SKIP pytest not available (LCG_109 env propably not sourced)"
        n_skip=$((n_skip + 1))
        return
    fi

    ( cd "$TESTS_DIR" && "$python" -m pytest "${PYTEST_FILES[@]}" -q -p no:ctest_measurements_reporter )
    local code=$?
    if [[ $code -eq 0 ]]; then
        n_pass=$((n_pass + 1))
    else
        n_fail=$((n_fail + 1))
        FAILED_NAMES+=("python unit tests")
    fi
}

for check in "${SHELL_CHECKS[@]}"; do
    run_shell_check "$check"
done
run_pytest

echo
echo "════ summary ════"
echo "pass: $n_pass   fail: $n_fail   skip: $n_skip"
if [[ ${#FAILED_NAMES[@]} -gt 0 ]]; then
    echo "failed: ${FAILED_NAMES[*]}"
    exit 1
fi
exit 0
