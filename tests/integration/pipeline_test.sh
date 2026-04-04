#!/bin/bash
# Post-preprocessor pipeline test for PHC (Phoenics)
#
# Tests the production pipeline: cc -E | phc | cc
# This verifies phc works correctly on preprocessed C source.
#
# Usage: pipeline_test.sh <path-to-phc> <dir-of-.phc-files>

set -euo pipefail

CC="${CC:-clang}"
PHC="${1:?Usage: $0 <path-to-phc> [dir]}"
DIR="${2:-tests/integration/cases}"
TMPDIR="$(mktemp -d)"
trap 'rm -rf "$TMPDIR"' EXIT

PASS=0
FAIL=0
TOTAL=0

echo ""
echo "PHC Post-Preprocessor Pipeline Test"
echo "===================================="
echo "Pipeline: $CC -x c -E -P | $PHC | $CC -x c -"
echo ""

for phc_file in "$DIR"/*.phc; do
    [ -f "$phc_file" ] || continue
    base="$(basename "$phc_file" .phc)"
    run_expected="$DIR/${base}.run_expected"
    error_expected="$DIR/${base}.expected_error"

    # Only test files with run_expected (need main() for full pipeline)
    [ -f "$run_expected" ] || continue
    # Skip error cases
    [ -f "$error_expected" ] && continue

    TOTAL=$((TOTAL + 1))
    printf "  %-40s " "$base"

    # Pipeline: preprocess → phc → compile
    if ! $CC -x c -E -P "$phc_file" 2>/dev/null | "$PHC" 2>"$TMPDIR/phc_err" | \
         $CC -std=c11 -Wno-unused-function -x c - -o "$TMPDIR/$base" 2>"$TMPDIR/cc_err"; then
        phc_err=$(cat "$TMPDIR/phc_err" 2>/dev/null)
        cc_err=$(cat "$TMPDIR/cc_err" 2>/dev/null)
        if [ -n "$phc_err" ]; then
            echo "FAIL (phc error: $phc_err)"
        else
            echo "FAIL (compile error)"
            head -5 "$TMPDIR/cc_err" | sed 's/^/    /'
        fi
        FAIL=$((FAIL + 1))
        continue
    fi

    # Run and check output
    expected="$(cat "$run_expected")"
    actual=$("$TMPDIR/$base" 2>/dev/null) || {
        echo "FAIL (runtime crash)"
        FAIL=$((FAIL + 1))
        continue
    }

    if [ "$actual" = "$expected" ]; then
        echo "PASS"
        PASS=$((PASS + 1))
    else
        echo "FAIL (output mismatch)"
        echo "    expected: $expected"
        echo "    actual:   $actual"
        FAIL=$((FAIL + 1))
    fi
done

echo ""
echo "========================================"
echo "Pass: $PASS | Fail: $FAIL | Total: $TOTAL"
echo "Gate: $([ $FAIL -gt 0 ] && echo 'BLOCKED' || echo 'OPEN')"
echo "========================================"

[ $FAIL -eq 0 ]
