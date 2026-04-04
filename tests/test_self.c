/*
 * Self-test for the PHC test framework.
 * Verifies that the framework correctly counts pass/fail/skip.
 *
 * This test is unusual: it tests the test harness itself.
 * We cannot use the harness to test itself (circular), so we
 * manually verify the counters after running test functions.
 */

#include "test_framework.h"

/* A test that should pass */
TEST(pass_test) {
    ASSERT_EQ(1 + 1, 2);
    ASSERT_STR_EQ("hello", "hello");
    ASSERT_NOT_NULL("non-null");
}

/* A test that should fail */
TEST(fail_test) {
    ASSERT_EQ(1, 2);
    /* This line must NOT execute — early return on failure */
    ASSERT_EQ(999, 999);
}

int main(void) {
    int errors = 0;

    printf("\n=== Test Framework Self-Test ===\n\n");

    /* Run a passing test */
    RUN_TEST(pass_test);
    if (_tf_pass != 1 || _tf_fail != 0 || _tf_total != 1) {
        printf("  FRAMEWORK BUG: after pass_test, expected pass=1 fail=0 total=1, "
               "got pass=%d fail=%d total=%d\n", _tf_pass, _tf_fail, _tf_total);
        errors++;
    }

    /* Run a failing test */
    RUN_TEST(fail_test);
    if (_tf_pass != 1 || _tf_fail != 1 || _tf_total != 2) {
        printf("  FRAMEWORK BUG: after fail_test, expected pass=1 fail=1 total=2, "
               "got pass=%d fail=%d total=%d\n", _tf_pass, _tf_fail, _tf_total);
        errors++;
    }

    /* Run a skip */
    SKIP_TEST(skipped_test, "intentionally skipped for self-test");
    if (_tf_skip != 1 || _tf_total != 3) {
        printf("  FRAMEWORK BUG: after skip, expected skip=1 total=3, "
               "got skip=%d total=%d\n", _tf_skip, _tf_total);
        errors++;
    }

    printf("\n");
    if (errors == 0) {
        printf("  Test framework self-test: ALL CHECKS PASSED\n");
        printf("  Counters verified: pass=%d fail=%d skip=%d total=%d\n",
               _tf_pass, _tf_fail, _tf_skip, _tf_total);
    } else {
        printf("  Test framework self-test: %d FRAMEWORK BUGS DETECTED\n", errors);
    }
    printf("\n");

    /* Self-test passes if framework accounting is correct.
     * The fail_test failure is expected — we're testing the framework, not the test. */
    return errors > 0 ? 1 : 0;
}
