#!/bin/bash
# #line directive tests for PHC P2
# Verifies that compiler errors reference correct original source line numbers
# after phc inserts generated code.
#
# Each test:
#   1. Feeds a .phc file (with a deliberate error) through phc
#   2. Compiles phc's output
#   3. Checks that the compiler error references the expected line number
#
# Test case files (in cases/line_directive/):
#   NN_name.phc             — input with deliberate error
#   NN_name.expected_line   — expected line number in compiler error

set -euo pipefail

CC="${CC:-clang}"
CFLAGS="-std=c11 -Wno-unused-function"
PHC="$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
CASES_DIR="$SCRIPT_DIR/cases/line_directive"
TMPDIR="$(mktemp -d)"
trap 'rm -rf "$TMPDIR"' EXIT

PASS=0
FAIL=0
TOTAL=0

if [ -z "${PHC:-}" ]; then
    echo "Usage: $0 <path-to-phc>"
    exit 1
fi

if [ ! -d "$CASES_DIR" ]; then
    echo "  No line directive test cases found"
    exit 0
fi

run_test() {
    local input_file="$1"
    local expected_line="$2"
    local mode="$3"  # "direct" or "pipeline"
    local base="$4"
    local suffix=""
    [ "$mode" = "pipeline" ] && suffix=" [pipeline]"

    TOTAL=$((TOTAL + 1))
    printf "  %-40s " "${base}${suffix}"

    phc_out="$TMPDIR/${base}_${mode}.c"

    if [ "$mode" = "pipeline" ]; then
        # Pipeline: cc -E | phc | cc
        if ! $CC -x c -E "$input_file" 2>/dev/null | "$PHC" > "$phc_out" 2>"$TMPDIR/${base}_${mode}.phc_err"; then
            echo "FAIL (phc rejected pipeline input)"
            cat "$TMPDIR/${base}_${mode}.phc_err" | sed 's/^/    /'
            FAIL=$((FAIL + 1))
            return
        fi
    else
        # Direct: phc only
        if ! "$PHC" < "$input_file" > "$phc_out" 2>"$TMPDIR/${base}_${mode}.phc_err"; then
            echo "FAIL (phc rejected input)"
            cat "$TMPDIR/${base}_${mode}.phc_err" | sed 's/^/    /'
            FAIL=$((FAIL + 1))
            return
        fi
    fi

    # Compile — expect failure (deliberate error in source)
    compile_err="$TMPDIR/${base}_${mode}.cc_err"
    if $CC $CFLAGS -c -o /dev/null "$phc_out" 2>"$compile_err"; then
        echo "FAIL (compilation succeeded — deliberate error not triggered)"
        FAIL=$((FAIL + 1))
        return
    fi

    # Extract line number from compiler error
    actual_line=$(head -1 "$compile_err" | sed -n 's/^[^:]*:\([0-9]*\):.*/\1/p')

    if [ -z "$actual_line" ]; then
        echo "FAIL (could not parse compiler error line number)"
        echo "    Compiler output: $(head -1 "$compile_err")"
        FAIL=$((FAIL + 1))
        return
    fi

    if [ "$actual_line" = "$expected_line" ]; then
        echo "PASS (line $actual_line)"
        PASS=$((PASS + 1))
    else
        echo "FAIL (expected line $expected_line, got line $actual_line)"
        echo "    Compiler error: $(head -1 "$compile_err")"
        FAIL=$((FAIL + 1))
    fi
}

for input_file in "$CASES_DIR"/*.phc; do
    [ -f "$input_file" ] || continue

    base="$(basename "$input_file" .phc)"

    # Direct-mode test
    expected_line_file="$CASES_DIR/${base}.expected_line"
    if [ -f "$expected_line_file" ]; then
        expected_line="$(cat "$expected_line_file" | tr -d '[:space:]')"
        run_test "$input_file" "$expected_line" "direct" "$base"
    fi

    # Pipeline-mode test
    pipeline_line_file="$CASES_DIR/${base}.pipeline_expected_line"
    if [ -f "$pipeline_line_file" ]; then
        expected_line="$(cat "$pipeline_line_file" | tr -d '[:space:]')"
        run_test "$input_file" "$expected_line" "pipeline" "$base"
    fi
done

echo ""
echo "  ========================================"
echo "  Pass: $PASS | Fail: $FAIL | Total: $TOTAL"
echo "  Gate: $([ $FAIL -gt 0 ] && echo 'BLOCKED' || echo 'OPEN')"
echo "  ========================================"

if [ $FAIL -gt 0 ]; then
    exit 1
fi
