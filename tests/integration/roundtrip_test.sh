#!/bin/bash
# Passthrough round-trip test for PHC (Phoenics)
#
# Verifies that standard C files pass through phc byte-for-byte unchanged.
# This is architecture invariant #1: phc(standard_c) == standard_c
#
# Usage:
#   roundtrip_test.sh <path-to-phc> <directory-or-file> [...]
#
# Examples:
#   roundtrip_test.sh build/phc vendor/sqlite3.c
#   roundtrip_test.sh build/phc vendor/sqlite/
#   roundtrip_test.sh build/phc /path/to/project/src/ /path/to/project/include/

set -euo pipefail

PHC="${1:?Usage: $0 <path-to-phc> <dir-or-file> [...]}"
shift

if [ $# -eq 0 ]; then
    echo "Usage: $0 <path-to-phc> <dir-or-file> [...]"
    exit 1
fi

PASS=0
FAIL=0
SKIP=0
TOTAL=0
FAIL_FILES=""
TMPFILE=$(mktemp)
trap 'rm -f "$TMPFILE"' EXIT

# Collect all .c and .h files from arguments
FILES=""
for arg in "$@"; do
    if [ -f "$arg" ]; then
        FILES="$FILES $arg"
    elif [ -d "$arg" ]; then
        FILES="$FILES $(find "$arg" -name '*.c' -o -name '*.h' | sort)"
    else
        echo "WARNING: $arg is not a file or directory, skipping"
    fi
done

if [ -z "$FILES" ]; then
    echo "No .c or .h files found"
    exit 1
fi

echo ""
echo "PHC Passthrough Round-Trip Test"
echo "==============================="
echo "Binary: $PHC"
echo ""

for f in $FILES; do
    TOTAL=$((TOTAL + 1))
    fname=$(basename "$f")
    fsize=$(wc -c < "$f" | tr -d ' ')

    printf "  %-50s (%s bytes) " "$fname" "$fsize"

    # Run phc and capture output
    if ! "$PHC" < "$f" > "$TMPFILE" 2>/dev/null; then
        echo "FAIL (phc error)"
        FAIL=$((FAIL + 1))
        FAIL_FILES="$FAIL_FILES $f"
        continue
    fi

    # Byte-for-byte comparison
    if diff -q "$f" "$TMPFILE" > /dev/null 2>&1; then
        echo "PASS"
        PASS=$((PASS + 1))
    else
        echo "FAIL (output differs)"
        # Show first few differences
        diff "$f" "$TMPFILE" | head -20 | sed 's/^/    /'
        FAIL=$((FAIL + 1))
        FAIL_FILES="$FAIL_FILES $f"
    fi
done

echo ""
echo "========================================"
echo "Pass: $PASS | Fail: $FAIL | Skip: $SKIP | Total: $TOTAL"
TOTAL_BYTES=0
for f in $FILES; do
    fsize=$(wc -c < "$f" | tr -d ' ')
    TOTAL_BYTES=$((TOTAL_BYTES + fsize))
done
echo "Total bytes processed: $TOTAL_BYTES"
if [ $FAIL -gt 0 ]; then
    echo "FAILED FILES:"
    for f in $FAIL_FILES; do
        echo "  $f"
    done
fi
echo "Gate: $([ $FAIL -gt 0 ] && echo 'BLOCKED' || echo 'OPEN')"
echo "========================================"

[ $FAIL -eq 0 ]
