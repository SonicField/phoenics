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

for input_file in "$CASES_DIR"/*.phc; do
    [ -f "$input_file" ] || continue

    base="$(basename "$input_file" .phc)"
    expected_line_file="$CASES_DIR/${base}.expected_line"

    if [ ! -f "$expected_line_file" ]; then
        continue
    fi

    TOTAL=$((TOTAL + 1))
    printf "  %-40s " "$base"

    expected_line="$(cat "$expected_line_file" | tr -d '[:space:]')"

    # Step 1: Run phc
    phc_out="$TMPDIR/${base}.c"
    if ! "$PHC" < "$input_file" > "$phc_out" 2>"$TMPDIR/${base}.phc_err"; then
        echo "FAIL (phc rejected input)"
        cat "$TMPDIR/${base}.phc_err" | sed 's/^/    /'
        FAIL=$((FAIL + 1))
        continue
    fi

    # Step 2: Compile — expect failure (deliberate error in source)
    compile_err="$TMPDIR/${base}.cc_err"
    if $CC $CFLAGS -c -o /dev/null "$phc_out" 2>"$compile_err"; then
        echo "FAIL (compilation succeeded — deliberate error not triggered)"
        FAIL=$((FAIL + 1))
        continue
    fi

    # Step 3: Extract line number from compiler error
    # clang format: "file.c:LINE:COL: error: ..."
    # gcc format:   "file.c:LINE:COL: error: ..."
    actual_line=$(head -1 "$compile_err" | sed -n 's/^[^:]*:\([0-9]*\):.*/\1/p')

    if [ -z "$actual_line" ]; then
        echo "FAIL (could not parse compiler error line number)"
        echo "    Compiler output: $(head -1 "$compile_err")"
        FAIL=$((FAIL + 1))
        continue
    fi

    if [ "$actual_line" = "$expected_line" ]; then
        echo "PASS (line $actual_line)"
        PASS=$((PASS + 1))
    else
        echo "FAIL (expected line $expected_line, got line $actual_line)"
        echo "    Compiler error: $(head -1 "$compile_err")"
        FAIL=$((FAIL + 1))
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
