#!/bin/bash
# Integration test runner for PHC (Phoenics)
# Usage: run_tests.sh <path-to-phc-binary>
#
# Test case files:
#   NN_name.phc             — input to PHC
#   NN_name.expected.c      — expected transpiler output (text match)
#   NN_name.expected_error  — expected error message substring (error case)
#   NN_name.run_expected    — expected stdout when compiled output is executed
#
# Each test does up to 3 checks:
#   1. PHC output matches expected output (text match)
#   2. The output compiles as valid C11 with the system compiler
#   3. If .run_expected exists, the compiled binary produces expected stdout

set -euo pipefail

CC="${CC:-clang}"
CFLAGS="-std=c11 -Wall -Wextra -Werror -pedantic"
PHC="$1"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
CASES_DIR="$SCRIPT_DIR/cases"
TMPDIR="$(mktemp -d)"
trap 'rm -rf "$TMPDIR"' EXIT

PASS=0
FAIL=0
SKIP=0
TOTAL=0

if [ -z "${PHC:-}" ]; then
    echo "Usage: $0 <path-to-phc>"
    exit 1
fi

for input_file in "$CASES_DIR"/*.phc; do
    [ -f "$input_file" ] || continue

    base="$(basename "$input_file" .phc)"
    expected_file="$CASES_DIR/${base}.expected.c"
    error_file="$CASES_DIR/${base}.expected_error"
    run_expected_file="$CASES_DIR/${base}.run_expected"

    TOTAL=$((TOTAL + 1))
    printf "  %-40s " "$base"

    if [ -f "$error_file" ]; then
        # === Error case: PHC should reject this input ===
        expected_error="$(cat "$error_file")"
        if actual_error=$("$PHC" < "$input_file" 2>&1); then
            echo "FAIL (expected error, got success)"
            FAIL=$((FAIL + 1))
            continue
        fi
        if echo "$actual_error" | grep -qF "$expected_error"; then
            echo "PASS"
            PASS=$((PASS + 1))
        else
            echo "FAIL (wrong error)"
            echo "    Expected error containing: $expected_error"
            echo "    Got: $actual_error"
            FAIL=$((FAIL + 1))
        fi

    elif [ -f "$expected_file" ]; then
        # === Success case: PHC should produce expected output ===

        # Step 1: Run PHC and capture output
        actual=$("$PHC" < "$input_file" 2>/dev/null) || {
            echo "FAIL (phc returned error)"
            FAIL=$((FAIL + 1))
            continue
        }
        expected="$(cat "$expected_file")"

        # Step 2: Check text match
        if [ "$actual" != "$expected" ]; then
            echo "FAIL (output mismatch)"
            diff <(echo "$actual") <(echo "$expected") | head -30
            FAIL=$((FAIL + 1))
            continue
        fi

        # Step 3: Verify output compiles as valid C11
        out_c="$TMPDIR/${base}.c"
        out_bin="$TMPDIR/${base}"
        echo "$actual" > "$out_c"
        if ! $CC $CFLAGS -o "$out_bin" "$out_c" 2>"$TMPDIR/${base}.compile_err"; then
            echo "FAIL (output does not compile)"
            echo "    Compiler errors:"
            sed 's/^/    /' "$TMPDIR/${base}.compile_err" | head -20
            FAIL=$((FAIL + 1))
            continue
        fi

        # Step 4: If run_expected exists, execute and check stdout
        if [ -f "$run_expected_file" ]; then
            run_expected="$(cat "$run_expected_file")"
            run_actual=$("$out_bin" 2>/dev/null) || {
                echo "FAIL (compiled binary crashed)"
                FAIL=$((FAIL + 1))
                continue
            }
            if [ "$run_actual" != "$run_expected" ]; then
                echo "FAIL (runtime output mismatch)"
                echo "    Expected: $run_expected"
                echo "    Actual:   $run_actual"
                FAIL=$((FAIL + 1))
                continue
            fi
        fi

        echo "PASS"
        PASS=$((PASS + 1))

    else
        echo "SKIP (no expected file)"
        SKIP=$((SKIP + 1))
    fi
done

echo ""
echo "  ========================================"
echo "  Pass: $PASS | Fail: $FAIL | Skip: $SKIP | Total: $TOTAL"
echo "  Gate: $([ $FAIL -gt 0 ] || [ $SKIP -gt 0 ] && echo 'BLOCKED' || echo 'OPEN') (full_suite: $([ $TOTAL -gt 0 ] && echo 'yes' || echo 'no'))"
echo "  ========================================"

if [ $FAIL -gt 0 ] || [ $SKIP -gt 0 ]; then
    exit 1
fi
if [ $TOTAL -eq 0 ]; then
    echo "  WARNING: no test cases found"
    exit 1
fi
