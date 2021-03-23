/*
 * Copyright (c) 2013, Google, Inc. All rights reserved
 * Copyright 2016 The Fuchsia Authors
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once
/*
 * Macros for writing unit tests.
 *
 * Sample usage:
 *
 * A test case runs a collection of tests like this, with
 * BEGIN_TEST_CASE and END_TEST_CASE and the beginning and end of the
 * function and RUN_TEST to call each individual test, as follows:
 *
 *  BEGIN_TEST_CASE(foo_tests);
 *
 *  RUN_TEST(test_foo);
 *  RUN_TEST(test_bar);
 *  RUN_TEST(test_baz);
 *
 *  END_TEST_CASE;
 *
 * This creates a static function foo_tests() and registers it with the
 * unit test framework.  foo_tests() can be executed either by a shell
 * command or by a call to run_all_tests(), which runs all registered
 * unit tests.
 *
 * A test looks like this, using the BEGIN_TEST and END_TEST macros at
 * the beginning and end of the test and the EXPECT_* macros to
 * validate test results, as shown:
 *
 * static bool test_foo(void)
 * {
 *      BEGIN_TEST;
 *
 *      ...declare variables and do stuff...
 *      int foo_value = foo_func();
 *      ...See if the stuff produced the correct value...
 *      EXPECT_EQ(1, foo_value, "foo_func failed");
 *      ... there are EXPECT_* macros for many conditions...
 *      EXPECT_TRUE(foo_condition(), "condition should be true");
 *      EXPECT_NEQ(ERR_TIMED_OUT, foo_event(), "event timed out");
 *
 *      END_TEST;
 * }
 *
 * To your rules.mk file, add lib/unittest to MODULE_DEPS:
 *
 * MODULE_DEPS += \
 *         lib/unittest   \
 */
#include <printf.h>
#include <stdbool.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

/*
 * Printf dedicated to the unittest library
 * the default output is the printf
 */
void unittest_printf (const char *format, ...) __PRINTFLIKE(1, 2);

/*
 * Function to set the callback for printing
 * the unit test output
 */
void unittest_set_output_function (_printf_engine_output_func fun, void *arg);

/*
 * Macros to format the error string
 */
