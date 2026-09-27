#!/bin/bash
# Regression tests for reproduce.sh and publish_results.sh's own CLI
# behavior - the things that were manually verified while building them.
# Everything here is safe to run against the real repo: it only ever uses
# --dry-run / --help, an isolated scratch copy, or a real (idempotent) rebuild
# of RESULTS/.
set -uo pipefail
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)/lib/common.sh"
cd "$REPO_ROOT"

fail_count=0

# --help works and mentions config.yaml
if out=$(./reproduce.sh --help 2>&1) && echo "$out" | grep -q "config.yaml"; then
    pass "reproduce.sh --help works"
else
    fail "reproduce.sh --help did not behave as expected"
    echo "$out" | sed 's/^/     /' >&2
    fail_count=$((fail_count + 1))
fi

# an unknown --stages value is rejected with a clear message, not silently ignored
if out=$(./reproduce.sh --stages bogus --dry-run 2>&1); then
    fail "reproduce.sh --stages bogus should have failed but exited 0"
    fail_count=$((fail_count + 1))
elif echo "$out" | grep -qi "unknown stage"; then
    pass "reproduce.sh rejects an unknown --stages value"
else
    fail "reproduce.sh --stages bogus failed, but not with the expected message:"
    echo "$out" | sed 's/^/     /' >&2
    fail_count=$((fail_count + 1))
fi

# a missing config.yaml is caught immediately, before touching any workflow.
# Uses an isolated scratch copy of just reproduce.sh so the real repo is never touched.
scratch="$(mktemp -d)"
cp reproduce.sh "$scratch/"
out=$("$scratch/reproduce.sh" --stages data --dry-run 2>&1)
code=$?
rm -rf "$scratch"
if [[ $code -ne 0 ]] && echo "$out" | grep -qi "config.yaml not found"; then
    pass "reproduce.sh detects a missing config.yaml"
else
    fail "reproduce.sh did not detect a missing config.yaml as expected (exit=$code):"
    echo "$out" | sed 's/^/     /' >&2
    fail_count=$((fail_count + 1))
fi

# the data stage either runs cleanly (valid proxy) or fails specifically
# because of the proxy (invalid/missing) - both are acceptable outcomes here,
# since proxy validity is a property of the machine, not of this repo.
out=$(./reproduce.sh --stages data --dry-run 2>&1)
code=$?
if [[ $code -eq 0 ]]; then
    pass "reproduce.sh --stages data --dry-run succeeded (grid proxy is valid here)"
elif echo "$out" | grep -qi "proxy"; then
    pass "reproduce.sh --stages data --dry-run failed for the expected reason (no valid proxy)"
else
    fail "reproduce.sh --stages data --dry-run failed for an unexpected reason:"
    echo "$out" | sed 's/^/     /' >&2
    fail_count=$((fail_count + 1))
fi

# publish_results.sh rebuilds RESULTS/ cleanly (real run - already designed to
# be idempotent and safe to re-run at any time)
if out=$(./publish_results.sh 2>&1); then
    if [[ -f RESULTS/MANIFEST.md ]]; then
        pass "publish_results.sh rebuilds RESULTS/MANIFEST.md"
    else
        fail "publish_results.sh exited 0 but RESULTS/MANIFEST.md is missing"
        fail_count=$((fail_count + 1))
    fi
else
    fail "publish_results.sh failed:"
    echo "$out" | sed 's/^/     /' >&2
    fail_count=$((fail_count + 1))
fi

if [[ $fail_count -gt 0 ]]; then
    exit 1
fi
exit 0
