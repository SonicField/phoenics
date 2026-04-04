/*
 * PHC Test Framework
 *
 * Minimal, dependency-free test harness for C.
 * Provides assertion macros, test registration, and summary reporting.
 *
 * Usage:
 *   #include "test_framework.h"
 *
 *   TEST(my_test) {
 *       ASSERT_EQ(1 + 1, 2);
 *       ASSERT_STR_EQ("hello", "hello");
 *   }
 *
 *   TEST_MAIN(
 *       RUN_TEST(my_test);
 *   )
 */

#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int _tf_pass = 0;
static int _tf_fail = 0;
static int _tf_skip = 0;
static int _tf_total = 0;
static int _tf_current_failed = 0;

/*
 * TEST defines a test function.
 * RUN_TEST executes it and tracks pass/fail correctly.
 *
 * The previous version had a critical bug: on ASSERT failure, the test
 * function returned early via `return`, but the wrapper still executed
 * `_tf_pass++` and printed "PASS". A failing test was counted as both
 * passed AND failed. The _tf_total manipulation was also broken.
 *
 * This version uses _tf_current_failed as a flag set by ASSERT macros.
 * The wrapper checks the flag after the test function returns.
 */
#define TEST(name) \
    static void test_##name(void)

#define RUN_TEST(name) \
    do { \
        _tf_total++; \
        _tf_current_failed = 0; \
        printf("  %-50s ", #name); \
        test_##name(); \
        if (_tf_current_failed) { \
            _tf_fail++; \
            printf("FAIL\n"); \
        } else { \
            _tf_pass++; \
            printf("PASS\n"); \
        } \
    } while (0)

#define SKIP_TEST(name, reason) \
    do { \
        _tf_total++; \
        _tf_skip++; \
        printf("  %-50s SKIP (%s)\n", #name, reason); \
    } while (0)

#define ASSERT(cond) \
    do { \
        if (!(cond)) { \
            printf("\n    ASSERTION FAILED: %s\n" \
                   "    at %s:%d\n", #cond, __FILE__, __LINE__); \
            _tf_current_failed = 1; \
            return; \
        } \
    } while(0)

#define ASSERT_MSG(cond, fmt, ...) \
    do { \
        if (!(cond)) { \
            printf("\n    ASSERTION FAILED: %s\n" \
                   "    " fmt "\n" \
                   "    at %s:%d\n", \
                   #cond, __VA_ARGS__, __FILE__, __LINE__); \
            _tf_current_failed = 1; \
            return; \
        } \
    } while(0)

#define ASSERT_EQ(a, b) \
    do { \
        long long _ae = (long long)(a); \
        long long _be = (long long)(b); \
        if (_ae != _be) { \
            printf("\n    ASSERT_EQ FAILED: %s == %s\n" \
                   "    expected: %lld\n" \
                   "    actual:   %lld\n" \
                   "    at %s:%d\n", \
                   #a, #b, _be, _ae, __FILE__, __LINE__); \
            _tf_current_failed = 1; \
            return; \
        } \
    } while(0)

#define ASSERT_NEQ(a, b) \
    do { \
        long long _ae = (long long)(a); \
        long long _be = (long long)(b); \
        if (_ae == _be) { \
            printf("\n    ASSERT_NEQ FAILED: %s != %s\n" \
                   "    both are: %lld\n" \
                   "    at %s:%d\n", \
                   #a, #b, _ae, __FILE__, __LINE__); \
            _tf_current_failed = 1; \
            return; \
        } \
    } while(0)

#define ASSERT_STR_EQ(a, b) \
    do { \
        const char *_sa = (a); \
        const char *_sb = (b); \
        if (_sa == NULL && _sb == NULL) break; \
        if (_sa == NULL || _sb == NULL || strcmp(_sa, _sb) != 0) { \
            printf("\n    ASSERT_STR_EQ FAILED: %s == %s\n" \
                   "    expected: \"%s\"\n" \
                   "    actual:   \"%s\"\n" \
                   "    at %s:%d\n", \
                   #a, #b, \
                   _sb ? _sb : "(null)", \
                   _sa ? _sa : "(null)", \
                   __FILE__, __LINE__); \
            _tf_current_failed = 1; \
            return; \
        } \
    } while(0)

#define ASSERT_STR_CONTAINS(haystack, needle) \
    do { \
        const char *_h = (haystack); \
        const char *_n = (needle); \
        if (_h == NULL || _n == NULL || strstr(_h, _n) == NULL) { \
            printf("\n    ASSERT_STR_CONTAINS FAILED\n" \
                   "    haystack: \"%s\"\n" \
                   "    needle:   \"%s\"\n" \
                   "    at %s:%d\n", \
                   _h ? _h : "(null)", \
                   _n ? _n : "(null)", \
                   __FILE__, __LINE__); \
            _tf_current_failed = 1; \
            return; \
        } \
    } while(0)

#define ASSERT_NOT_NULL(p) \
    do { \
        if ((p) == NULL) { \
            printf("\n    ASSERT_NOT_NULL FAILED: %s is NULL\n" \
                   "    at %s:%d\n", #p, __FILE__, __LINE__); \
            _tf_current_failed = 1; \
            return; \
        } \
    } while(0)

#define ASSERT_NULL(p) \
    do { \
        if ((p) != NULL) { \
            printf("\n    ASSERT_NULL FAILED: %s is not NULL\n" \
                   "    at %s:%d\n", #p, __FILE__, __LINE__); \
            _tf_current_failed = 1; \
            return; \
        } \
    } while(0)

#define ASSERT_TRUE(expr)  ASSERT(expr)
#define ASSERT_FALSE(expr) ASSERT(!(expr))

#define TEST_MAIN(...) \
    int main(void) { \
        printf("\n"); \
        __VA_ARGS__ \
        printf("\n  ========================================\n"); \
        printf("  Pass: %d | Fail: %d | Skip: %d | Total: %d\n", \
               _tf_pass, _tf_fail, _tf_skip, _tf_total); \
        printf("  Gate: %s (full_suite: no)\n", \
               (_tf_fail > 0 || _tf_skip > 0) ? "BLOCKED" : "OPEN"); \
        printf("  ========================================\n"); \
        return _tf_fail > 0 ? 1 : 0; \
    }

#endif /* TEST_FRAMEWORK_H */
