#!/bin/bash
# Checks the one thing every workflow depends on before anything else: that
# config.yaml exists and is valid YAML. Deliberately does NOT re-check
# individual keys here - that validation already lives in each Snakefile
# (see check_snakefile_dags.sh), and duplicating it here would just give it a
# second place to drift out of sync.
set -uo pipefail
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)/lib/common.sh"
cd "$REPO_ROOT"

if [[ ! -f config.yaml ]]; then
    fail "config.yaml not found at repo root ($REPO_ROOT/config.yaml)"
    exit 1
fi
pass "config.yaml exists"

PY=python3
command -v "$PY" >/dev/null 2>&1 || PY=python

if ! "$PY" -c "import yaml" >/dev/null 2>&1; then
    skip "PyYAML not importable via '$PY' - cannot verify config.yaml parses (snakemake itself will still catch this)"
    exit 2
fi

if ! err=$("$PY" -c "import yaml; yaml.safe_load(open('config.yaml'))" 2>&1); then
    fail "config.yaml is not valid YAML:"
    echo "$err" | sed 's/^/     /' >&2
    exit 1
fi
pass "config.yaml parses as valid YAML"

exit 0
