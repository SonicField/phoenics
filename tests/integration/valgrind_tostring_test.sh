#!/bin/bash
# Valgrind coverage for flags to_string code paths (D-1775642218 item 2)
#
# The flags to_string function uses raw pointer arithmetic:
#   char *pos = buf; char *end = buf + len - 1;
#   while (*s && pos < end) *pos++ = *s++;
#   *pos = '\0';
#
# This code class is where valgrind catches off-by-one errors, uninitialised
# reads, and buffer overflows that ASan may miss. These tests exercise every
# boundary condition under valgrind:
#   - len==0 (no-op, must not write anything)
#   - len==1 (only null terminator fits)
#   - exact __MAX_STRING (all flags set, buffer exactly fits)
#   - truncation (buffer smaller than needed)
#   - single flag, multiple flags, no flags, all flags
#   - enum to_string (simple switch path, for completeness)
#
# Usage: valgrind_tostring_test.sh <path-to-phc-binary>

set -euo pipefail

CC="${CC:-clang}"
CFLAGS="-std=c11 -Wall -Wextra -Werror -pedantic -g -Wno-unused-function"
PHC="$1"
TESTDIR="$(mktemp -d)"
trap 'rm -rf "$TESTDIR"' EXIT

PASS=0
FAIL=0
TOTAL=0

run_test() {
    local name="$1"
    TOTAL=$((TOTAL + 1))
    printf "  %-55s " "$name"
}

pass() { PASS=$((PASS + 1)); echo "PASS"; }
fail() { FAIL=$((FAIL + 1)); echo "FAIL ($1)"; }

VALGRIND="valgrind --leak-check=full --errors-for-leak-kinds=all --track-origins=yes --error-exitcode=42 --quiet"

# ============================================================
# Generate the phc output once, reuse across tests
# ============================================================

cat > "$TESTDIR/flags.phc" <<'EOF'
phc_flags Permissions { Read, Write, Execute };
EOF
"$PHC" < "$TESTDIR/flags.phc" > "$TESTDIR/flags_generated.c" 2>/dev/null

# Also generate a flags type with many values to stress __MAX_STRING
cat > "$TESTDIR/bigflags.phc" <<'EOF'
phc_flags Features { Alpha, Beta, Gamma, Delta, Epsilon, Zeta, Eta, Theta };
EOF
"$PHC" < "$TESTDIR/bigflags.phc" > "$TESTDIR/bigflags_generated.c" 2>/dev/null

# Generate enum for the enum to_string path
cat > "$TESTDIR/enums.phc" <<'EOF'
phc_enum Color { Red, Green, Blue };
EOF
"$PHC" < "$TESTDIR/enums.phc" > "$TESTDIR/enums_generated.c" 2>/dev/null

# ============================================================
# Helper: compile and valgrind a test harness
# ============================================================

compile_and_valgrind() {
    local name="$1"
    local src="$2"

    echo "$src" > "$TESTDIR/${name}.c"
    if ! $CC $CFLAGS -o "$TESTDIR/${name}" "$TESTDIR/${name}.c" 2>"$TESTDIR/${name}_cc.err"; then
        fail "compile error: $(cat "$TESTDIR/${name}_cc.err" | head -3)"
        return 1
    fi

    if ! $VALGRIND "$TESTDIR/${name}" > "$TESTDIR/${name}.out" 2>"$TESTDIR/${name}_vg.err"; then
        fail "valgrind error: $(cat "$TESTDIR/${name}_vg.err" | head -5)"
        return 1
    fi
    return 0
}

# ============================================================
# Section 1: Flags to_string boundary conditions
# ============================================================

# --- len==0: must be a no-op, no writes at all ---
run_test "flags_tostring_len0_noop"
HARNESS="$(cat "$TESTDIR/flags_generated.c")
#include <string.h>
int main(void) {
    Permissions p = Permissions_Read;
    char buf[4] = {'X','X','X','X'};
    const char *ret = Permissions_to_string(p, buf, 0);
    /* len==0 returns buf without writing — buf should be untouched */
    if (ret != buf) return 1;
    if (buf[0] != 'X') return 2;
    return 0;
}"
compile_and_valgrind "len0" "$HARNESS" && pass

# --- len==1: only null terminator fits, result is empty string ---
run_test "flags_tostring_len1_null_only"
HARNESS="$(cat "$TESTDIR/flags_generated.c")
#include <string.h>
int main(void) {
    Permissions p = Permissions_Read | Permissions_Write | Permissions_Execute;
    char buf[1];
    Permissions_to_string(p, buf, 1);
    /* Only null terminator should fit */
    if (buf[0] != '\0') return 1;
    return 0;
}"
compile_and_valgrind "len1" "$HARNESS" && pass

