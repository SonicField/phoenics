#!/bin/bash
# Standard library extern portability tests for PHC
# Verifies that generated code does not emit bare extern declarations
# for standard library functions (snprintf, strcmp, abort), which conflict
# with platform macro redefinitions (_FORTIFY_SOURCE, musl inlines).
#
# TDD: These tests should FAIL against code with bare externs,
# and PASS after the fix replaces them with guarded #includes.
#
# Usage: stdlib_extern_test.sh <path-to-phc-binary>

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
    printf "  %-50s " "$name"
}

pass() { PASS=$((PASS + 1)); echo "PASS"; }
fail() { FAIL=$((FAIL + 1)); echo "FAIL ($1)"; }

# ============================================================
# Section 1: No bare extern declarations in generated output
# ============================================================

# Two-path strategy (D-1775640339):
# .c path: bare externs for abort/strcmp are CORRECT (pipeline mode needs them,
#          they're not macros on any platform). Only snprintf extern is removed
#          (_FORTIFY_SOURCE redefines it as a macro on macOS).
# .phc.h path: no externs — must use #include (headers are self-contained).

# --- Enum-only: .c has extern strcmp (correct for pipeline) ---
run_test "enum_has_extern_strcmp_c"
cat > "$TESTDIR/colors.phc" <<'EOF'
phc_enum Color { Red, Green, Blue };
EOF
"$PHC" < "$TESTDIR/colors.phc" > "$TESTDIR/colors.c" 2>/dev/null
if grep -q 'extern int strcmp' "$TESTDIR/colors.c"; then
    pass
else
    fail "missing extern strcmp in .c (needed for pipeline)"
fi

# --- Enum-only: .phc.h uses #include, no extern strcmp ---
run_test "enum_header_includes_not_externs"
"$PHC" --emit-types="$TESTDIR/colors.phc-types" < "$TESTDIR/colors.phc" > /dev/null 2>/dev/null
if [ -f "$TESTDIR/colors.phc.h" ]; then
    if grep -q 'extern int strcmp' "$TESTDIR/colors.phc.h"; then
        fail "extern strcmp in .phc.h (should use #include)"
    elif grep -q '#include <string.h>' "$TESTDIR/colors.phc.h"; then
        pass
    else
        fail "no strcmp source in .phc.h"
    fi
else
    fail ".phc.h missing"
fi

# --- Descr-only: .c has extern abort (correct for pipeline) ---
run_test "descr_has_extern_abort_c"
cat > "$TESTDIR/shapes.phc" <<'EOF'
phc_descr Shape {
    Circle { double radius; },
    Rectangle { double width; double height; }
};
EOF
"$PHC" < "$TESTDIR/shapes.phc" > "$TESTDIR/shapes.c" 2>/dev/null
if grep -q 'extern void abort' "$TESTDIR/shapes.c"; then
    pass
else
    fail "missing extern abort in .c (needed for pipeline)"
fi

# --- Descr-only: .phc.h uses #include, no extern abort ---
run_test "descr_header_includes_not_externs"
"$PHC" --emit-types="$TESTDIR/shapes.phc-types" < "$TESTDIR/shapes.phc" > /dev/null 2>/dev/null
if [ -f "$TESTDIR/shapes.phc.h" ]; then
    if grep -q 'extern void abort' "$TESTDIR/shapes.phc.h"; then
        fail "extern abort in .phc.h (should use #include)"
    elif grep -q '#include <stdlib.h>' "$TESTDIR/shapes.phc.h"; then
        pass
    else
        fail "no abort source in .phc.h"
    fi
else
    fail ".phc.h missing"
fi

# --- Flags-only: .c has NO extern snprintf (removed — _FORTIFY_SOURCE conflict) ---
run_test "flags_no_extern_snprintf_c"
cat > "$TESTDIR/perms.phc" <<'EOF'
phc_flags Permissions { Read, Write, Execute };
EOF
"$PHC" < "$TESTDIR/perms.phc" > "$TESTDIR/perms.c" 2>/dev/null
if grep -q 'extern int snprintf' "$TESTDIR/perms.c"; then
    fail "extern snprintf in .c (conflicts with _FORTIFY_SOURCE)"
