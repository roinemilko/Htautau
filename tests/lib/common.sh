# Shared helpers for tests/check_*.sh scripts.

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd -P)"

if [[ -t 1 ]]; then
    _C_RED=$'\033[31m'; _C_GREEN=$'\033[32m'; _C_YELLOW=$'\033[33m'; _C_RESET=$'\033[0m'
else
    _C_RED=""; _C_GREEN=""; _C_YELLOW=""; _C_RESET=""
fi

pass() { echo "${_C_GREEN}PASS${_C_RESET} $*"; }
fail() { echo "${_C_RED}FAIL${_C_RESET} $*" >&2; }
skip() { echo "${_C_YELLOW}SKIP${_C_RESET} $*"; }
warn() { echo "${_C_YELLOW}WARN${_C_RESET} $*"; }
info() { echo "     $*"; }

try_source_lcg() {
    local setup="/cvmfs/sft.cern.ch/lcg/views/LCG_109/x86_64-el9-gcc15-opt/setup.sh"
    if [[ -f "$setup" ]]; then
        local had_u=0
        case "$-" in *u*) had_u=1 ;; esac
        set +u
        # shellcheck disable=SC1090
        source "$setup" >/dev/null 2>&1 || true
        [[ $had_u -eq 1 ]] && set -u
    fi
}