#define EXPECTED_STRING "%s\n        expected "
#define UNITTEST_FAIL_TRACEF_FORMAT "\n        [FAILED]\n        %s:%d: "
#define UNITTEST_FAIL_TRACEF(str, x...)                                                   \
  do {                                                                                    \
    unittest_printf(UNITTEST_FAIL_TRACEF_FORMAT str, __PRETTY_FUNCTION__, __LINE__, ##x); \
  } while (0)

/*
 * BEGIN_TEST_CASE and END_TEST_CASE define a function that calls
 * RUN_TEST.
 */
#define BEGIN_TEST_CASE(case_name)              \
    bool case_name(void)                        \
    {                                           \
    bool all_ok = true;                    \
    unittest_printf("\nCASE %-49s [STARTED] \n", #case_name);

#define DEFINE_REGISTER_TEST_CASE(case_name)                            \
    static void _register_##case_name(void)                             \
    {                                                                   \
        unittest_register_test_case(&_##case_name##_element);           \
    }                                                                   \
    void (*_register_##case_name##_ptr)(void) __SECTION(".ctors") =     \
        _register_##case_name;

#define END_TEST_CASE(case_name)                                        \
    if (all_ok) {                                                  \
        unittest_printf("CASE %-59s [PASSED]\n", #case_name);           \
    } else {                                                            \
        unittest_printf("CASE %-59s [FAILED]\n", #case_name);           \
    }                                                                   \
        return all_ok;                                             \
    }                                                                   \
    static struct test_case_element _##case_name##_element = {          \
        .next = NULL,                                                   \
        .failed_next = NULL,                                            \
        .name = #case_name,                                             \
        .test_case = case_name,                                         \
    };                                                                  \
    DEFINE_REGISTER_TEST_CASE(case_name);

#define RUN_TEST(test)                                  \
    unittest_printf("    %-50s [RUNNING]",  #test );    \
    if (! test ()) {                                    \
         all_ok = false;                           \
    } else {                                            \
        unittest_printf(" [PASSED] \n");                \
    }

/*
 * BEGIN_TEST and END_TEST go in a function that is called by RUN_TEST
 * and that call the EXPECT_ macros.
 */
#define BEGIN_TEST bool all_ok = true
#define END_TEST return all_ok

/*
 * UTCHECK_* macros are used to check test results.  Generally, one should
 * prefer to use either the EXPECT_* (non-terminating) or ASSERT_*
 * (terminating) forms of the macros.  See below.
 *
 * The parameter after |term| is an optional message (const char*) to be printed
 * if the check fails.
 */
#define UTCHECK_EQ(expected, actual, term, msg)                            \
    {                                                                      \
        const typeof(actual) _e = expected;                                \
        const typeof(actual) _a = actual;                                  \
        if (_e != _a) {                                                    \
            UNITTEST_FAIL_TRACEF (EXPECTED_STRING "%s (%ld), "             \
                   "actual %s (%ld)\n",                                    \
                   msg, #expected, (long)_e, #actual, (long)_a);           \
            if (term) {                                                    \
                return false;                                              \
            }                                                              \
            all_ok = false;                                                \
        }                                                                  \
    }

#define UTCHECK_NE(expected, actual, term, msg)                            \
    {                                                                      \
        const typeof(expected) _e = expected;                              \
        const typeof(actual) _a = actual;                                  \
        if (_e == _a) {                                                    \
            UNITTEST_FAIL_TRACEF(EXPECTED_STRING "%s (%ld), %s"            \
                   " to differ, but they are the same\n",                  \
                   msg, #expected, (long)_e, #actual);                     \
            if (term) {                                                    \
                return false;                                              \
            }                                                              \
            all_ok = false;                                                \
        }                                                                  \
    }

#define UTCHECK_LE(expected, actual, term, msg)                            \
    {                                                                      \
        const typeof(actual) _e = expected;                                \
        const typeof(actual) _a = actual;                                  \
        if (_e > _a) {                                                     \
            UNITTEST_FAIL_TRACEF(EXPECTED_STRING "%s (%ld) to be"          \
                   " less-than-or-equal-to actual %s (%ld)\n",             \
                   msg, #expected, (long)_e, #actual, (long)_a);           \
            if (term) {                                                    \
                return false;                                              \
            }                                                              \
            all_ok = false;                                                \
        }                                                                  \
    }

#define UTCHECK_LT(expected, actual, term, msg)                            \
    {                                                                      \
        const typeof(actual) _e = expected;                                \
        const typeof(actual) _a = actual;                                  \
        if (_e >= _a) {                                                    \
            UNITTEST_FAIL_TRACEF(EXPECTED_STRING "%s (%ld) to be"          \
                   " less-than actual %s (%ld)\n",                         \
                   msg, #expected, (long)_e, #actual, (long)_a);           \
            if (term) {                                                    \
                return false;                                              \
            }                                                              \
            all_ok = false;                                                \
        }                                                                  \
    }

#define UTCHECK_GE(expected, actual, term, msg)                            \
    {                                                                      \
        const typeof(actual) _e = expected;                                \
        const typeof(actual) _a = actual;                                  \
        if (_e < _a) {                                                     \
            UNITTEST_FAIL_TRACEF(EXPECTED_STRING "%s (%ld) to be"          \
                   " greater-than-or-equal-to actual %s (%ld)\n",          \
                   msg, #expected, (long)_e, #actual, (long)_a);           \
            if (term) {                                                    \
                return false;                                              \
            }                                                              \
            all_ok = false;                                                \
        }                                                                  \
    }

#define UTCHECK_GT(expected, actual, term, msg)                            \
    {                                                                      \
        const typeof(actual) _e = expected;                                \
        const typeof(actual) _a = actual;                                  \
        if (_e <= _a) {                                                    \
            UNITTEST_FAIL_TRACEF(EXPECTED_STRING "%s (%ld) to be"          \
                   " greater-than actual %s (%ld)\n",                      \
                   msg, #expected, (long)_e, #actual, (long)_a);           \
            if (term) {                                                    \
                return false;                                              \
            }                                                              \
            all_ok = false;                                                \
        }                                                                  \
    }

#define UTCHECK_TRUE(actual, term, msg)                                    \
    if (!(actual)) {                                                       \
        UNITTEST_FAIL_TRACEF("%s: %s is false\n", msg, #actual);           \
        if (term) {                                                        \
            return false;                                                  \
        }                                                                  \
        all_ok = false;                                                    \
    }

#define UTCHECK_FALSE(actual, term, msg)                                   \
    if (actual) {                                                          \
        UNITTEST_FAIL_TRACEF("%s: %s is true\n", msg, #actual);            \
        if (term) {                                                        \
            return false;                                                  \
        }                                                                  \
        all_ok = false;                                                    \
    }

#define UTCHECK_NULL(actual, term, msg)                                    \
    if (actual != NULL) {                                                  \
        UNITTEST_FAIL_TRACEF("%s: %s is non-null\n", msg, #actual);        \
        if (term) {                                                        \
            return false;                                                  \
        }                                                                  \
        all_ok = false;                                                    \
    }

#define UTCHECK_NONNULL(actual, term, msg)                                 \
    if (actual == NULL) {                                                  \
        UNITTEST_FAIL_TRACEF("%s: %s is null\n", msg, #actual);            \
        if (term) {                                                        \
            return false;                                                  \
        }                                                                  \
        all_ok = false;                                                    \
    }


#define UTCHECK_BYTES_EQ(expected, actual, length, term, msg)              \
    if (!expect_bytes_eq(expected, actual, length, msg)) {                 \
        if (term) {                                                        \
            return false;                                                  \
        }                                                                  \
        all_ok = false;                                                    \
    }

#define UTCHECK_BYTES_NE(bytes1, bytes2, length, term, msg)                \
    if (!memcmp(bytes1, bytes2, length)) {                                 \
        UNITTEST_FAIL_TRACEF("%s and %s are the same; "                    \
               "expected different\n", #bytes1, #bytes2);                  \
        hexdump8(bytes1, length);                                          \
        if (term) {                                                        \
            return false;                                                  \
        }                                                                  \
        all_ok = false;                                                    \
    }

/* For comparing uint64_t, like hw_id_t. */
#define UTCHECK_EQ_LL(expected, actual, term, msg)                         \
    {                                                                      \
        const typeof(actual) _e = expected;                                \
        const typeof(actual) _a = actual;                                  \
        if (_e != _a) {                                                    \
            UNITTEST_FAIL_TRACEF("%s: expected %llu, actual %llu\n",       \
                   msg, _e, _a);                                           \
            if (term) {                                                    \
                return false;                                              \
            }                                                              \
            all_ok = false;                                                \
        }                                                                  \
    }

/* EXPECT_* macros check the supplied condition and will print a diagnostic
 * message and flag the test as having failed if the condition fails.  The test
 * will continue to run, even if the condition fails.
 *
 * The last parameter is an optional const char* message to be included in the
 * print diagnostic message.
 */
#define EXPECT_EQ(expected, actual, msg) UTCHECK_EQ(expected, actual, false, msg)
#define EXPECT_NE(expected, actual, msg) UTCHECK_NE(expected, actual, false, msg)
#define EXPECT_LE(expected, actual, msg) UTCHECK_LE(expected, actual, false, msg)
#define EXPECT_LT(expected, actual, msg) UTCHECK_LT(expected, actual, false, msg)
#define EXPECT_GE(expected, actual, msg) UTCHECK_GE(expected, actual, false, msg)
#define EXPECT_GT(expected, actual, msg) UTCHECK_GT(expected, actual, false, msg)
#define EXPECT_TRUE(actual, msg) UTCHECK_TRUE(actual, false, msg)
#define EXPECT_FALSE(actual, msg) UTCHECK_FALSE(actual, false, msg)
#define EXPECT_BYTES_EQ(expected, actual, length, msg) \
  UTCHECK_BYTES_EQ(expected, actual, length, false, msg)
#define EXPECT_BYTES_NE(bytes1, bytes2, length, msg) \
  UTCHECK_BYTES_NE(bytes1, bytes2, length, false, msg)
#define EXPECT_EQ_LL(expected, actual, msg) UTCHECK_EQ_LL(expected, actual, false, msg)
#define EXPECT_NULL(actual, msg) UTCHECK_NULL(actual, false, msg)
#define EXPECT_NONNULL(actual, msg) UTCHECK_NONNULL(actual, false, msg)
#define EXPECT_OK(actual, msg) UTCHECK_EQ(ZX_OK, actual, false, msg)

/* ASSERT_* macros check the condition and will print a message and immediately
 * abort a test with a filure status if the condition fails.
 */
#define ASSERT_EQ(expected, actual, msg) UTCHECK_EQ(expected, actual, true, msg)
#define ASSERT_NE(expected, actual, msg) UTCHECK_NE(expected, actual, true, msg)
#define ASSERT_LE(expected, actual, msg) UTCHECK_LE(expected, actual, true, msg)
#define ASSERT_LT(expected, actual, msg) UTCHECK_LT(expected, actual, true, msg)
#define ASSERT_GE(expected, actual, msg) UTCHECK_GE(expected, actual, true, msg)
#define ASSERT_GT(expected, actual, msg) UTCHECK_GT(expected, actual, true, msg)
#define ASSERT_TRUE(actual, msg) UTCHECK_TRUE(actual, true, msg)
#define ASSERT_FALSE(actual, msg) UTCHECK_FALSE(actual, true, msg)
#define ASSERT_BYTES_EQ(expected, actual, length, msg) \
  UTCHECK_BYTES_EQ(expected, actual, length, true, msg)
#define ASSERT_BYTES_NE(bytes1, bytes2, length, msg) \
  UTCHECK_BYTES_NE(bytes1, bytes2, length, true, msg)
#define ASSERT_EQ_LL(expected, actual, msg) UTCHECK_EQ_LL(expected, actual, true, msg)
#define ASSERT_NULL(actual, msg) UTCHECK_NULL(actual, true, msg)
#define ASSERT_NONNULL(actual, msg) UTCHECK_NONNULL(actual, true, msg)
#define ASSERT_OK(actual, msg) UTCHECK_EQ(ZX_OK, actual, true, msg)

/*
 * The list of test cases is made up of these elements.
 */
struct test_case_element {
    struct test_case_element *next;
    struct test_case_element *failed_next;
    const char *name;
    bool (*test_case)(void);
};


/*
 * Registers a test case with the unit test framework.
 */
void unittest_register_test_case(struct test_case_element *elem);

/*
 * Runs all registered test cases.
 */
bool run_all_tests(void);

/*
 * Returns false if expected does not equal actual and prints msg and a hexdump8
 * of the input buffers.
 */
bool expect_bytes_eq(const uint8_t *expected, const uint8_t *actual, size_t len,
                     const char *msg);