else
    pass
fi

# --- Flags-only: .phc.h has no snprintf dependency at all ---
run_test "flags_header_no_snprintf_dep"
"$PHC" --emit-types="$TESTDIR/perms.phc-types" < "$TESTDIR/perms.phc" > /dev/null 2>/dev/null
if [ -f "$TESTDIR/perms.phc.h" ]; then
    if grep -q 'snprintf' "$TESTDIR/perms.phc.h"; then
        fail "snprintf reference in .phc.h"
    else
        pass
    fi
else
    fail ".phc.h missing"
fi

# --- Mixed: .c has abort/strcmp externs, no snprintf extern ---
run_test "mixed_c_extern_policy"
cat > "$TESTDIR/mixed.phc" <<'EOF'
phc_descr Shape {
    Circle { double radius; },
    Square { double side; }
};
phc_enum Color { Red, Green, Blue };
phc_flags Mode { Fast, Safe, Debug };
EOF
"$PHC" < "$TESTDIR/mixed.phc" > "$TESTDIR/mixed.c" 2>/dev/null
err=""
grep -q 'extern void abort' "$TESTDIR/mixed.c" || err="missing abort extern; "
grep -q 'extern int strcmp' "$TESTDIR/mixed.c" || err="${err}missing strcmp extern; "
grep -q 'extern int snprintf' "$TESTDIR/mixed.c" && err="${err}snprintf extern present"
if [ -z "$err" ]; then
    pass
else
    fail "$err"
fi

# --- Mixed: .phc.h uses #include for stdlib/string, no externs, no snprintf ---
run_test "mixed_header_includes_not_externs"
"$PHC" --emit-types="$TESTDIR/mixed.phc-types" < "$TESTDIR/mixed.phc" > /dev/null 2>/dev/null
if [ -f "$TESTDIR/mixed.phc.h" ]; then
    if grep -qE 'extern (int strcmp|void abort|int snprintf)' "$TESTDIR/mixed.phc.h"; then
        fail "extern found in .phc.h"
    elif grep -q 'snprintf' "$TESTDIR/mixed.phc.h"; then
        fail "snprintf reference in .phc.h"
    elif grep -q '#include <stdlib.h>' "$TESTDIR/mixed.phc.h" && \
         grep -q '#include <string.h>' "$TESTDIR/mixed.phc.h"; then
        pass
    else
        fail "missing #include in .phc.h"
    fi
else
    fail ".phc.h missing"
fi

# ============================================================
# Section 2: Headers compile standalone
# ============================================================

# --- Enum .phc.h compiles standalone ---
run_test "enum_header_standalone_compiles"
if [ -f "$TESTDIR/colors.phc.h" ]; then
    if $CC $CFLAGS -c "$TESTDIR/colors.phc.h" -o "$TESTDIR/colors_h.o" 2>/dev/null; then
        pass
    else
        fail "enum header does not compile standalone"
    fi
else
    fail ".phc.h missing"
fi

# --- Flags .phc.h compiles standalone ---
run_test "flags_header_standalone_compiles"
if [ -f "$TESTDIR/perms.phc.h" ]; then
    if $CC $CFLAGS -c "$TESTDIR/perms.phc.h" -o "$TESTDIR/perms_h.o" 2>/dev/null; then
        pass
    else
        fail "flags header does not compile standalone"
    fi
else
    fail ".phc.h missing"
fi

# --- Descr .phc.h compiles standalone ---
run_test "descr_header_standalone_compiles"
if [ -f "$TESTDIR/shapes.phc.h" ]; then
    if $CC $CFLAGS -c "$TESTDIR/shapes.phc.h" -o "$TESTDIR/shapes_h.o" 2>/dev/null; then
        pass
    else
        fail "descr header does not compile standalone"
    fi
else
    fail ".phc.h missing"
fi

# --- Mixed .phc.h compiles standalone ---
run_test "mixed_header_standalone_compiles"
if [ -f "$TESTDIR/mixed.phc.h" ]; then
    if $CC $CFLAGS -c "$TESTDIR/mixed.phc.h" -o "$TESTDIR/mixed_h.o" 2>/dev/null; then
        pass
    else
        fail "mixed header does not compile standalone"
    fi
