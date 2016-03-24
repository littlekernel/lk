/*
 * Copyright (c) 2013, Google, Inc. All rights reserved
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef _LIB_UNITTEST_INCLUDE_UNITTEST_H_
#define _LIB_UNITTEST_INCLUDE_UNITTEST_H_
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
#include <stdbool.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <trace.h>
#include <stdarg.h>

#define PRINT_BUFFER_SIZE                                       (512)

/*
 * Type for unit test result Output
 */
typedef void (*test_output_func) (const char *line, int len, void *arg);

/*
 * Printf dedicated to the unittest library
 * the default output is the printf
 */
void unittest_printf (const char *format, ...);

/*
 * Function to set the callback for printing
 * the unit test output
 */
void unittest_set_output_function (test_output_func fun, void *arg);

/*
 * Macros to format the error string
 */
#define EXPECTED_STRING            "%s:\n        expected "
#define UNITTEST_TRACEF(str, x...) do {                                 \
        unittest_printf(" [FAILED] \n        %s:%d:\n        " str,     \
                __PRETTY_FUNCTION__, __LINE__, ## x);                   \
    } while (0)

/*
 * BEGIN_TEST_CASE and END_TEST_CASE define a function that calls
 * RUN_TEST.
 */
#define BEGIN_TEST_CASE(case_name)              \
    bool case_name(void)                        \
    {                                           \
    bool all_success = true;                    \
    unittest_printf("\nCASE %-49s [STARTED] \n", #case_name);

#define DEFINE_REGISTER_TEST_CASE(case_name)                            \
    static void _register_##case_name(void)                             \
    {                                                                   \
        unittest_register_test_case(&_##case_name##_element);           \
    }                                                                   \
    void (*_register_##case_name##_ptr)(void) __SECTION(".ctors") =     \
        _register_##case_name;

#define END_TEST_CASE(case_name)                                        \
    if (all_success) {                                                  \
        unittest_printf("CASE %-59s [PASSED]\n", #case_name);           \
    } else {                                                            \
        unittest_printf("CASE %-59s [FAILED]\n", #case_name);           \
    }                                                                   \
        return all_success;                                             \
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
         all_success = false;                           \
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
 * Use the EXPECT_* macros to check test results.
 */
#define EXPECT_EQ(expected, actual, msg)                                   \
    {                                                                      \
        typeof(actual) _e = expected;                                      \
        typeof(actual) _a = actual;                                        \
        if (_e != _a) {                                                    \
            UNITTEST_TRACEF (EXPECTED_STRING "%s (%d), "                   \
                   "actual %s (%d)\n",                                     \
                   msg, #expected, (int)_e, #actual, (int)_a);             \
            all_ok = false;                                                \
        }                                                                  \
    }

#define EXPECT_NEQ(expected, actual, msg)                                  \
    {                                                                      \
        const typeof(expected) _e = expected;                              \
        if (_e == actual) {                                                \
            UNITTEST_TRACEF(EXPECTED_STRING "%s (%d), %s"                  \
                   " to differ, but they are the same %d\n",               \
                   msg, #expected, (int)_e, #actual);                      \
            all_ok = false;                                                \
        }                                                                  \
    }

#define EXPECT_LE(expected, actual, msg)                                   \
    {                                                                      \
        const typeof(actual) _e = expected;                                \
        const typeof(actual) _a = actual;                                  \
        if (_e > _a) {                                                     \
            UNITTEST_TRACEF(EXPECTED_STRING "%s (%d) to be"                \
                   " less-than-or-equal-to actual %s (%d)\n",              \
                   msg, #expected, (int)_e, #actual, (int)_a);             \
            all_ok = false;                                                \
        }                                                                  \
    }

#define EXPECT_LT(expected, actual, msg)                                  \
    {                                                                     \
        const typeof(actual) _e = expected;                               \
        const typeof(actual) _a = actual;                                 \
        if (_e >= _a) {                                                   \
            UNITTEST_TRACEF(EXPECTED_STRING "%s (%d) to be"               \
                   " less-than actual %s (%d)\n",                         \
                   msg, #expected, (int)_e, #actual, (int)_a);             \
            all_ok = false;                                               \
        }                                                                 \
    }

#define EXPECT_GE(expected, actual, msg)                                  \
    {                                                                     \
        const typeof(actual) _e = expected;                               \
        const typeof(actual) _a = actual;                                 \
        if (_e < _a) {                                                    \
            UNITTEST_TRACEF(EXPECTED_STRING "%s (%d) to be"               \
                   " greater-than-or-equal-to actual %s (%d)\n",          \
                   msg, #expected, (int)_e, #actual, (int)_a);             \
            all_ok = false;                                               \
        }                                                                 \
    }

#define EXPECT_GT(expected, actual, msg)                                  \
    {                                                                     \
        const typeof(actual) _e = expected;                               \
        const typeof(actual) _a = actual;                                 \
        if (_e <= _a) {                                                   \
            UNITTEST_TRACEF(EXPECTED_STRING "%s (%d) to be"               \
                   " greater-than actual %s (%d)\n",                      \
                   msg, #expected, (int)_e, #actual, (int)_a);             \
            all_ok = false;                                               \
        }                                                                 \
    }

#define EXPECT_TRUE(actual, msg)                                          \
    if (!(actual)) {                                                      \
        UNITTEST_TRACEF("%s: %s is false\n", msg, #actual);               \
        all_ok = false;                                                   \
    }

#define EXPECT_FALSE(actual, msg)                                         \
    if (actual) {                                                         \
        UNITTEST_TRACEF("%s: %s is true\n", msg, #actual);                \
        all_ok = false;                                                   \
    }

#define EXPECT_BYTES_EQ(expected, actual, length, msg)                    \
    if (!expect_bytes_eq(expected, actual, length, msg)) {                \
        all_ok = false;                                                   \
    }

#define EXPECT_BYTES_NE(bytes1, bytes2, length, msg)                      \
    if (!memcmp(bytes1, bytes2, length)) {                                \
        UNITTEST_TRACEF("%s and %s are the same; "                        \
               "expected different\n", #bytes1, #bytes2);                 \
        hexdump8(bytes1, length);                                         \
        all_ok = false;                                                   \
    }

/* For comparing uint64_t, like hw_id_t. */
#define EXPECT_EQ_LL(expected, actual, msg)                               \
    {                                                                     \
        const typeof(actual) _e = expected;                               \
        const typeof(actual) _a = actual;                                 \
        if (_e != _a) {                                                   \
            UNITTEST_TRACEF("%s: expected %llu, actual %llu\n",           \
                   msg, _e, _a);                                          \
            all_ok = false;                                               \
        }                                                                 \
    }

/*
 * The ASSERT_* macros are similar to the EXPECT_* macros except that
 * they return on failure.
 */
#define ASSERT_NOT_NULL(p)                      \
    if (!p) {                   \
        UNITTEST_TRACEF("ERROR: NULL pointer\n");    \
        return false;               \
    }

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

#endif  /* _LIB_UNITTEST_INCLUDE_UNITTEST_H_ */
