#!/bin/bash
# Header generation tests for PHC V12
# Verifies that --emit-types generates a companion .phc.h header
# alongside the .phc-types manifest.
#
# Usage: header_gen_test.sh <path-to-phc-binary>

set -euo pipefail

CC="${CC:-clang}"
CFLAGS="-std=c11 -Wall -Wextra -Werror -pedantic -Wno-unused-function"
PHC="$1"
TMPDIR="$(mktemp -d)"
trap 'rm -rf "$TMPDIR"' EXIT

PASS=0
FAIL=0
TOTAL=0

run_test() {
    local name="$1"
    TOTAL=$((TOTAL + 1))
    printf "  %-50s " "$name"
}

pass() { PASS=$((PASS + 1)); echo "PASS"; }
fail() { FAIL=$((FAIL + 1)); echo "FAIL ($1)"; }

# === Test 1: --emit-types generates .phc.h for phc_descr ===
run_test "descr_header_generated"
cat > "$TMPDIR/shapes.phc" <<'EOF'
phc_descr Shape {
    Circle { double radius; },
    Rectangle { double width; double height; }
};
EOF
"$PHC" --emit-types="$TMPDIR/shapes.phc-types" < "$TMPDIR/shapes.phc" > "$TMPDIR/shapes.c" 2>/dev/null
if [ -f "$TMPDIR/shapes.phc.h" ]; then
    pass
else
    fail ".phc.h not generated"
fi

# === Test 2: Generated .phc.h compiles standalone ===
run_test "descr_header_compiles"
if [ -f "$TMPDIR/shapes.phc.h" ]; then
    if $CC $CFLAGS -c "$TMPDIR/shapes.phc.h" -o "$TMPDIR/shapes_h.o" 2>/dev/null; then
        pass
    else
        fail "header does not compile"
    fi
else
    fail ".phc.h missing"
fi

# === Test 3: Generated .phc.h has include guard ===
run_test "descr_header_include_guard"
if [ -f "$TMPDIR/shapes.phc.h" ] && grep -q '#ifndef PHC_' "$TMPDIR/shapes.phc.h"; then
    pass
else
    fail "no include guard"
fi

# === Test 4: Generated .phc.h contains type definitions ===
run_test "descr_header_contains_typedef"
if [ -f "$TMPDIR/shapes.phc.h" ] && grep -q 'typedef struct Shape' "$TMPDIR/shapes.phc.h"; then
    pass
else
    fail "no typedef found"
fi

# === Test 5: --emit-types generates .phc.h for phc_enum ===
run_test "enum_header_generated"
cat > "$TMPDIR/colors.phc" <<'EOF'
phc_enum Color {
    Red,
    Green,
    Blue
};
EOF
"$PHC" --emit-types="$TMPDIR/colors.phc-types" < "$TMPDIR/colors.phc" > "$TMPDIR/colors.c" 2>/dev/null
if [ -f "$TMPDIR/colors.phc.h" ]; then
    pass
else
    fail ".phc.h not generated"
fi

# === Test 6: Enum .phc.h contains to_string ===
run_test "enum_header_contains_to_string"
if [ -f "$TMPDIR/colors.phc.h" ] && grep -q 'Color_to_string' "$TMPDIR/colors.phc.h"; then
    pass
else
    fail "no to_string found"
fi

# === Test 7: --emit-types generates .phc.h for phc_flags ===
run_test "flags_header_generated"
cat > "$TMPDIR/perms.phc" <<'EOF'
phc_flags Permissions {
    Read,
    Write,
    Execute
};
EOF
"$PHC" --emit-types="$TMPDIR/perms.phc-types" < "$TMPDIR/perms.phc" > "$TMPDIR/perms.c" 2>/dev/null
if [ -f "$TMPDIR/perms.phc.h" ]; then
    pass
else
    fail ".phc.h not generated"
fi

# === Test 8: Flags .phc.h contains helpers ===
run_test "flags_header_contains_helpers"
if [ -f "$TMPDIR/perms.phc.h" ] && grep -q 'Permissions_has' "$TMPDIR/perms.phc.h"; then
    pass
else
    fail "no helpers found"
fi

# === Test 9: Cross-file using generated header ===
run_test "cross_file_with_header"
cat > "$TMPDIR/user.phc" <<'EOF'
#include "shapes.phc.h"
#include <stdio.h>

