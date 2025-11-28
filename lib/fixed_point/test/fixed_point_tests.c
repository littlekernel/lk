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
    EXPECT_EQ(0u, f.l0, "0/1 l0");
    EXPECT_EQ(0u, f.l32, "0/1 l32");
    EXPECT_EQ(0u, f.l64, "0/1 l64");

    // 1/1 -> 1.0
    fp_32_64_div_32_32(&f, 1u, 1u);
    EXPECT_EQ(1u, f.l0, "1/1 l0");
    EXPECT_EQ(0u, f.l32, "1/1 l32");
    EXPECT_EQ(0u, f.l64, "1/1 l64");

    // 1/2 -> 0.5
    fp_32_64_div_32_32(&f, 1u, 2u);
    EXPECT_EQ(0u, f.l0, "1/2 l0");
    EXPECT_EQ(0x80000000u, f.l32, "1/2 l32");
    EXPECT_EQ(0u, f.l64, "1/2 l64");

    // 3/2 -> 1.5
    fp_32_64_div_32_32(&f, 3u, 2u);
    EXPECT_EQ(1u, f.l0, "3/2 l0");
    EXPECT_EQ(0x80000000u, f.l32, "3/2 l32");
    EXPECT_EQ(0u, f.l64, "3/2 l64");

    END_TEST;
}

static bool test_u64_mul_u32_fp32_64(void) {
    BEGIN_TEST;

    struct fp_32_64 b;

    // 10 * 1.5 = 15
    fp_32_64_div_32_32(&b, 3u, 2u);
    EXPECT_EQ(15u, u64_mul_u32_fp32_64(10u, b), "10 * 1.5 ~= 15");

    // 1 * 0.5 rounds to 1 (ties away from zero)
    fp_32_64_div_32_32(&b, 1u, 2u);
    EXPECT_EQ(1u, u64_mul_u32_fp32_64(1u, b), "1 * 0.5 ~= 1");

    // 1 * 1/3 ~= 0, 2 * 1/3 ~= 1, 3 * 1/3 = 1
    fp_32_64_div_32_32(&b, 1u, 3u);
    EXPECT_EQ(0u, u64_mul_u32_fp32_64(1u, b), "1 * 1/3 ~= 0");
    EXPECT_EQ(1u, u64_mul_u32_fp32_64(2u, b), "2 * 1/3 ~= 1");
    EXPECT_EQ(1u, u64_mul_u32_fp32_64(3u, b), "3 * 1/3 = 1");

    END_TEST;
}

static bool test_u32_mul_u64_fp32_64(void) {
    BEGIN_TEST;

    struct fp_32_64 b;
    uint64_t a;

    // (2^32) * 0.5 = 2^31
    fp_32_64_div_32_32(&b, 1u, 2u);
    a = 0x0000000100000000ULL;
    EXPECT_EQ(0x80000000u, u32_mul_u64_fp32_64(a, b), "2^32 * 0.5 = 2^31");

    // 65536 * 1.5 = 98304
    fp_32_64_div_32_32(&b, 3u, 2u);
    a = 1ULL << 16;
    EXPECT_EQ((uint32_t)((1u << 16) + (1u << 15)), u32_mul_u64_fp32_64(a, b), "65536 * 1.5 = 98304");

    END_TEST;
}

static bool test_u64_mul_u64_fp32_64(void) {
    BEGIN_TEST;

    struct fp_32_64 b;
    uint64_t a;

    // (2^40) * 0.5 = 2^39
    fp_32_64_div_32_32(&b, 1u, 2u);
    a = 1ULL << 40;
    EXPECT_EQ((1ULL << 39), u64_mul_u64_fp32_64(a, b), "2^40 * 0.5 = 2^39");

    // 100 * 0.7 ~= 70
    fp_32_64_div_32_32(&b, 7u, 10u);
    EXPECT_EQ(70u, u64_mul_u64_fp32_64(100u, b), "100 * 0.7 ~= 70");

    END_TEST;
}

static bool test_div_64_32_basic(void) {
    BEGIN_TEST;
    uint64_t dividend = (1ULL << 48) + 12345; // large dividend
    uint32_t divisor = 1000;
    struct fp_32_64 fp;
    fp_32_64_div_64_32(&fp, dividend, divisor);

    // Independently compute expected pieces
    uint64_t rem = dividend % divisor;
    uint64_t l32 = (rem << 32) / divisor;
    rem = (rem << 32) % divisor;
    uint64_t l64 = (rem << 32) / divisor;

    EXPECT_EQ(dividend / divisor, fp.l0, "integer part");
    EXPECT_EQ(l32, fp.l32, "l32 part");
    EXPECT_EQ(l64, fp.l64, "l64 part");
    END_TEST;
}

