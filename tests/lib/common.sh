# Shared helpers for tests/check_*.sh scripts. Source this at the top of every
# check script:
#
#   source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)/lib/common.sh"
#
# It defines REPO_ROOT and pass/fail/skip/warn/info print helpers, and the
# exit-code convention every check script follows:
#   0 = pass, 1 = fail, 2 = skip (not applicable here - not a failure)

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

# Best-effort: makes root/python3/pytest/dasgoclient etc. available for checks
# that need them, without hard-failing if CVMFS isn't reachable from here.
# Every check that needs these tools calls this itself, so it stays runnable
# standalone (matching how every workflow's own run.sh sources this itself
# rather than assuming a caller already did).
try_source_lcg() {
    local setup="/cvmfs/sft.cern.ch/lcg/views/LCG_109/x86_64-el9-gcc15-opt/setup.sh"
    if [[ -f "$setup" ]]; then
        # The LCG view's own setup.sh references at least one unset variable
        # internally - under our set -u that would abort THIS shell (not just
        # the sourced one) with the error silently swallowed by the redirect
        # below. Disable -u for the duration of the source only.
        local had_u=0
        case "$-" in *u*) had_u=1 ;; esac
        set +u
        # shellcheck disable=SC1090
        source "$setup" >/dev/null 2>&1 || true
        [[ $had_u -eq 1 ]] && set -u
    fi
}
