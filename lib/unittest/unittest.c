/*
 * Copyright (c) 2013, Google, Inc. All rights reserved
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
/*
 * Functions for unit tests.  See lib/unittest/include/unittest.h for usage.
 */
#include <lib/unittest.h>

#include <lk/debug.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

#if LK_DEBUGLEVEL > 2
#define TEST_FAILURE_CASES 1
#else
#define TEST_FAILURE_CASES 0
#endif

// Default output function is the printf
static _printf_engine_output_func out_func = _fprintf_output_func;
// Buffer the argument to be sent to the output function
static void *out_func_arg = stdout;

/**
 * \brief Function called to dump results
 *
 * This function will call the out_func callback
 */
void unittest_printf (const char *format, ...) {
    va_list ap;
    va_start (ap, format);

    _printf_engine(out_func, out_func_arg, format, ap);

    va_end(ap);
}

bool expect_bytes_eq(const uint8_t *expected, const uint8_t *actual, size_t len,
                     const char *msg) {
    if (memcmp(expected, actual, len)) {
        printf("%s. expected\n", msg);
        hexdump8(expected, len);
        printf("actual\n");
        hexdump8(actual, len);
        return false;
    }
    return true;
}

void unittest_set_output_function (_printf_engine_output_func fun, void *arg) {
    out_func = fun;
    out_func_arg = arg;
}

/* test case for unittests itself */
static bool unittest_printf_tests(void) {
    BEGIN_TEST;

    unittest_printf("\n");
    unittest_printf("test printf\n");
    unittest_printf("test printf with args %d\n", 1);

    END_TEST;
}

static bool unittest_simple_pass(void) {
    BEGIN_TEST;

    // simple true conditions
    // note: most of these will probably compile out
    EXPECT_EQ(0, 0, "0=0");
    EXPECT_EQ(1, 1, "1=1");
    EXPECT_NE(0, 1, "0=1");
    EXPECT_NE(1, 0, "1=0");
    EXPECT_LE(0, 1, "0<=1");
    EXPECT_LE(1, 1, "1<=1");
    EXPECT_LT(0, 1, "0<1");
    EXPECT_GE(1, 0, "1>=0");
    EXPECT_GE(1, 1, "1>=1");
    EXPECT_GT(1, 0, "1>0");
    EXPECT_TRUE(true, "true");
    EXPECT_FALSE(false, "false");
    EXPECT_NULL((void *)0, "null");
    EXPECT_NONNULL((void *)1, "nonnull");

    END_TEST;
}

static bool unittest_simple_fail(void) {
    BEGIN_TEST;

    // failure conditions
    EXPECT_EQ(0, 1, "0=1");
    EXPECT_EQ(1, 0, "1=0");
    EXPECT_NE(1, 1, "1=1");
    EXPECT_LE(2, 1, "2<=1");
    EXPECT_LE(2, 1, "2<=1");
    EXPECT_LT(2, 1, "2<1");
    EXPECT_GE(1, 2, "1>=2");
    EXPECT_GE(1, 2, "1>=2");
    EXPECT_GT(1, 2, "1>2");
    EXPECT_TRUE(false, "false");
    EXPECT_FALSE(true, "true");
    EXPECT_NULL((void *)1, "null");
    EXPECT_NONNULL((void *)0, "nonnull");

    END_TEST;
}

static bool unittest_assert_pass(void) {
    BEGIN_TEST;

    // simple true conditions
    // note: most of these will probably compile out
    ASSERT_EQ(0, 0, "0=0");
    ASSERT_EQ(1, 1, "1=1");
    ASSERT_NE(0, 1, "0=1");
    ASSERT_NE(1, 0, "1=0");
    ASSERT_LE(0, 1, "0<=1");
    ASSERT_LE(1, 1, "1<=1");
    ASSERT_LT(0, 1, "0<1");
    ASSERT_GE(1, 0, "1>=0");
    ASSERT_GE(1, 1, "1>=1");
    ASSERT_GT(1, 0, "1>0");
    ASSERT_TRUE(true, "true");
    ASSERT_FALSE(false, "false");
    EXPECT_NULL((void *)0, "null");
    EXPECT_NONNULL((void *)1, "nonnull");

    END_TEST;
}

static bool unittest_assert_fail_eq(void) {
    BEGIN_TEST;
    ASSERT_EQ(0, 1, "0=1");
    unittest_printf("should not see this\n");
    END_TEST;
}

static bool unittest_assert_fail_ne(void) {
    BEGIN_TEST;
    ASSERT_NE(1, 1, "1=1");
    unittest_printf("should not see this\n");
    END_TEST;
}

static bool unittest_assert_fail_le(void) {
    BEGIN_TEST;
    ASSERT_LE(2, 1, "2<=1");
    unittest_printf("should not see this\n");
    END_TEST;
}

static bool unittest_assert_fail_lt(void) {
    BEGIN_TEST;
    ASSERT_LT(2, 1, "2<1");
    unittest_printf("should not see this\n");
    END_TEST;
}

static bool unittest_assert_fail_ge(void) {
    BEGIN_TEST;
    ASSERT_GE(1, 2, "1>=2");
    unittest_printf("should not see this\n");
    END_TEST;
}

static bool unittest_assert_fail_gt(void) {
    BEGIN_TEST;
    ASSERT_GT(1, 2, "1>2");
    unittest_printf("should not see this\n");
    END_TEST;
}

static bool unittest_assert_fail_true(void) {
    BEGIN_TEST;
    ASSERT_GT(1, 2, "1>2");
    unittest_printf("should not see this\n");
    END_TEST;
}

static bool unittest_assert_fail_false(void) {
    BEGIN_TEST;
    ASSERT_GT(1, 2, "1>2");
    unittest_printf("should not see this\n");
    END_TEST;
}

static bool unittest_assert_fail_null(void) {
    BEGIN_TEST;
    ASSERT_NULL((void *)1, "null");
    unittest_printf("should not see this\n");
    END_TEST;
}

static bool unittest_assert_fail_nonnull(void) {
    BEGIN_TEST;
    ASSERT_NONNULL((void *)0, "nonnull");
    unittest_printf("should not see this\n");
    END_TEST;
}

BEGIN_TEST_CASE(unittest)
    RUN_TEST(unittest_printf_tests);
    RUN_TEST(unittest_simple_pass);
    RUN_TEST(unittest_assert_pass);
#if TEST_FAILURE_CASES
    RUN_TEST(unittest_simple_fail);
    RUN_TEST(unittest_assert_fail_eq);
    RUN_TEST(unittest_assert_fail_ne);
    RUN_TEST(unittest_assert_fail_le);
    RUN_TEST(unittest_assert_fail_lt);
    RUN_TEST(unittest_assert_fail_ge);
    RUN_TEST(unittest_assert_fail_gt);
    RUN_TEST(unittest_assert_fail_true);
    RUN_TEST(unittest_assert_fail_false);
    RUN_TEST(unittest_assert_fail_null);
    RUN_TEST(unittest_assert_fail_nonnull);
#endif
END_TEST_CASE(unittest)
