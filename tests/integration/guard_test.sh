#!/bin/bash
# Guard tests for PHC (D-1775642218 item 3)
#
# Two concerns:
# 1. #ifndef guards on abort/strcmp externs — verify that when abort or strcmp
#    is defined as a macro (as on some platforms), the guard skips the extern
#    and the generated code still compiles and runs correctly.
# 2. Guard name collision in --emit-types — verify that two .phc files whose
#    names differ only in dash vs underscore do NOT silently produce headers
#    with identical include guards (they currently do — this test documents
#    the known collision).
#
# Usage: guard_test.sh <path-to-phc-binary>

set -euo pipefail

CC="${CC:-clang}"
CFLAGS="-std=c11 -Wall -Wextra -Werror -pedantic -Wno-unused-function"
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

# ============================================================
# Section 1: #ifndef abort guard
# ============================================================

# Test 1: abort as macro — guard skips extern, code compiles
run_test "ifndef_abort_guard_skips_extern"
cat > "$TESTDIR/abort_guard.phc" <<'EOF'
phc_descr Shape {
    Circle { double radius; },
    Square { double side; }
};
EOF
"$PHC" < "$TESTDIR/abort_guard.phc" > "$TESTDIR/abort_guard_raw.c" 2>/dev/null
# Verify the extern is present in raw output (precondition)
if ! grep -q '#ifndef abort' "$TESTDIR/abort_guard_raw.c"; then
    fail "no #ifndef abort in output — precondition violated"
else
    # Wrap: define abort as macro, then include phc output
    cat > "$TESTDIR/abort_guard_test.c" <<WRAPPER
#include <stdio.h>
#include <stdlib.h>
/* Simulate a platform where abort is a macro (e.g. some embedded toolchains) */
static int abort_called = 0;
static void my_abort(void) { abort_called = 1; _Exit(42); }
#undef abort
#define abort() my_abort()
/* Now include the phc output — the #ifndef abort guard should skip the extern */
WRAPPER
    cat "$TESTDIR/abort_guard_raw.c" >> "$TESTDIR/abort_guard_test.c"
    cat >> "$TESTDIR/abort_guard_test.c" <<'MAIN'

int main(void) {
    /* Create a Shape and access the correct variant — should NOT abort */
    Shape s = Shape_mk_Circle(5.0);
    (void)Shape_as_Circle(s);
    printf("correct access OK\n");
    return 0;
}
MAIN
    if $CC $CFLAGS -o "$TESTDIR/abort_guard_test" "$TESTDIR/abort_guard_test.c" 2>"$TESTDIR/abort_compile.err"; then
        if "$TESTDIR/abort_guard_test" 2>/dev/null; then
            pass
        else
            fail "compiled but crashed"
        fi
    else
        fail "compilation error: $(head -1 "$TESTDIR/abort_compile.err")"
    fi
fi

# Test 2: abort macro — wrong variant access calls macro abort, not extern
run_test "ifndef_abort_macro_version_called"
cat > "$TESTDIR/abort_macro_call.c" <<WRAPPER
#include <stdio.h>
#include <stdlib.h>
static void my_abort(void) { fprintf(stderr, "MACRO_ABORT_CALLED\n"); _Exit(42); }
#undef abort
#define abort() my_abort()
WRAPPER
cat "$TESTDIR/abort_guard_raw.c" >> "$TESTDIR/abort_macro_call.c"
cat >> "$TESTDIR/abort_macro_call.c" <<'MAIN'

int main(void) {
    Shape s = Shape_mk_Circle(5.0);
    /* Access WRONG variant — should call abort (the macro version) */
    (void)Shape_as_Square(s);
    /* Should not reach here */
    return 0;
}
MAIN
if $CC $CFLAGS -o "$TESTDIR/abort_macro_call" "$TESTDIR/abort_macro_call.c" 2>/dev/null; then
    if "$TESTDIR/abort_macro_call" 2>"$TESTDIR/abort_macro_stderr" ; then
        fail "should have aborted but didn't"
    elif [ $? -eq 42 ] && grep -q "MACRO_ABORT_CALLED" "$TESTDIR/abort_macro_stderr"; then
        pass
    else
        fail "aborted but not via macro (exit=$?)"
    fi
else
    fail "compilation error"
fi

# ============================================================
# Section 2: #ifndef strcmp guard
# ============================================================