# --- len==2: one character plus null ---
run_test "flags_tostring_len2_one_char"
HARNESS="$(cat "$TESTDIR/flags_generated.c")
#include <string.h>
int main(void) {
    Permissions p = Permissions_Read;
    char buf[2];
    Permissions_to_string(p, buf, 2);
    /* Should get 'R' + null */
    if (buf[0] != 'R') return 1;
    if (buf[1] != '\0') return 2;
    return 0;
}"
compile_and_valgrind "len2" "$HARNESS" && pass

# --- exact __MAX_STRING: buffer exactly fits all flags ---
run_test "flags_tostring_exact_max_string"
HARNESS="$(cat "$TESTDIR/flags_generated.c")
#include <stdio.h>
#include <string.h>
int main(void) {
    Permissions p = Permissions__ALL;
    /* __MAX_STRING is 19 for Read|Write|Execute (18 chars + null) */
    char buf[Permissions__MAX_STRING];
    Permissions_to_string(p, buf, sizeof(buf));
    if (strcmp(buf, \"Read|Write|Execute\") != 0) {
        fprintf(stderr, \"expected 'Read|Write|Execute', got '%s'\n\", buf);
        return 1;
    }
    return 0;
}"
compile_and_valgrind "exact_max" "$HARNESS" && pass

# --- one byte short of __MAX_STRING: truncation ---
run_test "flags_tostring_truncation_max_minus_1"
HARNESS="$(cat "$TESTDIR/flags_generated.c")
#include <stdio.h>
#include <string.h>
int main(void) {
    Permissions p = Permissions__ALL;
    char buf[Permissions__MAX_STRING - 1];
    Permissions_to_string(p, buf, sizeof(buf));
    /* Must be null-terminated and shorter than full string */
    size_t len = strlen(buf);
    if (len >= Permissions__MAX_STRING - 1) return 1;
    if (buf[len] != '\0') return 2;
    return 0;
}"
compile_and_valgrind "trunc_max_minus1" "$HARNESS" && pass

# --- no flags set: should produce \"(none)\" ---
run_test "flags_tostring_zero_none"
HARNESS="$(cat "$TESTDIR/flags_generated.c")
#include <string.h>
int main(void) {
    Permissions p = 0;
    char buf[64];
    Permissions_to_string(p, buf, sizeof(buf));
    if (strcmp(buf, \"(none)\") != 0) return 1;
    return 0;
}"
compile_and_valgrind "zero_none" "$HARNESS" && pass

# --- no flags, len==1: \"(none)\" truncated to empty ---
run_test "flags_tostring_zero_len1"
HARNESS="$(cat "$TESTDIR/flags_generated.c")
#include <string.h>
int main(void) {
    Permissions p = 0;
    char buf[1];
    Permissions_to_string(p, buf, 1);
    if (buf[0] != '\0') return 1;
    return 0;
}"
compile_and_valgrind "zero_len1" "$HARNESS" && pass

# --- single flag, large buffer ---
run_test "flags_tostring_single_flag"
HARNESS="$(cat "$TESTDIR/flags_generated.c")
#include <string.h>
int main(void) {
    char buf[64];
    Permissions_to_string(Permissions_Read, buf, sizeof(buf));
    if (strcmp(buf, \"Read\") != 0) return 1;
    Permissions_to_string(Permissions_Write, buf, sizeof(buf));
    if (strcmp(buf, \"Write\") != 0) return 2;
    Permissions_to_string(Permissions_Execute, buf, sizeof(buf));
    if (strcmp(buf, \"Execute\") != 0) return 3;
    return 0;
}"
compile_and_valgrind "single_flag" "$HARNESS" && pass

# --- two flags with separator ---
run_test "flags_tostring_two_flags"
HARNESS="$(cat "$TESTDIR/flags_generated.c")
#include <string.h>
int main(void) {
    char buf[64];
    Permissions_to_string(Permissions_Read | Permissions_Execute, buf, sizeof(buf));
    if (strcmp(buf, \"Read|Execute\") != 0) return 1;
    return 0;
}"
compile_and_valgrind "two_flags" "$HARNESS" && pass

# ============================================================
# Section 2: Big flags type (more values, longer __MAX_STRING)
# ============================================================

# --- big flags: all set, exact buffer ---
run_test "bigflags_tostring_all_exact"
HARNESS="$(cat "$TESTDIR/bigflags_generated.c")
#include <stdio.h>
#include <string.h>
int main(void) {
    Features f = Features__ALL;
    char buf[Features__MAX_STRING];
    Features_to_string(f, buf, sizeof(buf));
    /* Verify all names present and pipe-separated */
    if (strstr(buf, \"Alpha\") == NULL) return 1;
    if (strstr(buf, \"Theta\") == NULL) return 2;
    /* Must be null-terminated within buffer */
    if (strlen(buf) >= Features__MAX_STRING) return 3;
    return 0;
}"
compile_and_valgrind "big_all_exact" "$HARNESS" && pass

# --- big flags: truncation at various sizes ---
run_test "bigflags_tostring_truncation_sweep"
HARNESS="$(cat "$TESTDIR/bigflags_generated.c")
#include <string.h>
int main(void) {
    Features f = Features__ALL;
    /* Sweep buffer sizes from 1 to __MAX_STRING */
    for (unsigned long sz = 1; sz <= Features__MAX_STRING; sz++) {
        char buf[256];  /* oversized backing, use sz as len param */
        Features_to_string(f, buf, sz);
        /* Must always be null-terminated within sz bytes */
        int found_null = 0;
        for (unsigned long i = 0; i < sz; i++) {
            if (buf[i] == '\0') { found_null = 1; break; }
        }
        if (!found_null) return 1;
    }
    return 0;
}"
compile_and_valgrind "big_trunc_sweep" "$HARNESS" && pass

# --- big flags: each flag individually ---
run_test "bigflags_tostring_each_individual"
HARNESS="$(cat "$TESTDIR/bigflags_generated.c")
#include <string.h>
int main(void) {
    char buf[64];
    const char *names[] = {\"Alpha\",\"Beta\",\"Gamma\",\"Delta\",\"Epsilon\",\"Zeta\",\"Eta\",\"Theta\"};
    Features flags[] = {Features_Alpha, Features_Beta, Features_Gamma, Features_Delta,
                        Features_Epsilon, Features_Zeta, Features_Eta, Features_Theta};
    for (int i = 0; i < 8; i++) {
        Features_to_string(flags[i], buf, sizeof(buf));
        if (strcmp(buf, names[i]) != 0) return i + 1;
    }
    return 0;
}"
compile_and_valgrind "big_individual" "$HARNESS" && pass

# ============================================================
# Section 3: Enum to_string under valgrind (simple path)
# ============================================================

run_test "enum_tostring_valgrind"
HARNESS="$(cat "$TESTDIR/enums_generated.c")
#include <string.h>
int main(void) {
    if (strcmp(Color_to_string(Color_Red), \"Red\") != 0) return 1;
    if (strcmp(Color_to_string(Color_Green), \"Green\") != 0) return 2;
    if (strcmp(Color_to_string(Color_Blue), \"Blue\") != 0) return 3;
    /* out-of-range value */
    if (strcmp(Color_to_string((Color)99), \"(unknown)\") != 0) return 4;
    return 0;
}"
compile_and_valgrind "enum_ts" "$HARNESS" && pass

# ============================================================
# Section 4: Valgrind on phc binary itself processing flags
# ============================================================

run_test "phc_binary_valgrind_flags_codegen"
if ! $VALGRIND "$PHC" < "$TESTDIR/flags.phc" > /dev/null 2>"$TESTDIR/phc_vg.err"; then
    fail "valgrind error on phc: $(cat "$TESTDIR/phc_vg.err" | head -5)"
else
    pass
fi

run_test "phc_binary_valgrind_bigflags_codegen"
if ! $VALGRIND "$PHC" < "$TESTDIR/bigflags.phc" > /dev/null 2>"$TESTDIR/phc_big_vg.err"; then
    fail "valgrind error on phc: $(cat "$TESTDIR/phc_big_vg.err" | head -5)"
else
    pass
fi

run_test "phc_binary_valgrind_mixed_codegen"
cat > "$TESTDIR/mixed.phc" <<'EOF'
phc_descr Shape {
    Circle { double radius; },
    Square { double side; }
};
phc_enum Color { Red, Green, Blue };
phc_flags Mode { Fast, Safe, Debug };
EOF
if ! $VALGRIND "$PHC" < "$TESTDIR/mixed.phc" > /dev/null 2>"$TESTDIR/phc_mixed_vg.err"; then
    fail "valgrind error on phc: $(cat "$TESTDIR/phc_mixed_vg.err" | head -5)"
else
    pass
fi

# === Summary ===
echo ""
echo "  ========================================"
echo "  Pass: $PASS | Fail: $FAIL | Total: $TOTAL"
echo "  Gate: $([ $FAIL -gt 0 ] && echo 'BLOCKED' || echo 'OPEN')"
echo "  ========================================"

[ $FAIL -eq 0 ] || exit 1
