#!/bin/bash
# phc_assert TDD tests — V13
# Tests for phc_require, phc_check, phc_invariant
# with compile-time stripping via --strip-check and --strip-invariant.
#
# Usage: assert_test.sh <path-to-phc-binary>

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
# Section 1: Basic code generation
# ============================================================

# --- phc_require generates if/abort ---
run_test "require_generates_abort"
cat > "$TESTDIR/pre.phc" <<'EOF'
void check(int x) {
    phc_require(x > 0, "x must be positive");
}
EOF
"$PHC" < "$TESTDIR/pre.phc" > "$TESTDIR/pre.c" 2>/dev/null
if grep -q 'if (!(x > 0)) abort()' "$TESTDIR/pre.c"; then
    pass
else
    fail "expected if/abort pattern"
fi

# --- phc_check generates if/abort ---
run_test "check_generates_abort"
cat > "$TESTDIR/post.phc" <<'EOF'
void check(int x) {
    phc_check(x > 0, "x must be positive");
}
EOF
"$PHC" < "$TESTDIR/post.phc" > "$TESTDIR/post.c" 2>/dev/null
if grep -q 'if (!(x > 0)) abort()' "$TESTDIR/post.c"; then
    pass
else
    fail "expected if/abort pattern"
fi

# --- phc_invariant generates if/abort ---
run_test "invariant_generates_abort"
cat > "$TESTDIR/inv.phc" <<'EOF'
void check(int x) {
    phc_invariant(x > 0, "x must be positive");
}
EOF
"$PHC" < "$TESTDIR/inv.phc" > "$TESTDIR/inv.c" 2>/dev/null
if grep -q 'if (!(x > 0)) abort()' "$TESTDIR/inv.c"; then
    pass
else
    fail "expected if/abort pattern"
fi

# --- message preserved as comment (not as raw phc_require text) ---
run_test "message_in_comment"
# The message should appear as a C comment, not as phc_require(...) passthrough
if grep -q '/\*.*x must be positive' "$TESTDIR/pre.c"; then
    pass
elif grep -q '// x must be positive' "$TESTDIR/pre.c"; then
    pass
else
    fail "message not in comment"
fi

# ============================================================
# Section 2: Compile-time stripping
# ============================================================

# --- --strip-check removes check ---
run_test "strip_check_removes_check"
cat > "$TESTDIR/strip_post.phc" <<'EOF'
void check(int x) {
    phc_require(x > 0, "pre");
    phc_check(x < 100, "post");
    phc_invariant(x != 42, "inv");
}
EOF
"$PHC" --strip-check < "$TESTDIR/strip_post.phc" > "$TESTDIR/strip_post.c" 2>/dev/null || true
# check should be physically absent
if grep -q 'x < 100' "$TESTDIR/strip_post.c"; then
    fail "check not stripped"
else
    pass
fi

# --- --strip-check keeps require and invariant ---
run_test "strip_check_keeps_require_and_inv"
pre_present=0; inv_present=0
grep -q 'x > 0' "$TESTDIR/strip_post.c" && pre_present=1
grep -q 'x != 42' "$TESTDIR/strip_post.c" && inv_present=1
if [ "$pre_present" -eq 1 ] && [ "$inv_present" -eq 1 ]; then
    pass
else
    fail "pre=$pre_present inv=$inv_present"
fi

# --- --strip-invariant removes invariant ---
run_test "strip_invariant_removes_invariant"
"$PHC" --strip-invariant < "$TESTDIR/strip_post.phc" > "$TESTDIR/strip_inv.c" 2>/dev/null || true
if grep -q 'x != 42' "$TESTDIR/strip_inv.c"; then
    fail "invariant not stripped"
else
    pass
fi

# --- --strip-invariant keeps require and check ---
run_test "strip_invariant_keeps_pre_and_post"
pre_present=0; post_present=0
grep -q 'x > 0' "$TESTDIR/strip_inv.c" && pre_present=1
grep -q 'x < 100' "$TESTDIR/strip_inv.c" && post_present=1
if [ "$pre_present" -eq 1 ] && [ "$post_present" -eq 1 ]; then
    pass
else
    fail "pre=$pre_present post=$post_present"
fi