else
    fail ".phc.h missing"
fi

# ============================================================
# Section 3: Generated .c compiles with consumer includes
# ============================================================

# --- Enum .c output compiles with string.h ---
run_test "enum_c_compiles_with_string_h"
cat > "$TESTDIR/enum_consumer.c" <<'EOF'
#include <string.h>
#include <stdio.h>
EOF
cat "$TESTDIR/colors.c" >> "$TESTDIR/enum_consumer.c"
cat >> "$TESTDIR/enum_consumer.c" <<'EOF'
int main(void) {
    Color c = Color_Red;
    printf("%s\n", Color_to_string(c));
    return 0;
}
EOF
if $CC $CFLAGS -Wno-everything -o "$TESTDIR/enum_consumer" "$TESTDIR/enum_consumer.c" 2>/dev/null; then
    pass
else
    fail "does not compile with string.h included"
fi

# --- Flags .c output compiles with stdio.h ---
run_test "flags_c_compiles_with_stdio_h"
cat > "$TESTDIR/flags_consumer.c" <<'EOF'
#include <stdio.h>
EOF
cat "$TESTDIR/perms.c" >> "$TESTDIR/flags_consumer.c"
cat >> "$TESTDIR/flags_consumer.c" <<'EOF'
int main(void) {
    Permissions p = Permissions_Read | Permissions_Write;
    char buf[64];
    printf("%s\n", Permissions_to_string(p, buf, sizeof(buf)));
    return 0;
}
EOF
if $CC $CFLAGS -Wno-everything -o "$TESTDIR/flags_consumer" "$TESTDIR/flags_consumer.c" 2>/dev/null; then
    pass
else
    fail "does not compile with stdio.h included"
fi

# --- Descr .c output compiles with stdlib.h ---
run_test "descr_c_compiles_with_stdlib_h"
cat > "$TESTDIR/descr_consumer.c" <<'EOF'
#include <stdlib.h>
#include <stdio.h>
EOF
cat "$TESTDIR/shapes.c" >> "$TESTDIR/descr_consumer.c"
cat >> "$TESTDIR/descr_consumer.c" <<'EOF'
int main(void) {
    Shape s = Shape_mk_Circle(3.14);
    printf("ok\n");
    return 0;
}
EOF
if $CC $CFLAGS -Wno-everything -o "$TESTDIR/descr_consumer" "$TESTDIR/descr_consumer.c" 2>/dev/null; then
    pass
else
    fail "does not compile with stdlib.h included"
fi

# ============================================================
# Section 4: _FORTIFY_SOURCE compatibility
# ============================================================

# --- Flags .phc.h compiles with _FORTIFY_SOURCE=2 ---
run_test "flags_header_fortify_source"
if [ -f "$TESTDIR/perms.phc.h" ]; then
    cat > "$TESTDIR/fortify_flags.c" <<'EOF'
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
EOF
    cat >> "$TESTDIR/fortify_flags.c" <<INCLUDEEOF
#include "perms.phc.h"
INCLUDEEOF
    cat >> "$TESTDIR/fortify_flags.c" <<'EOF'
int main(void) {
    Permissions p = Permissions_Read;
    char buf[64];
    Permissions_to_string(p, buf, sizeof(buf));
    return 0;
}
EOF
    if $CC -std=c11 -Wall -Wextra -Werror -O2 -D_FORTIFY_SOURCE=2 -Wno-unused-function \
       -I"$TESTDIR" -o "$TESTDIR/fortify_flags" "$TESTDIR/fortify_flags.c" 2>/dev/null; then
        pass
    else
        fail "does not compile with _FORTIFY_SOURCE=2"
    fi
else
    fail ".phc.h missing"
fi

# --- Enum .phc.h compiles with _FORTIFY_SOURCE=2 ---
run_test "enum_header_fortify_source"
if [ -f "$TESTDIR/colors.phc.h" ]; then
    cat > "$TESTDIR/fortify_enum.c" <<'EOF'
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
EOF
    cat >> "$TESTDIR/fortify_enum.c" <<INCLUDEEOF
