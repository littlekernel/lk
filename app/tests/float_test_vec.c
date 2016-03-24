/*
 * Copyright (c) 2014 Travis Geiselbrecht
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

#include <stdint.h>

union double_int {
    double d;
    uint64_t i;
};

static const union double_int float_test_vec[] = {
    { .d = -2.0 },
    { .d = -1.0 },
    { .d = -0.5 },
    { .d = -0.0 },
    { .d = 0.0 },
    { .d = 0.01 },
    { .d = 0.1 },
    { .d = 0.2 },
    { .d = 0.25 },
    { .d = 0.5 },
    { .d = 0.75 },
    { .d = 1.0 },
    { .d = 2.0 },
    { .d = 3.0 },
    { .d = 10.0 },
    { .d = 100.0 },
    { .d = 123456.0 },
    { .d = -123456.0 },
    { .d = 546.5645644531f },
    { .d = -546.5645644531f },
    { .d = 0.12345 },
    { .d = 0.0000012345 },
    { .d = 0.0000019999 },
    { .d = 0.0000015 },
    { .i = 0x4005bf0a8b145649ULL }, // e
    { .i = 0x400921fb54442d18ULL }, // pi
    { .i = 0x43f0000000000000ULL }, // 2^64
    { .i = 0x7fefffffffffffffULL }, // largest normalized
    { .i = 0x0010000000000000ULL }, // least positive normalized
    { .i = 0x0000000000000001ULL }, // smallest possible denorm
    { .i = 0x000fffffffffffffULL }, // largest possible denorm
    { .i = 0x7ff0000000000001ULL }, // smallest SNAn
    { .i = 0x7ff7ffffffffffffULL }, // largest SNAn
    { .i = 0x7ff8000000000000ULL }, // smallest QNAn
    { .i = 0x7fffffffffffffffULL }, // largest QNAn
    { .i = 0xfff0000000000000ULL }, // -infinity
    { .i = 0x7ff0000000000000ULL }, // +infinity
};

#define countof(a) (sizeof(a) / sizeof((a)[0]))
static const unsigned int float_test_vec_size = countof(float_test_vec);

#define PRINT_FLOAT \
        printf("0x%016llx %f %F %a %A\n", \
                float_test_vec[i], \
                *(const double *)&float_test_vec[i], \
                *(const double *)&float_test_vec[i], \
                *(const double *)&float_test_vec[i], \
                *(const double *)&float_test_vec[i])