# --- both strip flags remove post + invariant, keep require ---
run_test "both_strips_keep_require"
"$PHC" --strip-check --strip-invariant < "$TESTDIR/strip_post.phc" > "$TESTDIR/strip_both.c" 2>/dev/null || true
pre_present=0; post_absent=1; inv_absent=1
grep -q 'x > 0' "$TESTDIR/strip_both.c" && pre_present=1
grep -q 'x < 100' "$TESTDIR/strip_both.c" && post_absent=0
grep -q 'x != 42' "$TESTDIR/strip_both.c" && inv_absent=0
if [ "$pre_present" -eq 1 ] && [ "$post_absent" -eq 1 ] && [ "$inv_absent" -eq 1 ]; then
    pass
else
    fail "pre=$pre_present post_absent=$post_absent inv_absent=$inv_absent"
fi

# --- require NEVER stripped (even with both flags) ---
run_test "require_never_stripped"
cat > "$TESTDIR/pre_only.phc" <<'EOF'
void check(int x) {
    phc_require(x > 0, "must be positive");
}
EOF
"$PHC" --strip-check --strip-invariant < "$TESTDIR/pre_only.phc" > "$TESTDIR/pre_only.c" 2>/dev/null || true
if grep -q 'if (!(x > 0)) abort()' "$TESTDIR/pre_only.c"; then
    pass
else
    fail "require was stripped"
fi

# --- stripped check leaves comment ---
run_test "stripped_check_leaves_comment"
"$PHC" --strip-check < "$TESTDIR/strip_post.phc" > "$TESTDIR/strip_comment.c" 2>/dev/null || true
if grep -q 'stripped' "$TESTDIR/strip_comment.c"; then
    pass
else
    fail "no stripped comment in output"
fi

# --- stripped invariant leaves comment ---
run_test "stripped_invariant_leaves_comment"
"$PHC" --strip-invariant < "$TESTDIR/strip_post.phc" > "$TESTDIR/strip_inv_comment.c" 2>/dev/null || true
if grep -q 'stripped' "$TESTDIR/strip_inv_comment.c"; then
    pass
else
    fail "no stripped comment in output"
fi

# --- stripped comment does NOT leak expression or message ---
run_test "stripped_comment_no_expr_leak"
cat > "$TESTDIR/strip_leak.phc" <<'EOF'
void f(int x) {
    phc_check(x > 42, "secret check msg");
    phc_invariant(x != 0, "nonzero guard msg");
}
EOF
"$PHC" --strip-check --strip-invariant < "$TESTDIR/strip_leak.phc" > "$TESTDIR/strip_leak.c" 2>/dev/null || true
# Messages and assertion-specific expressions must not appear in stripped output
if grep -q 'secret check msg' "$TESTDIR/strip_leak.c" || grep -q 'nonzero guard msg' "$TESTDIR/strip_leak.c" || grep -q 'x > 42' "$TESTDIR/strip_leak.c" || grep -q 'x != 0' "$TESTDIR/strip_leak.c"; then
    fail "assertion expression or message leaked into stripped output"
else
    pass
fi

# ============================================================
# Section 3: Multiple assertions in one function
# ============================================================

run_test "multiple_assertions_one_function"
cat > "$TESTDIR/multi.phc" <<'EOF'
void validate(int x, int y) {
    phc_require(x > 0, "x positive");
    phc_require(y > 0, "y positive");
    phc_invariant(x + y < 1000, "sum bounded");
    phc_check(x * y > 0, "product positive");
}
EOF
"$PHC" < "$TESTDIR/multi.phc" > "$TESTDIR/multi.c" 2>/dev/null
count=$(grep -c 'if (!(' "$TESTDIR/multi.c" || true)
count=${count:-0}
if [ "$count" -eq 4 ]; then
    pass
else
    fail "expected 4 assertions, got $count"
fi

# ============================================================
# Section 4: Compile and run — assertion passes (no abort)
# ============================================================

run_test "assertion_passes_no_abort"
cat > "$TESTDIR/run_pass.phc" <<'EOF'
#include <stdlib.h>
int main(void) {
    int x = 5;
    phc_require(x > 0, "x must be positive");
    phc_check(x < 100, "x must be small");
    phc_invariant(x == 5, "x must be five");
    return 0;
}
EOF
"$PHC" < "$TESTDIR/run_pass.phc" > "$TESTDIR/run_pass.c" 2>/dev/null
if $CC $CFLAGS -Wno-everything -o "$TESTDIR/run_pass" "$TESTDIR/run_pass.c" 2>/dev/null; then
    if "$TESTDIR/run_pass" 2>/dev/null; then
        pass
    else
        fail "program aborted unexpectedly"
    fi