#include "colors.phc.h"
INCLUDEEOF
    cat >> "$TESTDIR/fortify_enum.c" <<'EOF'
int main(void) {
    Color c = Color_Red;
    printf("%s\n", Color_to_string(c));
    return 0;
}
EOF
    if $CC -std=c11 -Wall -Wextra -Werror -O2 -D_FORTIFY_SOURCE=2 -Wno-unused-function \
       -I"$TESTDIR" -o "$TESTDIR/fortify_enum" "$TESTDIR/fortify_enum.c" 2>/dev/null; then
        pass
    else
        fail "does not compile with _FORTIFY_SOURCE=2"
    fi
else
    fail ".phc.h missing"
fi

# --- Mixed .phc.h compiles with _FORTIFY_SOURCE=2 ---
run_test "mixed_header_fortify_source"
if [ -f "$TESTDIR/mixed.phc.h" ]; then
    cat > "$TESTDIR/fortify_mixed.c" <<'EOF'
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
EOF
    cat >> "$TESTDIR/fortify_mixed.c" <<INCLUDEEOF
#include "mixed.phc.h"
INCLUDEEOF
    cat >> "$TESTDIR/fortify_mixed.c" <<'EOF'
int main(void) {
    Color c = Color_Red;
    printf("%s\n", Color_to_string(c));
    Mode p = Mode_Fast;
    char buf[64];
    Mode_to_string(p, buf, sizeof(buf));
    Shape s = Shape_mk_Circle(1.0);
    (void)s;
    return 0;
}
EOF
    if $CC -std=c11 -Wall -Wextra -Werror -O2 -D_FORTIFY_SOURCE=2 -Wno-unused-function \
       -I"$TESTDIR" -o "$TESTDIR/fortify_mixed" "$TESTDIR/fortify_mixed.c" 2>/dev/null; then
        pass
    else
        fail "does not compile with _FORTIFY_SOURCE=2"
    fi
else
    fail ".phc.h missing"
fi

# ============================================================
# Section 5: Pipeline mode (cc -E | phc | cc)
# ============================================================

# --- Flags in pipeline mode compiles ---
run_test "flags_pipeline_compiles"
cat > "$TESTDIR/pipe_flags.phc" <<'EOF'
#include <stdio.h>
phc_flags Permissions { Read, Write, Execute };
int main(void) {
    Permissions p = Permissions_Read | Permissions_Write;
    char buf[64];
    printf("%s\n", Permissions_to_string(p, buf, sizeof(buf)));
    return 0;
}
EOF
if $CC -x c -E -P "$TESTDIR/pipe_flags.phc" 2>/dev/null | \
   "$PHC" > "$TESTDIR/pipe_flags.c" 2>/dev/null; then
    if $CC -std=c11 -Wno-everything -o "$TESTDIR/pipe_flags" "$TESTDIR/pipe_flags.c" 2>/dev/null; then
        actual=$("$TESTDIR/pipe_flags" 2>/dev/null)
        if [ "$actual" = "Read|Write" ]; then
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

# --- Enum in pipeline mode compiles ---
run_test "enum_pipeline_compiles"
cat > "$TESTDIR/pipe_enum.phc" <<'EOF'
#include <stdio.h>
#include <string.h>
phc_enum Color { Red, Green, Blue };
int main(void) {
    Color c = Color_Green;
    printf("%s\n", Color_to_string(c));
    return 0;
}
EOF
if $CC -x c -E -P "$TESTDIR/pipe_enum.phc" 2>/dev/null | \
   "$PHC" > "$TESTDIR/pipe_enum.c" 2>/dev/null; then
    if $CC -std=c11 -Wno-everything -o "$TESTDIR/pipe_enum" "$TESTDIR/pipe_enum.c" 2>/dev/null; then
        actual=$("$TESTDIR/pipe_enum" 2>/dev/null)
        if [ "$actual" = "Green" ]; then
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

