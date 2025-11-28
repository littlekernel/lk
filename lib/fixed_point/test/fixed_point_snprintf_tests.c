/*
 * Copyright (c) 2025 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
/*
 * Tests for fp_32_64_snprintf fixed point formatting.
 */
#include <lib/fixed_point.h>
#include <lib/unittest.h>
#include <string.h>

static bool test_zero(void) {
    BEGIN_TEST;
    struct fp_32_64 fp = {0, 0, 0};
    char buf[32];
    EXPECT_EQ(buf, fp_32_64_snprintf(buf, sizeof(buf), &fp, 9), "return pointer");
    EXPECT_EQ(0, strcmp("0.000000000", buf), "zero format");
    END_TEST;
}

static bool test_integer_only(void) {
    BEGIN_TEST;
    struct fp_32_64 fp = {42, 0, 0};
    char buf[32];
    fp_32_64_snprintf(buf, sizeof(buf), &fp, 5);
    EXPECT_EQ(0, strcmp("42.00000", buf), "integer part");
    END_TEST;
}

static bool test_quarter(void) {
    BEGIN_TEST;
    struct fp_32_64 fp; // 1/4
    fp_32_64_div_32_32(&fp, 1, 4); // sets l0=0, l32=0x40000000, l64=0
    char buf[32];
    fp_32_64_snprintf(buf, sizeof(buf), &fp, 9);
    EXPECT_EQ(0, strcmp("0.250000000", buf), "1/4");
    END_TEST;
}

static bool test_half(void) {
    BEGIN_TEST;
    struct fp_32_64 fp; // 1/2
    fp_32_64_div_32_32(&fp, 1, 2); // l32=0x80000000
    char buf[32];
    fp_32_64_snprintf(buf, sizeof(buf), &fp, 6);
    EXPECT_EQ(0, strcmp("0.500000", buf), "1/2");
    END_TEST;
}

static bool test_one_third(void) {
    BEGIN_TEST;
    struct fp_32_64 fp; // 1/3
    fp_32_64_div_32_32(&fp, 1, 3);
    char buf[32];
    fp_32_64_snprintf(buf, sizeof(buf), &fp, 9);
    EXPECT_EQ(0, strcmp("0.333333333", buf), "1/3 truncated");
    END_TEST;
}

static bool test_large_integer(void) {
    BEGIN_TEST;
    struct fp_32_64 fp; // 3000000000/1000 = 3000000
    fp_32_64_div_64_32(&fp, 3000000000ULL, 1000);
    char buf[32];
    fp_32_64_snprintf(buf, sizeof(buf), &fp, 9);
    EXPECT_EQ(0, strcmp("3000000.000000000", buf), "large integer");
    END_TEST;
}

static bool test_hires_ratio(void) {
    BEGIN_TEST;
    struct fp_32_64 fp; // 1000000/3000000000
    fp_32_64_div_32_64(&fp, 1000000, 3000000000ULL);
    char buf[32];
    fp_32_64_snprintf(buf, sizeof(buf), &fp, 9);
    EXPECT_EQ(0, strcmp("0.000333333", buf), "hires ratio");
    END_TEST;
}

BEGIN_TEST_CASE(fixed_point_snprintf_tests)
RUN_TEST(test_zero)
RUN_TEST(test_integer_only)
RUN_TEST(test_quarter)
RUN_TEST(test_half)
RUN_TEST(test_one_third)
RUN_TEST(test_large_integer)
RUN_TEST(test_hires_ratio)
END_TEST_CASE(fixed_point_snprintf_tests)
