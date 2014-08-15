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

/*
 * BEGIN_TEST_CASE and END_TEST_CASE define a function that calls
 * RUN_TEST.
 */
#define BEGIN_TEST_CASE(case_name)              \
    bool case_name(void)                        \
    {                                           \
    bool all_success = true;                    \
    printf("\nRunning %s\n", #case_name);

#define DEFINE_REGISTER_TEST_CASE(case_name)                            \
    static void _register_##case_name(void)                             \
    {                                                                   \
        unittest_register_test_case(&_##case_name##_element);           \
    }                                                                   \
    void (*_register_##case_name##_ptr)(void) __SECTION(".ctors") =     \
        _register_##case_name;

#define END_TEST_CASE(case_name)                                        \
    if (all_success) {                                                  \
        printf("SUCCESS!  All tests in %s passed.\n", #case_name);      \
    } else {                                                            \
        printf("FAILED!  One or more tests in %s failed.\n", #case_name); \
    }                                                                   \
        return all_success;                                             \
    }                                                                   \
    static struct test_case_element _##case_name##_element = { \
        .next = NULL,                                                   \
        .failed_next = NULL,                                            \
        .name = #case_name,                                             \
        .test_case = case_name,                                         \
    };                                                                  \
    DEFINE_REGISTER_TEST_CASE(case_name);

#define RUN_TEST(test)                          \
    printf("Running " #test "\n");              \
    if (! test ()) {                            \
        printf("FAILED " #test "\n");           \
        all_success = false;                    \
    } else {                                    \
        printf("PASSED " #test "\n");           \
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
#define EXPECT_EQ(expected, actual, msg)                \
    {                                                   \
        typeof(actual) _e = expected;                   \
        typeof(actual) _a = actual;                     \
        if (_e != _a) {                                 \
            TRACEF("%s: expected " #expected " (%d), "  \
                   "actual " #actual " (%d)\n",     \
                   msg, (int)_e, (int)_a);              \
            all_ok = false;                             \
        }                                               \
    }

#define EXPECT_NEQ(expected, actual, msg)                       \
    {                               \
        const typeof(expected) _e = expected;           \
        if (_e == actual) {                 \
            TRACEF("%s: expected " #expected " and actual " \
                   #actual                  \
                   " to differ, but they are the same %d\n",    \
                   msg, (int)_e);               \
            all_ok = false;                 \
        }                           \
    }

#define EXPECT_LE(expected, actual, msg)                        \
    {                               \
        const typeof(actual) _e = expected;         \
        const typeof(actual) _a = actual;           \
        if (_e > _a) {                      \
            TRACEF("%s: expected " #expected " (%d) to be"  \
                   " less-than-or-equal-to actual "     \
                   #actual  " (%d)\n",          \
                   msg, (int)_e, (int)_a);          \
            all_ok = false;                 \
        }                           \
    }

#define EXPECT_LT(expected, actual, msg)                        \
    {                                                           \
        const typeof(actual) _e = expected;                     \
        const typeof(actual) _a = actual;                       \
        if (_e >= _a) {                                         \
            TRACEF("%s: expected " #expected " (%d) to be"      \
                   " less-than actual "                         \
                   #actual  " (%d)\n",                          \
                   msg, (int)_e, (int)_a);                      \
            all_ok = false;                                     \
        }                                                       \
    }

#define EXPECT_GE(expected, actual, msg)                        \
    {                               \
        const typeof(actual) _e = expected;         \
        const typeof(actual) _a = actual;           \
        if (_e < _a) {                      \
            TRACEF("%s: expected " #expected " (%d) to be"  \
                   " greater-than-or-equal-to actual "          \
                   #actual " (%d)\n",                           \
                   msg, (int)_e, (int)_a);          \
            all_ok = false;                 \
        }                           \
    }

#define EXPECT_GT(expected, actual, msg)                        \
    {                                                           \
        const typeof(actual) _e = expected;                     \
        const typeof(actual) _a = actual;                       \
        if (_e <= _a) {                                         \
            TRACEF("%s: expected " #expected " (%d) to be"      \
                   " greater-than actual "                      \
                   #actual " (%d)\n",                           \
                   msg, (int)_e, (int)_a);                      \
            all_ok = false;                                     \
        }                                                       \
    }

#define EXPECT_TRUE(actual, msg)                        \
    if (!(actual)) {                                    \
        TRACEF("%s: " #actual " is false\n", msg);      \
        all_ok = false;                                 \
    }

#define EXPECT_FALSE(actual, msg)                       \
    if (actual) {                                       \
        TRACEF("%s: " #actual " is true\n", msg);       \
        all_ok = false;                                 \
    }

#define EXPECT_BYTES_EQ(expected, actual, length, msg)          \
    if (!expect_bytes_eq(expected, actual, length, msg)) {  \
        all_ok = false;                                         \
    }

#define EXPECT_BYTES_NE(bytes1, bytes2, length, msg)            \
    if (!memcmp(bytes1, bytes2, length)) {                      \
        TRACEF(#bytes1 " and " #bytes2 " are the same; "    \
               "expected different\n");             \
        hexdump8(bytes1, length);               \
        all_ok = false;                     \
    }

/* For comparing uint64_t, like hw_id_t. */
#define EXPECT_EQ_LL(expected, actual, msg)             \
    {                                                   \
        const typeof(actual) _e = expected;             \
        const typeof(actual) _a = actual;               \
        if (_e != _a) {                                 \
            TRACEF("%s: expected %llu, actual %llu\n",  \
                   msg, _e, _a);                        \
            all_ok = false;                             \
        }                                               \
    }

/*
 * The ASSERT_* macros are similar to the EXPECT_* macros except that
 * they return on failure.
 */
#define ASSERT_NOT_NULL(p)                      \
    if (!p) {                   \
        TRACEF("ERROR: NULL pointer\n");    \
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