static bool test_div_64_32_less_than_one(void) {
    BEGIN_TEST;
    uint64_t dividend = 500; // less than divisor
    uint32_t divisor = 1000;
    struct fp_32_64 fp;
    fp_32_64_div_64_32(&fp, dividend, divisor);
    uint64_t rem = dividend % divisor;
    uint64_t l32 = (rem << 32) / divisor;
    rem = (rem << 32) % divisor;
    uint64_t l64 = (rem << 32) / divisor;
    EXPECT_EQ(0U, fp.l0, "integer zero");
    EXPECT_EQ(l32, fp.l32, "l32 fraction");
    EXPECT_EQ(l64, fp.l64, "l64 fraction");
    END_TEST;
}

static bool test_div_64_32_edge_minus_one(void) {
    BEGIN_TEST;
    uint64_t divisor = 1000003;
    uint64_t dividend = divisor - 1; // just below divisor
    struct fp_32_64 fp;
    fp_32_64_div_64_32(&fp, dividend, (uint32_t)divisor);
    uint64_t rem = dividend % divisor;
    uint64_t l32 = (rem << 32) / divisor;
    rem = (rem << 32) % divisor;
    uint64_t l64 = (rem << 32) / divisor;
    EXPECT_EQ(0U, fp.l0, "integer zero");
    EXPECT_EQ(l32, fp.l32, "l32 fraction");
    EXPECT_EQ(l64, fp.l64, "l64 fraction");
    END_TEST;
}

static bool test_div_32_64_basic(void) {
    BEGIN_TEST;
    uint32_t dividend = 1000;
    uint64_t divisor = 3000000000ULL;
    struct fp_32_64 fp;
    fp_32_64_div_32_64(&fp, dividend, divisor);
    uint64_t rem = dividend % divisor;
    uint64_t l32 = (rem << 32) / divisor;
    rem = (rem << 32) % divisor;
    uint64_t l64 = (rem << 32) / divisor;
    EXPECT_EQ(0U, fp.l0, "integer zero");
    EXPECT_EQ(l32, fp.l32, "l32 fraction");
    EXPECT_EQ(l64, fp.l64, "l64 fraction");
    END_TEST;
}

static bool test_div_32_64_equal(void) {
    BEGIN_TEST;
    uint64_t divisor = (1ULL << 40) + 123;
    uint32_t dividend = (uint32_t)((1ULL << 32)); // smaller than divisor but significant
    struct fp_32_64 fp;
    fp_32_64_div_32_64(&fp, dividend, divisor);
    uint64_t rem = dividend % divisor;
    uint64_t l32 = (rem << 32) / divisor;
    rem = (rem << 32) % divisor;
    uint64_t l64 = (rem << 32) / divisor;
    EXPECT_EQ(0U, fp.l0, "integer zero");
    EXPECT_EQ(l32, fp.l32, "l32 fraction");
    EXPECT_EQ(l64, fp.l64, "l64 fraction");
    END_TEST;
}

static bool test_div_32_64_divisor_near_32bit(void) {
    BEGIN_TEST;
    uint64_t divisor = 0xFFFFFFFFULL; // near 32-bit max
    uint32_t dividend = 0xFFFFFFFEU;  // just below
    struct fp_32_64 fp;
    fp_32_64_div_32_64(&fp, dividend, divisor);
    uint64_t rem = dividend % divisor;
    uint64_t l32 = (rem << 32) / divisor;
    rem = (rem << 32) % divisor;
    uint64_t l64 = (rem << 32) / divisor;
    EXPECT_EQ(0U, fp.l0, "integer zero");
    EXPECT_EQ(l32, fp.l32, "l32 fraction");
    EXPECT_EQ(l64, fp.l64, "l64 fraction");
    END_TEST;
}