# Test 3: strcmp as macro — guard skips extern, code compiles
run_test "ifndef_strcmp_guard_skips_extern"
cat > "$TESTDIR/strcmp_guard.phc" <<'EOF'
phc_enum Color { Red, Green, Blue };
EOF
"$PHC" < "$TESTDIR/strcmp_guard.phc" > "$TESTDIR/strcmp_guard_raw.c" 2>/dev/null
if ! grep -q '#ifndef strcmp' "$TESTDIR/strcmp_guard_raw.c"; then
    fail "no #ifndef strcmp in output — precondition violated"
else
    cat > "$TESTDIR/strcmp_guard_test.c" <<WRAPPER
#include <stdio.h>
#include <string.h>
/* Simulate a platform where strcmp is a macro (glibc can do this via builtins) */
static int my_strcmp(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}
#undef strcmp
#define strcmp my_strcmp
WRAPPER
    cat "$TESTDIR/strcmp_guard_raw.c" >> "$TESTDIR/strcmp_guard_test.c"
    cat >> "$TESTDIR/strcmp_guard_test.c" <<'MAIN'

int main(void) {
    Color c;
    if (Color_from_string("Green", &c)) {
        printf("%s\n", Color_to_string(c));
    }
    return 0;
}
MAIN
    if $CC $CFLAGS -o "$TESTDIR/strcmp_guard_test" "$TESTDIR/strcmp_guard_test.c" 2>"$TESTDIR/strcmp_compile.err"; then
        output=$("$TESTDIR/strcmp_guard_test" 2>/dev/null)
        if [ "$output" = "Green" ]; then
            pass
        else
            fail "wrong output: $output"
        fi
    else
        fail "compilation error: $(head -1 "$TESTDIR/strcmp_compile.err")"
    fi
fi

# ============================================================
# Section 3: Guard name collision in --emit-types
# ============================================================

# Test 4: dash vs underscore produce distinct guard tokens
# Dash maps to double-underscore: foo-bar -> PHC_FOO__BAR_PHC_H
# Underscore stays single:        foo_bar -> PHC_FOO_BAR_PHC_H
run_test "guard_names_distinct_dash_vs_underscore"
cat > "$TESTDIR/foo-bar.phc" <<'EOF'
phc_descr Alpha {
    A1 { int x; }
};
EOF
cat > "$TESTDIR/foo_bar.phc" <<'EOF'
phc_descr Beta {
    B1 { int y; }
};
EOF
"$PHC" --emit-types="$TESTDIR/foo-bar.phc-types" < "$TESTDIR/foo-bar.phc" > "$TESTDIR/foo-bar.c" 2>/dev/null
"$PHC" --emit-types="$TESTDIR/foo_bar.phc-types" < "$TESTDIR/foo_bar.phc" > "$TESTDIR/foo_bar.c" 2>/dev/null

if [ -f "$TESTDIR/foo-bar.phc.h" ] && [ -f "$TESTDIR/foo_bar.phc.h" ]; then
    guard1=$(grep '#ifndef PHC_' "$TESTDIR/foo-bar.phc.h" | head -1)
    guard2=$(grep '#ifndef PHC_' "$TESTDIR/foo_bar.phc.h" | head -1)
    if [ "$guard1" != "$guard2" ]; then
        pass
    else
        fail "guards still collide: $guard1"
    fi
else
    fail "headers not generated"
fi

# Test 5: both headers can be included in one file (no collision)
run_test "multi_header_inclusion_dash_vs_underscore"
if [ -f "$TESTDIR/foo-bar.phc.h" ] && [ -f "$TESTDIR/foo_bar.phc.h" ]; then
    cat > "$TESTDIR/collision_test.c" <<CFILE
#include "foo-bar.phc.h"
#include "foo_bar.phc.h"
int main(void) {
    Alpha a = Alpha_mk_A1(1);
    Beta b = Beta_mk_B1(2);
    (void)a; (void)b;
    return 0;
}
CFILE
    if $CC $CFLAGS -I"$TESTDIR" -o "$TESTDIR/collision_test" "$TESTDIR/collision_test.c" 2>/dev/null; then
        pass
    else
        fail "multi-header inclusion failed"
    fi
else
    fail "headers not generated"
fi

# ============================================================
# Section 4: Non-identifier bytes — hex-encoding (security + i18n)
# ============================================================