int main(void) {
    Shape s = Shape_mk_Circle(3.14);
    phc_match(Shape, s) {
        case Circle: { printf("circle\n"); } break;
        case Rectangle: { printf("rect\n"); } break;
    }
    return 0;
}
EOF
if [ -f "$TMPDIR/shapes.phc.h" ] && [ -f "$TMPDIR/shapes.phc-types" ]; then
    # Preprocess (expands #include), then run phc with manifest
    if $CC -x c -E -I"$TMPDIR" "$TMPDIR/user.phc" 2>/dev/null | \
       "$PHC" --type-manifest="$TMPDIR/shapes.phc-types" > "$TMPDIR/user.c" 2>/dev/null; then
        if $CC $CFLAGS -Wno-gnu-line-marker -I"$TMPDIR" -o "$TMPDIR/user" "$TMPDIR/user.c" 2>/dev/null; then
            actual=$("$TMPDIR/user" 2>/dev/null)
            if [ "$actual" = "circle" ]; then
                pass
            else
                fail "wrong output: $actual"
            fi
        else
            fail "does not compile"
        fi
    else
        fail "phc failed"
    fi
else
    fail "prerequisite files missing"
fi

# === Test 10: Cross-file enum matching via header + manifest ===
run_test "cross_file_enum_match"
cat > "$TMPDIR/enum_user.phc" <<'EOF'
#include "colors.phc.h"
#include <stdio.h>

int main(void) {
    Color c = Color_Green;
    phc_match(Color, c) {
        case Red: { printf("red\n"); } break;
        case Green: { printf("green\n"); } break;
        case Blue: { printf("blue\n"); } break;
    }
    return 0;
}
EOF
if [ -f "$TMPDIR/colors.phc.h" ] && [ -f "$TMPDIR/colors.phc-types" ]; then
    if $CC -x c -E -I"$TMPDIR" "$TMPDIR/enum_user.phc" 2>/dev/null | \
       "$PHC" --type-manifest="$TMPDIR/colors.phc-types" > "$TMPDIR/enum_user.c" 2>/dev/null; then
        if $CC $CFLAGS -Wno-gnu-line-marker -I"$TMPDIR" -o "$TMPDIR/enum_user" "$TMPDIR/enum_user.c" 2>/dev/null; then
            actual=$("$TMPDIR/enum_user" 2>/dev/null)
            if [ "$actual" = "green" ]; then
                pass
            else
                fail "wrong output: $actual"
            fi
        else
            fail "does not compile"
        fi
    else
        fail "phc failed"
    fi
else
    fail "prerequisite files missing"
fi

# === Test 11: Cross-file flags matching via header + manifest ===
run_test "cross_file_flags_match"
cat > "$TMPDIR/flags_user.c" <<'EOF'
#include "perms.phc.h"
#include <stdio.h>

int main(void) {
    Permissions p = Permissions_Read | Permissions_Write;
    if (Permissions_has(p, Permissions_Read)) printf("can read\n");
    if (Permissions_has(p, Permissions_Execute)) printf("can exec\n");
    char buf[Permissions__MAX_STRING + 1];
    printf("str: %s\n", Permissions_to_string(p, buf, sizeof(buf)));
    return 0;
}
EOF
if [ -f "$TMPDIR/perms.phc.h" ]; then
    if $CC $CFLAGS -Wno-gnu-line-marker -I"$TMPDIR" -o "$TMPDIR/flags_user" "$TMPDIR/flags_user.c" 2>/dev/null; then
        actual=$("$TMPDIR/flags_user" 2>/dev/null)
        expected=$(printf "can read\nstr: Read|Write")
        if [ "$actual" = "$expected" ]; then
            pass
        else
            fail "wrong output: $actual"
        fi
    else
        fail "does not compile"
    fi
else
    fail "prerequisite files missing"
fi

# === Test 12: Double-include (include guard works) ===
run_test "double_include_guard"
cat > "$TMPDIR/double.c" <<'EOF'
#include "shapes.phc.h"
#include "shapes.phc.h"
int main(void) { return 0; }
EOF
if [ -f "$TMPDIR/shapes.phc.h" ]; then
    if $CC $CFLAGS -I"$TMPDIR" -o "$TMPDIR/double" "$TMPDIR/double.c" 2>/dev/null; then
        pass
    else
        fail "double include fails"
    fi
else
    fail ".phc.h missing"
fi

# === Summary ===
echo ""
echo "  ========================================"
echo "  Pass: $PASS | Fail: $FAIL | Total: $TOTAL"
echo "  Gate: $([ $FAIL -gt 0 ] && echo 'BLOCKED' || echo 'OPEN')"
echo "  ========================================"

[ $FAIL -eq 0 ] || exit 1
