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

/**
 * \brief Default function to dump unit test results
 *
 * \param[in] line is the buffer to dump
 * \param[in] len is the length of the buffer to dump
 * \param[in] arg can be any kind of arguments needed to dump the values
 */
static void default_printf (const char *line, int len, void *arg) {
    printf (line);
}

// Default output function is the printf
static test_output_func out_func = default_printf;
// Buffer the argument to be sent to the output function
static void *out_func_arg = NULL;

/**
 * \brief Function called to dump results
 *
 * This function will call the out_func callback
 */
void unittest_printf (const char *format, ...) {
    static char print_buffer[PRINT_BUFFER_SIZE];

    va_list argp;
    va_start (argp, format);

    if (out_func != NULL) {
        // Format the string
        vsnprintf(print_buffer, PRINT_BUFFER_SIZE, format, argp);
        out_func (print_buffer, PRINT_BUFFER_SIZE, out_func_arg);
    }

    va_end (argp);
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

void unittest_set_output_function (test_output_func fun, void *arg) {
    out_func = fun;
    out_func_arg = arg;
}