# --- Descr in pipeline mode compiles ---
run_test "descr_pipeline_compiles"
cat > "$TESTDIR/pipe_descr.phc" <<'EOF'
#include <stdio.h>
#include <stdlib.h>
phc_descr Shape {
    Circle { double radius; },
    Square { double side; }
};
int main(void) {
    Shape s = Shape_mk_Circle(2.0);
    phc_match(Shape, s) {
        case Circle: { printf("circle\n"); } break;
        case Square: { printf("square\n"); } break;
    }
    return 0;
}
EOF
if $CC -x c -E -P "$TESTDIR/pipe_descr.phc" 2>/dev/null | \
   "$PHC" > "$TESTDIR/pipe_descr.c" 2>/dev/null; then
    if $CC -std=c11 -Wno-everything -o "$TESTDIR/pipe_descr" "$TESTDIR/pipe_descr.c" 2>/dev/null; then
        actual=$("$TESTDIR/pipe_descr" 2>/dev/null)
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

# ============================================================
# Section 6: Includes emitted once (no duplicates)
# ============================================================

# --- Mixed .c has at most one of each #include ---
run_test "mixed_no_duplicate_includes_c"
"$PHC" < "$TESTDIR/mixed.phc" > "$TESTDIR/mixed.c" 2>/dev/null
stdio_count=$(grep -c '#include <stdio.h>' "$TESTDIR/mixed.c" || true)
string_count=$(grep -c '#include <string.h>' "$TESTDIR/mixed.c" || true)
stdlib_count=$(grep -c '#include <stdlib.h>' "$TESTDIR/mixed.c" || true)
stdio_count=${stdio_count:-0}; string_count=${string_count:-0}; stdlib_count=${stdlib_count:-0}
if [ "$stdio_count" -le 1 ] && [ "$string_count" -le 1 ] && [ "$stdlib_count" -le 1 ]; then
    pass
else
    fail "duplicate includes: stdio=$stdio_count string=$string_count stdlib=$stdlib_count"
fi

# --- Mixed .phc.h has at most one of each #include ---
run_test "mixed_no_duplicate_includes_h"
if [ -f "$TESTDIR/mixed.phc.h" ]; then
    stdio_count=$(grep -c '#include <stdio.h>' "$TESTDIR/mixed.phc.h" || true)
    string_count=$(grep -c '#include <string.h>' "$TESTDIR/mixed.phc.h" || true)
    stdlib_count=$(grep -c '#include <stdlib.h>' "$TESTDIR/mixed.phc.h" || true)
    stdio_count=${stdio_count:-0}; string_count=${string_count:-0}; stdlib_count=${stdlib_count:-0}
    if [ "$stdio_count" -le 1 ] && [ "$string_count" -le 1 ] && [ "$stdlib_count" -le 1 ]; then
        pass
    else
        fail "duplicate includes: stdio=$stdio_count string=$string_count stdlib=$stdlib_count"
    fi
else
    fail ".phc.h missing"
fi

# ============================================================
# Section 7: No snprintf in generated code at all (D-1775640845)
# to_string must use raw char* processing instead
# ============================================================

# --- Flags .c output must not call snprintf ---
run_test "flags_no_snprintf_call_c"
"$PHC" < "$TESTDIR/perms.phc" > "$TESTDIR/perms.c" 2>/dev/null
if grep -q 'snprintf' "$TESTDIR/perms.c"; then
    fail "snprintf call found in .c"
else
    pass
fi

# --- Flags .phc.h must not call snprintf ---
run_test "flags_no_snprintf_call_h"
if [ -f "$TESTDIR/perms.phc.h" ] && grep -q 'snprintf' "$TESTDIR/perms.phc.h"; then
    fail "snprintf found in .phc.h"
else
    pass
fi

# --- Mixed .c output must not call snprintf ---
run_test "mixed_no_snprintf_call_c"
"$PHC" < "$TESTDIR/mixed.phc" > "$TESTDIR/mixed.c" 2>/dev/null
if grep -q 'snprintf' "$TESTDIR/mixed.c"; then
    fail "snprintf call found in .c"
else
    pass
fi

# --- Mixed .phc.h must not call snprintf ---
run_test "mixed_no_snprintf_call_h"
if [ -f "$TESTDIR/mixed.phc.h" ] && grep -q 'snprintf' "$TESTDIR/mixed.phc.h"; then
    fail "snprintf found in .phc.h"
else
    pass
fi

