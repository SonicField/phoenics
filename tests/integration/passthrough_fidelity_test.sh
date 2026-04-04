#!/bin/bash
# Passthrough fidelity tests — byte-for-byte identity for non-Phoenics input
# These test edge cases the main integration runner can't handle
# (empty input, invalid C, etc.)
set -euo pipefail

PHC="$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
TMPDIR="$(mktemp -d)"
trap 'rm -rf "$TMPDIR"' EXIT

PASS=0
FAIL=0
TOTAL=0

check_identity() {
    local name="$1"
    local input_file="$2"
    TOTAL=$((TOTAL + 1))
    printf "  %-40s " "$name"

    actual=$("$PHC" < "$input_file" 2>/dev/null)
    expected=$(cat "$input_file")

    if [ "$actual" = "$expected" ]; then
        echo "PASS"
        PASS=$((PASS + 1))
    else
        echo "FAIL"
        echo "    Input bytes:  $(wc -c < "$input_file")"
        echo "    Output bytes: $(echo -n "$actual" | wc -c)"
        diff <(xxd "$input_file") <(echo -n "$actual" | xxd) | head -10
        FAIL=$((FAIL + 1))
    fi
}

check_identity_inline() {
    local name="$1"
    local content="$2"
    local f="$TMPDIR/${name}.c"
    printf '%s' "$content" > "$f"
    check_identity "$name" "$f"
}

# Empty input
check_identity_inline "empty_input" ""

# Whitespace only
check_identity_inline "whitespace_only" "

"

# No trailing newline
check_identity_inline "no_trailing_newline" "int x = 42;"

# Multiple trailing newlines
check_identity_inline "multiple_trailing_newlines" "int x;


"

# Tab-heavy indentation
check_identity_inline "tab_heavy" "	int	x	=	42	;
"

# Very long line (1000 chars)
long_line=$(printf 'int x = %0998d;' 0)
check_identity_inline "very_long_line" "$long_line
"

# Consecutive blank lines (10)
check_identity_inline "many_blank_lines" "int x;








int y;"

# String containing phc keywords
check_identity_inline "keywords_in_strings" 'const char *a = "phc_descr";
const char *b = "phc_match";
const char *c = "phc_descr Shape { Circle {} };";
'

echo ""
echo "  ========================================"
echo "  Pass: $PASS | Fail: $FAIL | Total: $TOTAL"
echo "  Gate: $([ $FAIL -gt 0 ] && echo 'BLOCKED' || echo 'OPEN')"
echo "  ========================================"

[ $FAIL -eq 0 ]
