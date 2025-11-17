/*
 * Copyright (c) 2025 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lib/fixed_point.h>
#include <lib/unittest.h>

static bool test_fp32_64_div_construct(void) {
    BEGIN_TEST;

    struct fp_32_64 f;

    // 0/1 -> 0.0
    fp_32_64_div_32_32(&f, 0u, 1u);
    EXPECT_EQ(f.l0, 0u, "0/1 l0");
    EXPECT_EQ(f.l32, 0u, "0/1 l32");
    EXPECT_EQ(f.l64, 0u, "0/1 l64");

    // 1/1 -> 1.0
    fp_32_64_div_32_32(&f, 1u, 1u);
    EXPECT_EQ(f.l0, 1u, "1/1 l0");
    EXPECT_EQ(f.l32, 0u, "1/1 l32");
    EXPECT_EQ(f.l64, 0u, "1/1 l64");

    // 1/2 -> 0.5
    fp_32_64_div_32_32(&f, 1u, 2u);
    EXPECT_EQ(f.l0, 0u, "1/2 l0");
    EXPECT_EQ(f.l32, 0x80000000u, "1/2 l32");
    EXPECT_EQ(f.l64, 0u, "1/2 l64");

    // 3/2 -> 1.5
    fp_32_64_div_32_32(&f, 3u, 2u);
    EXPECT_EQ(f.l0, 1u, "3/2 l0");
    EXPECT_EQ(f.l32, 0x80000000u, "3/2 l32");
    EXPECT_EQ(f.l64, 0u, "3/2 l64");

    END_TEST;
}

static bool test_u64_mul_u32_fp32_64(void) {
    BEGIN_TEST;

    struct fp_32_64 b;

    // 10 * 1.5 = 15
    fp_32_64_div_32_32(&b, 3u, 2u);
    EXPECT_EQ(u64_mul_u32_fp32_64(10u, b), 15u, "10 * 1.5 ~= 15");

    // 1 * 0.5 rounds to 1 (ties away from zero)
    fp_32_64_div_32_32(&b, 1u, 2u);
    EXPECT_EQ(u64_mul_u32_fp32_64(1u, b), 1u, "1 * 0.5 ~= 1");

    // 1 * 1/3 ~= 0, 2 * 1/3 ~= 1, 3 * 1/3 = 1
    fp_32_64_div_32_32(&b, 1u, 3u);
    EXPECT_EQ(u64_mul_u32_fp32_64(1u, b), 0u, "1 * 1/3 ~= 0");
    EXPECT_EQ(u64_mul_u32_fp32_64(2u, b), 1u, "2 * 1/3 ~= 1");
    EXPECT_EQ(u64_mul_u32_fp32_64(3u, b), 1u, "3 * 1/3 = 1");

    END_TEST;
}

static bool test_u32_mul_u64_fp32_64(void) {
    BEGIN_TEST;

    struct fp_32_64 b;
    uint64_t a;

    // (2^32) * 0.5 = 2^31
    fp_32_64_div_32_32(&b, 1u, 2u);
    a = 0x0000000100000000ULL;
    EXPECT_EQ(u32_mul_u64_fp32_64(a, b), 0x80000000u, "2^32 * 0.5 = 2^31");

    // 65536 * 1.5 = 98304
    fp_32_64_div_32_32(&b, 3u, 2u);
    a = 1ULL << 16;
    EXPECT_EQ(u32_mul_u64_fp32_64(a, b), (uint32_t)((1u << 16) + (1u << 15)), "65536 * 1.5 = 98304");

    END_TEST;
}

static bool test_u64_mul_u64_fp32_64(void) {
    BEGIN_TEST;

    struct fp_32_64 b;
    uint64_t a;

    // (2^40) * 0.5 = 2^39
    fp_32_64_div_32_32(&b, 1u, 2u);
    a = 1ULL << 40;
    EXPECT_EQ(u64_mul_u64_fp32_64(a, b), (1ULL << 39), "2^40 * 0.5 = 2^39");

    // 100 * 0.7 ~= 70
    fp_32_64_div_32_32(&b, 7u, 10u);
    EXPECT_EQ(u64_mul_u64_fp32_64(100u, b), 70u, "100 * 0.7 ~= 70");

    END_TEST;
}

static bool test_time_conversion(void) {
    BEGIN_TEST;

    struct fp_32_64 tsc_to_timebase;
    struct fp_32_64 tsc_to_timebase_hires;
    struct fp_32_64 timebase_to_tsc;
    uint64_t tsc_hz = 2500000000ULL; // 2.5 GHz

    // Compute the ratio of TSC to timebase
    fp_32_64_div_32_32(&tsc_to_timebase, 1000, tsc_hz);
    fp_32_64_div_32_32(&tsc_to_timebase_hires, 1000000, tsc_hz);
    fp_32_64_div_32_32(&timebase_to_tsc, tsc_hz, 1000);

    // Verify some known conversions
    // 1 ms = 2,500,000 TSC ticks
    EXPECT_EQ(u64_mul_u32_fp32_64(1u, timebase_to_tsc), 2500000u, "1 ms to TSC ticks");

    // 1,000,000 TSC ticks = 0.4 ms
    EXPECT_EQ(u32_mul_u64_fp32_64(1000000u, tsc_to_timebase), 0u, "1,000,000 TSC ticks to ms (l0)");
    EXPECT_EQ(u32_mul_u64_fp32_64(1000000u, tsc_to_timebase_hires), 400u, "1,000,000 TSC ticks to ms (l32)");

    END_TEST;
}

BEGIN_TEST_CASE(fixed_point)
RUN_TEST(test_fp32_64_div_construct)
RUN_TEST(test_u64_mul_u32_fp32_64)
RUN_TEST(test_u32_mul_u64_fp32_64)
RUN_TEST(test_u64_mul_u64_fp32_64)
RUN_TEST(test_time_conversion)
END_TEST_CASE(fixed_point)