# --- to_string correctness: single flag ---
run_test "flags_tostring_single"
cat > "$TESTDIR/ts_single.phc" <<'EOF'
#include <stdio.h>
phc_flags Perms { Read, Write, Execute };
int main(void) {
    Perms p = Perms_Write;
    char buf[64];
    printf("%s\n", Perms_to_string(p, buf, sizeof(buf)));
    return 0;
}
EOF
"$PHC" < "$TESTDIR/ts_single.phc" > "$TESTDIR/ts_single.c" 2>/dev/null
if $CC $CFLAGS -Wno-everything -o "$TESTDIR/ts_single" "$TESTDIR/ts_single.c" 2>/dev/null; then
    actual=$("$TESTDIR/ts_single" 2>/dev/null)
    if [ "$actual" = "Write" ]; then
        pass
    else
        fail "expected 'Write', got '$actual'"
    fi
else
    fail "does not compile"
fi

# --- to_string correctness: multiple flags with | separator ---
run_test "flags_tostring_multiple"
cat > "$TESTDIR/ts_multi.phc" <<'EOF'
#include <stdio.h>
phc_flags Perms { Read, Write, Execute };
int main(void) {
    Perms p = Perms_Read | Perms_Execute;
    char buf[64];
    printf("%s\n", Perms_to_string(p, buf, sizeof(buf)));
    return 0;
}
EOF
"$PHC" < "$TESTDIR/ts_multi.phc" > "$TESTDIR/ts_multi.c" 2>/dev/null
if $CC $CFLAGS -Wno-everything -o "$TESTDIR/ts_multi" "$TESTDIR/ts_multi.c" 2>/dev/null; then
    actual=$("$TESTDIR/ts_multi" 2>/dev/null)
    if [ "$actual" = "Read|Execute" ]; then
        pass
    else
        fail "expected 'Read|Execute', got '$actual'"
    fi
else
    fail "does not compile"
fi

# --- to_string correctness: zero flags ---
run_test "flags_tostring_none"
cat > "$TESTDIR/ts_none.phc" <<'EOF'
#include <stdio.h>
phc_flags Perms { Read, Write, Execute };
int main(void) {
    Perms p = 0;
    char buf[64];
    printf("%s\n", Perms_to_string(p, buf, sizeof(buf)));
    return 0;
}
EOF
"$PHC" < "$TESTDIR/ts_none.phc" > "$TESTDIR/ts_none.c" 2>/dev/null
if $CC $CFLAGS -Wno-everything -o "$TESTDIR/ts_none" "$TESTDIR/ts_none.c" 2>/dev/null; then
    actual=$("$TESTDIR/ts_none" 2>/dev/null)
    if [ "$actual" = "(none)" ]; then
        pass
    else
        fail "expected '(none)', got '$actual'"
    fi
else
    fail "does not compile"
fi

# --- to_string correctness: all flags ---
run_test "flags_tostring_all"
cat > "$TESTDIR/ts_all.phc" <<'EOF'
#include <stdio.h>
phc_flags Perms { Read, Write, Execute };
int main(void) {
    Perms p = Perms_Read | Perms_Write | Perms_Execute;
    char buf[64];
    printf("%s\n", Perms_to_string(p, buf, sizeof(buf)));
    return 0;
}
EOF
"$PHC" < "$TESTDIR/ts_all.phc" > "$TESTDIR/ts_all.c" 2>/dev/null
if $CC $CFLAGS -Wno-everything -o "$TESTDIR/ts_all" "$TESTDIR/ts_all.c" 2>/dev/null; then
    actual=$("$TESTDIR/ts_all" 2>/dev/null)
    if [ "$actual" = "Read|Write|Execute" ]; then
        pass
    else
        fail "expected 'Read|Write|Execute', got '$actual'"
    fi
else
    fail "does not compile"
fi

# === Summary ===
echo ""
echo "  ========================================"
echo "  Pass: $PASS | Fail: $FAIL | Total: $TOTAL"
echo "  Gate: $([ $FAIL -gt 0 ] && echo 'BLOCKED' || echo 'OPEN')"
echo "  ========================================"

[ $FAIL -eq 0 ] || exit 1
