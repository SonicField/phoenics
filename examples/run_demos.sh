#!/bin/bash
# Run PHC enforcement demos
set -e

PHC="${1:-build/phc}"
CC="${CC:-clang}"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
TMPDIR="$(mktemp -d)"
trap 'rm -rf "$TMPDIR"' EXIT

echo "=== Demo 1: Successful enforcement (all variants handled) ==="
echo ""
"$PHC" < "$SCRIPT_DIR/enforcement_demo.phc" > "$TMPDIR/demo.c"
$CC -std=c11 -Wall -Wextra -Werror -pedantic -Wno-unused-function -o "$TMPDIR/demo" "$TMPDIR/demo.c"
"$TMPDIR/demo"

echo ""
echo "=== Demo 2: Rejected — non-exhaustive match (Triangle missing) ==="
echo ""
if "$PHC" < "$SCRIPT_DIR/enforcement_error.phc" > "$TMPDIR/error.c" 2>"$TMPDIR/error.txt"; then
    echo "UNEXPECTED: phc accepted the file (should have rejected)"
    exit 1
else
    echo "PHC correctly rejected the file:"
    cat "$TMPDIR/error.txt"
    echo ""
    echo "This is the enforcement in action — you cannot forget a variant."
fi
