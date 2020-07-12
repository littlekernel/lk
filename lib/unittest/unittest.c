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
BEGIN_TEST_CASE(unittest)
    unittest_printf("test printf\n");
    unittest_printf("test printf with args %d\n", 1);

    EXPECT_EQ(0, 0, "0=0");
    EXPECT_EQ(0, 1, "0=1");
    EXPECT_EQ(1, 0, "1=0");
END_TEST_CASE(unittest)
