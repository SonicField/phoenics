#!/bin/bash
# Multi-file integration tests for PHC v2 type manifests
# Usage: multifile_test.sh <path-to-phc-binary>
#
# Test case directories:
#   NN_name/
#     *.phc              — phc source files (processed in alphabetical order)
#     expected_manifest  — expected .phc-types manifest content (for manifest emission tests)
#     expected_error     — expected error substring (for error tests)
#     run_expected       — expected stdout when compiled output is executed
#
# Test types (detected from files present):
#   1. Manifest emission: has expected_manifest → emit manifest and compare
#   2. Cross-file match: has main.phc + other .phc files → process with manifests
#   3. Error case: has expected_error → expect failure with message

set -euo pipefail

CC="${CC:-clang}"
CFLAGS="-std=c11 -Wall -Wextra -Werror -pedantic -Wno-unused-function"
PHC="$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
CASES_DIR="$SCRIPT_DIR/multifile"
TMPDIR="$(mktemp -d)"
trap 'rm -rf "$TMPDIR"' EXIT

PASS=0
FAIL=0
TOTAL=0

if [ -z "${PHC:-}" ]; then
    echo "Usage: $0 <path-to-phc>"
    exit 1
fi

for case_dir in "$CASES_DIR"/*/; do
    [ -d "$case_dir" ] || continue

    case_name="$(basename "$case_dir")"
    TOTAL=$((TOTAL + 1))
    printf "  %-40s " "$case_name"

    expected_manifest="$case_dir/expected_manifest"
    expected_error="$case_dir/expected_error"
    run_expected="$case_dir/run_expected"
    work_dir="$TMPDIR/$case_name"
    mkdir -p "$work_dir"

    # Copy all phc files and pre-supplied manifests to work dir
    cp "$case_dir"/*.phc "$work_dir/" 2>/dev/null || true
    cp "$case_dir"/*.phc-types "$work_dir/" 2>/dev/null || true

    # --- Test type 1: Manifest emission ---
    # If expected_manifest exists and no main.phc (pure manifest test)
    if [ -f "$expected_manifest" ] && [ ! -f "$case_dir/main.phc" ]; then
        # Find the single .phc file
        phc_file=$(ls "$work_dir"/*.phc | head -1)
        phc_basename="$(basename "$phc_file" .phc)"
        manifest_out="$work_dir/${phc_basename}.phc-types"

        # Run phc with --emit-types
        if ! "$PHC" --emit-types="$manifest_out" < "$phc_file" > "$work_dir/${phc_basename}.c" 2>"$work_dir/stderr"; then
            echo "FAIL (phc returned error)"
            cat "$work_dir/stderr" | sed 's/^/    /' | head -10
            FAIL=$((FAIL + 1))
            continue
        fi

        # Check manifest was emitted
        if [ ! -f "$manifest_out" ]; then
            echo "FAIL (no manifest emitted)"
            echo "    Expected: $manifest_out"
            FAIL=$((FAIL + 1))
            continue
        fi

        # Compare manifest content
        expected_content="$(cat "$expected_manifest")"
        actual_content="$(cat "$manifest_out")"
        if [ "$actual_content" != "$expected_content" ]; then
            echo "FAIL (manifest mismatch)"
            diff <(echo "$actual_content") <(echo "$expected_content") | head -20
            FAIL=$((FAIL + 1))
            continue
        fi

        echo "PASS"
        PASS=$((PASS + 1))
        continue
    fi

    # --- Test type 2: Cross-file match ---
    # Has main.phc and at least one other .phc file
    if [ -f "$case_dir/main.phc" ]; then
        # Step 1: Process non-main .phc files first to generate manifests
        # Skip if manifest already exists (pre-supplied for adversarial tests)
        for phc_file in "$work_dir"/*.phc; do
            [ "$(basename "$phc_file")" = "main.phc" ] && continue
            phc_basename="$(basename "$phc_file" .phc)"
            manifest_out="$work_dir/${phc_basename}.phc-types"
            header_out="$work_dir/${phc_basename}.h"

            # Skip if manifest already pre-supplied
            [ -f "$manifest_out" ] && continue

            if ! "$PHC" --emit-types="$manifest_out" < "$phc_file" > "$header_out" 2>"$work_dir/stderr"; then
                echo "FAIL (phc failed on $phc_basename.phc)"
                cat "$work_dir/stderr" | sed 's/^/    /' | head -10
                FAIL=$((FAIL + 1))
                continue 2
            fi
        done

        # Step 2: Process main.phc (with manifests available in work_dir)
        main_out="$work_dir/main.c"

        # Build --type-manifest flags for all generated manifests
        manifest_flags=""
        for mf in "$work_dir"/*.phc-types; do
            [ -f "$mf" ] && manifest_flags="$manifest_flags --type-manifest=$mf"
        done

        if [ -f "$expected_error" ]; then
            # Error case: phc should reject main.phc
            expected_err="$(cat "$expected_error")"
            if actual_err=$("$PHC" $manifest_flags < "$work_dir/main.phc" 2>&1); then
                echo "FAIL (expected error, got success)"
                FAIL=$((FAIL + 1))
                continue
            fi
            if echo "$actual_err" | grep -qF "$expected_err"; then
                echo "PASS"
                PASS=$((PASS + 1))
            else
                echo "FAIL (wrong error)"
                echo "    Expected containing: $expected_err"
                echo "    Got: $actual_err"
                FAIL=$((FAIL + 1))
            fi
            continue
        fi

        # Success case: phc should process main.phc using manifests
        if ! "$PHC" $manifest_flags < "$work_dir/main.phc" > "$work_dir/main.c" 2>"$work_dir/stderr"; then
            echo "FAIL (phc failed on main.phc)"
            cat "$work_dir/stderr" | sed 's/^/    /' | head -10
            FAIL=$((FAIL + 1))
            continue
        fi

        # Step 3: Compile and optionally run
        if echo "$(cat "$main_out")" | grep -q 'int main'; then
            # Compile with all generated .h files in include path
            compile_cmd="$CC $CFLAGS -I$work_dir -o $work_dir/main $main_out"
        else
            compile_cmd="$CC $CFLAGS -I$work_dir -c -o $work_dir/main.o $main_out"
        fi

        if ! eval "$compile_cmd" 2>"$work_dir/compile_err"; then
            echo "FAIL (output does not compile)"
            sed 's/^/    /' "$work_dir/compile_err" | head -20
            FAIL=$((FAIL + 1))
            continue
        fi

        # Step 4: Runtime check
        if [ -f "$run_expected" ]; then
            run_exp="$(cat "$run_expected")"
            run_act=$("$work_dir/main" 2>/dev/null) || {
                echo "FAIL (compiled binary crashed)"
                FAIL=$((FAIL + 1))
                continue
            }
            if [ "$run_act" != "$run_exp" ]; then
                echo "FAIL (runtime output mismatch)"
                echo "    Expected: $run_exp"
                echo "    Actual:   $run_act"
                FAIL=$((FAIL + 1))
                continue
            fi
        fi

        echo "PASS"
        PASS=$((PASS + 1))
        continue
    fi

    echo "SKIP (unknown test type)"
    TOTAL=$((TOTAL - 1))
done

echo ""
echo "  ========================================"
echo "  Pass: $PASS | Fail: $FAIL | Total: $TOTAL"
echo "  Gate: $([ $FAIL -gt 0 ] && echo 'BLOCKED' || echo 'OPEN') (full_suite: $([ $TOTAL -gt 0 ] && echo 'yes' || echo 'no'))"
echo "  ========================================"

if [ $FAIL -gt 0 ]; then
    exit 1
fi
if [ $TOTAL -eq 0 ]; then
    echo "  WARNING: no test cases found"
    exit 1
fi