else
    fail "does not compile"
fi

# ============================================================
# Section 5: Compile and run — assertion fails (abort caught)
# ============================================================

run_test "assertion_fails_aborts"
cat > "$TESTDIR/run_fail.phc" <<'EOF'
#include <stdlib.h>
int main(void) {
    int x = -1;
    phc_require(x > 0, "x must be positive");
    return 0;
}
EOF
"$PHC" < "$TESTDIR/run_fail.phc" > "$TESTDIR/run_fail.c" 2>/dev/null
if $CC $CFLAGS -Wno-everything -o "$TESTDIR/run_fail" "$TESTDIR/run_fail.c" 2>/dev/null; then
    # Should abort (non-zero exit, signal)
    if "$TESTDIR/run_fail" 2>/dev/null; then
        fail "program did not abort on failed require"
    else
        pass
    fi
else
    fail "does not compile"
fi

# ============================================================
# Section 6: Pipeline mode (cc -E | phc | cc)
# ============================================================

run_test "assert_pipeline_mode"
cat > "$TESTDIR/pipe_assert.phc" <<'EOF'
#include <stdlib.h>
int main(void) {
    int x = 10;
    phc_require(x > 0, "x positive");
    return 0;
}
EOF
if $CC -x c -E -P "$TESTDIR/pipe_assert.phc" 2>/dev/null | \
   "$PHC" > "$TESTDIR/pipe_assert.c" 2>/dev/null; then
    if $CC -std=c11 -Wno-everything -o "$TESTDIR/pipe_assert" "$TESTDIR/pipe_assert.c" 2>/dev/null; then
        if "$TESTDIR/pipe_assert" 2>/dev/null; then
            pass
        else
            fail "aborted unexpectedly in pipeline"
        fi
    else
        fail "does not compile"
    fi
else
    fail "phc failed"
fi

# ============================================================
# Section 7: Stripping in pipeline mode
# ============================================================

run_test "strip_in_pipeline_mode"
cat > "$TESTDIR/pipe_strip.phc" <<'EOF'
#include <stdlib.h>
int main(void) {
    int x = -1;
    phc_check(x > 0, "will fail if not stripped");
    return 0;
}
EOF
if $CC -x c -E -P "$TESTDIR/pipe_strip.phc" 2>/dev/null | \
   "$PHC" --strip-check > "$TESTDIR/pipe_strip.c" 2>/dev/null; then
    if $CC -std=c11 -Wno-everything -o "$TESTDIR/pipe_strip" "$TESTDIR/pipe_strip.c" 2>/dev/null; then
        # Should NOT abort — check was stripped
        if "$TESTDIR/pipe_strip" 2>/dev/null; then
            pass
        else
            fail "aborted despite stripping"
        fi
    else
        fail "does not compile"
    fi
else
    fail "phc failed"
fi

# ============================================================
# Section 8: Passthrough — assertions don't affect other code
# ============================================================

run_test "assertions_preserve_surrounding_code"
cat > "$TESTDIR/passthrough.phc" <<'EOF'
int add(int a, int b) {
    phc_require(a >= 0, "a non-negative");
    int result = a + b;
    phc_check(result >= b, "no overflow");
    return result;
}
EOF
"$PHC" < "$TESTDIR/passthrough.phc" > "$TESTDIR/passthrough.c" 2>/dev/null
# Verify surrounding code is preserved
if grep -q 'int result = a + b;' "$TESTDIR/passthrough.c" && \
   grep -q 'return result;' "$TESTDIR/passthrough.c"; then
    pass
else
    fail "surrounding code not preserved"
fi

# === Summary ===
echo ""
echo "  ========================================"
echo "  Pass: $PASS | Fail: $FAIL | Total: $TOTAL"
echo "  Gate: $([ $FAIL -gt 0 ] && echo 'BLOCKED' || echo 'OPEN')"
echo "  ========================================"

[ $FAIL -eq 0 ] || exit 1
