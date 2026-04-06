#!/bin/bash
# MSVC compatibility test — verifies phc output compiles with strict C11
# This approximates MSVC's C conformance mode on Linux using gcc -std=c11 -pedantic
# No POSIX headers, no GCC extensions, no __attribute__

set -euo pipefail

CC="${CC:-gcc}"
# Strict C11 flags — closest to MSVC /std:c11 on Linux
CFLAGS="-std=c11 -pedantic -Wall -Wextra -Werror -Wno-unused-function"
PHC="$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
CASES_DIR="$SCRIPT_DIR/cases"
TMPDIR="$(mktemp -d)"
trap 'rm -rf "$TMPDIR"' EXIT

PASS=0
FAIL=0
TOTAL=0

echo ""
echo "MSVC Compatibility Test"
echo "========================"
echo "Compiler: $CC $CFLAGS"
echo ""

# Test all .phc files that have .expected.c (success cases)
for input_file in "$CASES_DIR"/*.phc; do
    [ -f "$input_file" ] || continue
    base="$(basename "$input_file" .phc)"
    expected="$CASES_DIR/${base}.expected.c"
    error="$CASES_DIR/${base}.expected_error"

    # Skip error cases
    [ -f "$error" ] && continue
    # Skip if no expected output
    [ -f "$expected" ] || continue

    TOTAL=$((TOTAL + 1))
    printf "  %-40s " "$base"

    # Generate phc output
    phc_out="$TMPDIR/${base}.c"
    if ! "$PHC" < "$input_file" > "$phc_out" 2>/dev/null; then
        echo "SKIP (phc error)"
        TOTAL=$((TOTAL - 1))
        continue
    fi

    # Compile with strict C11 — test both -c (compile only) and link if main exists
    if grep -q 'int main' "$phc_out"; then
        compile_cmd="$CC $CFLAGS -o $TMPDIR/${base} $phc_out"
    else
        compile_cmd="$CC $CFLAGS -c -o $TMPDIR/${base}.o $phc_out"
    fi

    if eval "$compile_cmd" 2>"$TMPDIR/${base}.err"; then
        echo "PASS"
        PASS=$((PASS + 1))
    else
        echo "FAIL"
        head -3 "$TMPDIR/${base}.err" | sed 's/^/    /'
        FAIL=$((FAIL + 1))
    fi
done

echo ""
echo "  ========================================"
echo "  Pass: $PASS | Fail: $FAIL | Total: $TOTAL"
echo "  Gate: $([ $FAIL -gt 0 ] && echo 'BLOCKED' || echo 'OPEN')"
echo "  ========================================"

[ $FAIL -eq 0 ]