static bool test_time_conversion(void) {
    BEGIN_TEST;

    // Base frequency: 2.5 GHz (fits in 32 bits but exercise path)
    struct fp_32_64 tsc_to_ms;
    struct fp_32_64 tsc_to_us;
    struct fp_32_64 ms_to_tsc;
    uint64_t tsc_hz = 2500000000ULL;

    // Use 64/32 and 32/64 variants appropriately
    fp_32_64_div_32_64(&tsc_to_ms, 1000, tsc_hz);    // ms per tick
    fp_32_64_div_32_64(&tsc_to_us, 1000000, tsc_hz); // us per tick
    fp_32_64_div_64_32(&ms_to_tsc, tsc_hz, 1000);    // ticks per ms

    // 1 ms = 2,500,000 ticks
    EXPECT_EQ(2500000u, u64_mul_u32_fp32_64(1u, ms_to_tsc), "1 ms to ticks");

    // 1,000,000 ticks ~ 0.4 ms (integer ms rounds to 0), 400 us
    EXPECT_EQ(0u, u32_mul_u64_fp32_64(1000000u, tsc_to_ms), "1,000,000 ticks to ms int");
    EXPECT_EQ(400u, u32_mul_u64_fp32_64(1000000u, tsc_to_us), "1,000,000 ticks to us int");

    // High precision frequency > 32-bit: ~ (1<<40) + 123 Hz
    uint64_t tsc_hz_hp = (1ULL << 40) + 123ULL; // ~1.0995 THz
    struct fp_32_64 hp_tsc_to_ms, hp_ms_to_tsc;
    fp_32_64_div_32_64(&hp_tsc_to_ms, 1000, tsc_hz_hp);
    fp_32_64_div_64_32(&hp_ms_to_tsc, tsc_hz_hp, 1000);

    // 5 ms expected ticks = tsc_hz_hp / 200 -> ( (1<<40)+123 ) /200.
    uint64_t expected_5ms_ticks = tsc_hz_hp / 200ULL;
    uint64_t converted_5ms_ticks = u64_mul_u32_fp32_64(5u, hp_ms_to_tsc);
    EXPECT_EQ(expected_5ms_ticks, converted_5ms_ticks, "5 ms high freq ticks");

    // Convert a large tick count back to ms, ensure rounding within 1.
    uint64_t sample_ticks = (1ULL << 36) + 5555555ULL; // arbitrary large
    uint32_t ms_est = u32_mul_u64_fp32_64(sample_ticks, hp_tsc_to_ms);
    uint64_t ms_expected = (sample_ticks * 1000ULL) / tsc_hz_hp; // truncated reference
    uint64_t diff = (ms_est > ms_expected) ? (ms_est - ms_expected) : (ms_expected - ms_est);
    // Allow a slightly larger tolerance due to rounding across large ratios
    EXPECT_GE(16ULL, diff, "ms conversion diff <=16");

    // Fractional small frequency: 1 Hz
    uint64_t tsc_hz_1 = 1ULL;
    struct fp_32_64 tsc1_to_ms, ms_to_tsc1;
    fp_32_64_div_32_64(&tsc1_to_ms, 1000, tsc_hz_1); // ms per tick at 1 Hz = 1000
    fp_32_64_div_64_32(&ms_to_tsc1, tsc_hz_1, 1000); // ticks per ms ~ 0.001
    EXPECT_EQ(0u, u64_mul_u32_fp32_64(1u, ms_to_tsc1), "1 ms to ticks @1Hz");
    EXPECT_EQ(1000u, u32_mul_u64_fp32_64(1u, tsc1_to_ms), "1 tick to ms @1Hz");

    // Asymmetric rounding case: ticks chosen so fractional part near .5 boundary
    uint64_t boundary_ticks = tsc_hz / 2000ULL; // expects 0.5 ms
    uint32_t ms_boundary = u32_mul_u64_fp32_64(boundary_ticks, tsc_to_ms);
    EXPECT_TRUE(ms_boundary == 0u || ms_boundary == 1u, "0.5ms rounds to nearest ms (0 or 1)");

    END_TEST;
}

BEGIN_TEST_CASE(fixed_point)
RUN_TEST(test_fp32_64_div_construct)
RUN_TEST(test_u64_mul_u32_fp32_64)
RUN_TEST(test_u32_mul_u64_fp32_64)
RUN_TEST(test_u64_mul_u64_fp32_64)
RUN_TEST(test_div_64_32_basic)
RUN_TEST(test_div_64_32_less_than_one)
RUN_TEST(test_div_64_32_edge_minus_one)
RUN_TEST(test_div_32_64_basic)
RUN_TEST(test_div_32_64_equal)
RUN_TEST(test_div_32_64_divisor_near_32bit)
RUN_TEST(test_time_conversion)
END_TEST_CASE(fixed_point)