# Test 6: Unicode filename — C11 universal character names in guard
run_test "guard_uses_ucn_for_unicode_filename"
# 测试 is U+6D4B U+8BD5
cat > "$TESTDIR/测试.phc" <<'EOF'
phc_descr Gamma {
    G1 { int z; }
};
EOF
"$PHC" --emit-types="$TESTDIR/测试.phc-types" < "$TESTDIR/测试.phc" > "$TESTDIR/测试.c" 2>/dev/null
if [ -f "$TESTDIR/测试.phc.h" ]; then
    guard=$(grep '#ifndef PHC_' "$TESTDIR/测试.phc.h" | head -1 | awk '{print $2}')
    # Unicode codepoints become \uXXXX universal character names
    if echo "$guard" | grep -q '\\u6D4B\\u8BD5'; then
        # Verify it compiles with C11 (UCN support required)
        cat > "$TESTDIR/unicode_test.c" <<CFILE
#include "测试.phc.h"
int main(void) {
    Gamma g = Gamma_mk_G1(1);
    (void)g;
    return 0;
}
CFILE
        if $CC $CFLAGS -I"$TESTDIR" -o "$TESTDIR/unicode_test" "$TESTDIR/unicode_test.c" 2>/dev/null; then
            pass
        else
            fail "UCN guard but compilation failed"
        fi
    else
        fail "guard missing UCN \\u6D4B\\u8BD5: $guard"
    fi
else
    fail "header not generated"
fi

# Test 7: special chars (semicolon, angle bracket) are hex-encoded
run_test "guard_hex_encodes_special_chars"
# Create a file with semicolon in the name — tests preprocessor injection safety
fname="a;b.phc"
cat > "$TESTDIR/$fname" <<'EOF'
phc_descr Delta {
    D1 { int w; }
};
EOF
"$PHC" --emit-types="$TESTDIR/${fname}-types" < "$TESTDIR/$fname" > "$TESTDIR/${fname%.phc}.c" 2>/dev/null
if [ -f "$TESTDIR/$fname.h" ]; then
    guard=$(grep '#ifndef PHC_' "$TESTDIR/$fname.h" | head -1 | awk '{print $2}')
    # Semicolon 0x3B → X_3B_
    if echo "$guard" | grep -q 'X_3B_'; then
        # Verify it compiles
        cat > "$TESTDIR/special_test.c" <<CFILE
#include "a;b.phc.h"
int main(void) {
    Delta d = Delta_mk_D1(1);
    (void)d;
    return 0;
}
CFILE
        if $CC $CFLAGS -I"$TESTDIR" -o "$TESTDIR/special_test" "$TESTDIR/special_test.c" 2>/dev/null; then
            pass
        else
            fail "valid guard but compilation failed"
        fi
    else
        fail "semicolon not hex-encoded: $guard"
    fi
else
    fail "header not generated"
fi

# Test 8: space in filename is hex-encoded (not collapsed to underscore)
run_test "guard_hex_encodes_space_distinct_from_dot"
cat > "$TESTDIR/a b.phc" <<'EOF'
phc_descr Epsilon {
    E1 { int v; }
};
EOF
cat > "$TESTDIR/a.b.phc" <<'EOF'
phc_descr Zeta {
    Z1 { int u; }
};
EOF
"$PHC" --emit-types="$TESTDIR/a b.phc-types" < "$TESTDIR/a b.phc" > "$TESTDIR/a b.c" 2>/dev/null
"$PHC" --emit-types="$TESTDIR/a.b.phc-types" < "$TESTDIR/a.b.phc" > "$TESTDIR/a.b.c" 2>/dev/null
if [ -f "$TESTDIR/a b.phc.h" ] && [ -f "$TESTDIR/a.b.phc.h" ]; then
    guard_space=$(grep '#ifndef PHC_' "$TESTDIR/a b.phc.h" | head -1 | awk '{print $2}')
    guard_dot=$(grep '#ifndef PHC_' "$TESTDIR/a.b.phc.h" | head -1 | awk '{print $2}')
    if [ "$guard_space" != "$guard_dot" ]; then
        # Space 0x20 → X_20_, dot → _. Distinct.
        pass
    else
        fail "space and dot produced same guard: $guard_space"
    fi
else
    fail "headers not generated"
fi

# ============================================================
# Summary
# ============================================================
echo ""
echo "  $PASS/$TOTAL passed, $FAIL failed"
[ "$FAIL" -eq 0 ] || exit 1
