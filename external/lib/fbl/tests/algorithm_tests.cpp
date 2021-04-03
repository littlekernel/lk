// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fbl/algorithm.h>

#include <fbl/limits.h>
#include <lib/unittest.h>

namespace {

// TODO(vtl): We use this because EXPECT_EQ() doesn't work well with functions that return a
// reference.
template <typename T>
T val(const T& x) {
    return x;
}

bool min_test() {
    BEGIN_TEST;

    EXPECT_EQ(val(fbl::min(1, 2)), 1);
    EXPECT_EQ(val(fbl::min(2.1, 1.1)), 1.1);
    EXPECT_EQ(val(fbl::min(1u, 1u)), 1u);

    END_TEST;
}

bool max_test() {
    BEGIN_TEST;

    EXPECT_EQ(val(fbl::max(1, 2)), 2);
    EXPECT_EQ(val(fbl::max(2.1, 1.1)), 2.1);
    EXPECT_EQ(val(fbl::max(1u, 1u)), 1u);

    END_TEST;
}

bool clamp_test() {
    BEGIN_TEST;

    EXPECT_EQ(val(fbl::clamp(1, 2, 6)), 2);
    EXPECT_EQ(val(fbl::clamp(2.1, 2.1, 6.1)), 2.1);
    EXPECT_EQ(val(fbl::clamp(3u, 2u, 6u)), 3u);
    EXPECT_EQ(val(fbl::clamp(6, 2, 6)), 6);
    EXPECT_EQ(val(fbl::clamp(7, 2, 6)), 6);

    EXPECT_EQ(val(fbl::clamp(1, 2, 2)), 2);
    EXPECT_EQ(val(fbl::clamp(2, 2, 2)), 2);
    EXPECT_EQ(val(fbl::clamp(3, 2, 2)), 2);

    END_TEST;
}

bool round_up_test() {
    BEGIN_TEST;

    EXPECT_EQ(fbl::round_up(0u, 1u), 0u);
    EXPECT_EQ(fbl::round_up(0u, 5u), 0u);
    EXPECT_EQ(fbl::round_up(5u, 5u), 5u);

    EXPECT_EQ(fbl::round_up(1u, 6u), 6u);
    EXPECT_EQ(fbl::round_up(6u, 1u), 6u);
    EXPECT_EQ(fbl::round_up(6u, 3u), 6u);
    EXPECT_EQ(fbl::round_up(6u, 4u), 8u);

    EXPECT_EQ(fbl::round_up(15u, 8u), 16u);
    EXPECT_EQ(fbl::round_up(16u, 8u), 16u);
    EXPECT_EQ(fbl::round_up(17u, 8u), 24u);
    EXPECT_EQ(fbl::round_up(123u, 100u), 200u);
    EXPECT_EQ(fbl::round_up(123456u, 1000u), 124000u);

    uint64_t large_int = fbl::numeric_limits<uint32_t>::max() + 1LLU;
    EXPECT_EQ(fbl::round_up(large_int, 64U), large_int);
    EXPECT_EQ(fbl::round_up(large_int, 64LLU), large_int);
    EXPECT_EQ(fbl::round_up(large_int + 63LLU, 64U), large_int + 64LLU);
    EXPECT_EQ(fbl::round_up(large_int + 63LLU, 64LLU), large_int + 64LLU);
    EXPECT_EQ(fbl::round_up(large_int, 3U), large_int + 2LLU);
    EXPECT_EQ(fbl::round_up(large_int, 3LLU), large_int + 2LLU);

    EXPECT_EQ(fbl::round_up(2U, large_int), large_int);
    EXPECT_EQ(fbl::round_up(2LLU, large_int), large_int);

    END_TEST;
}

bool round_down_test() {
    BEGIN_TEST;

    EXPECT_EQ(fbl::round_down(0u, 1u), 0u);
    EXPECT_EQ(fbl::round_down(0u, 5u), 0u);
    EXPECT_EQ(fbl::round_down(5u, 5u), 5u);

    EXPECT_EQ(fbl::round_down(1u, 6u), 0u);
    EXPECT_EQ(fbl::round_down(6u, 1u), 6u);
    EXPECT_EQ(fbl::round_down(6u, 3u), 6u);
    EXPECT_EQ(fbl::round_down(6u, 4u), 4u);

    EXPECT_EQ(fbl::round_down(15u, 8u), 8u);
    EXPECT_EQ(fbl::round_down(16u, 8u), 16u);
    EXPECT_EQ(fbl::round_down(17u, 8u), 16u);
    EXPECT_EQ(fbl::round_down(123u, 100u), 100u);
    EXPECT_EQ(fbl::round_down(123456u, 1000u), 123000u);

    uint64_t large_int = fbl::numeric_limits<uint32_t>::max() + 1LLU;
    EXPECT_EQ(fbl::round_down(large_int, 64U), large_int);
    EXPECT_EQ(fbl::round_down(large_int, 64LLU), large_int);
    EXPECT_EQ(fbl::round_down(large_int + 63LLU, 64U), large_int);
    EXPECT_EQ(fbl::round_down(large_int + 63LLU, 64LLU), large_int);
    EXPECT_EQ(fbl::round_down(large_int + 65LLU, 64U), large_int + 64LLU);
    EXPECT_EQ(fbl::round_down(large_int + 65LLU, 64LLU), large_int + 64LLU);
    EXPECT_EQ(fbl::round_down(large_int + 2LLU, 3U), large_int + 2LLU);
    EXPECT_EQ(fbl::round_down(large_int + 2LLU, 3LLU), large_int + 2LLU);

    EXPECT_EQ(fbl::round_down(2U, large_int), 0);
    EXPECT_EQ(fbl::round_down(2LLU, large_int), 0);

    END_TEST;
}

template <typename T>
bool is_pow2_test() {
    BEGIN_TEST;

    T val = 0;
    EXPECT_FALSE(fbl::is_pow2<T>(val));
    EXPECT_FALSE(fbl::is_pow2<T>(static_cast<T>(val - 1)));

    for (val = 1u; val; val = static_cast<T>(val << 1u)) {
        EXPECT_TRUE(fbl::is_pow2<T>(val));
        EXPECT_FALSE(fbl::is_pow2<T>(static_cast<T>(val - 5u)));
        EXPECT_FALSE(fbl::is_pow2<T>(static_cast<T>(val + 5u)));
    }

    END_TEST;
}

bool max_element_test() {
    BEGIN_TEST;
    int* null = nullptr;
    EXPECT_EQ(fbl::max_element(null, null), null);

    int value = 5;
    EXPECT_EQ(fbl::max_element(&value, &value), &value);

    int values[] = { 1, 3, 7, -2, 5, 7 };
    constexpr size_t count = fbl::count_of(values);
    int* first = values;
    int* last = values + count;
    EXPECT_EQ(fbl::max_element(first, last), values + 2);

    END_TEST;
}

bool max_compare(int a, int b) {
    return a > b;
}

bool max_element_compare_test() {
    BEGIN_TEST;
    int* null = nullptr;
    EXPECT_EQ(fbl::max_element(null, null, max_compare), null);

    int value = 5;
    EXPECT_EQ(fbl::max_element(&value, &value, max_compare), &value);

    int values[] = { 1, 3, 7, -2, 5, 7 };
    constexpr size_t count = fbl::count_of(values);
    int* first = values;
    int* last = values + count;
    EXPECT_EQ(fbl::max_element(first, last, max_compare), values + 2);

    END_TEST;
}

bool min_element_test() {
    BEGIN_TEST;
    int* null = nullptr;
    EXPECT_EQ(fbl::min_element(null, null), null);

    int value = 5;
    EXPECT_EQ(fbl::min_element(&value, &value), &value);

    int values[] = { 1, 3, -7, -2, 5, -7 };
    constexpr size_t count = fbl::count_of(values);
    int* first = values;
    int* last = values + count;
    EXPECT_EQ(fbl::min_element(first, last), values + 2);

    END_TEST;
}

bool min_compare(int a, int b) {
    return a < b;
}

bool min_element_compare_test() {
    BEGIN_TEST;
    int* null = nullptr;
    EXPECT_EQ(fbl::min_element(null, null, min_compare), null);

    int value = 5;
    EXPECT_EQ(fbl::min_element(&value, &value, min_compare), &value);

    int values[] = { 1, 3, -7, -2, 5, -7 };
    constexpr size_t count = fbl::count_of(values);
    int* first = values;
    int* last = values + count;
    EXPECT_EQ(fbl::min_element(first, last, min_compare), values + 2);

    END_TEST;
}

bool lower_bound_test() {
    BEGIN_TEST;

    int* null = nullptr;
    EXPECT_EQ(fbl::lower_bound(null, null, 0), null);

    int value = 5;
    EXPECT_EQ(fbl::lower_bound(&value, &value, 4), &value);
    EXPECT_EQ(fbl::lower_bound(&value, &value, 5), &value);
    EXPECT_EQ(fbl::lower_bound(&value, &value, 6), &value);

    EXPECT_EQ(fbl::lower_bound(&value, &value + 1, 4), &value);
    EXPECT_EQ(fbl::lower_bound(&value, &value + 1, 5), &value);
    EXPECT_EQ(fbl::lower_bound(&value, &value + 1, 6), &value + 1);

    constexpr size_t count = 4;
    int values[count] = { 1, 3, 5, 7 };
    int* first = values;
    int* last = values + count;

    EXPECT_EQ(*fbl::lower_bound(first, last, 0), 1);
    EXPECT_EQ(*fbl::lower_bound(first, last, 1), 1);
    EXPECT_EQ(*fbl::lower_bound(first, last, 2), 3);
    EXPECT_EQ(*fbl::lower_bound(first, last, 3), 3);
    EXPECT_EQ(*fbl::lower_bound(first, last, 4), 5);
    EXPECT_EQ(*fbl::lower_bound(first, last, 5), 5);
    EXPECT_EQ(*fbl::lower_bound(first, last, 6), 7);
    EXPECT_EQ(*fbl::lower_bound(first, last, 7), 7);
    EXPECT_EQ(fbl::lower_bound(first, last, 8), last);

    last = first - 1;
    EXPECT_EQ(fbl::lower_bound(first, first, 0), first);
    EXPECT_EQ(fbl::lower_bound(first, last, 0), last);

    END_TEST;
}

struct LessThan {
    bool operator()(const int& lhs, const int& rhs) {
        return lhs < rhs;
    }
};

bool lower_bound_compare_test() {
    BEGIN_TEST;

    LessThan lessThan;

    int* null = nullptr;
    EXPECT_EQ(fbl::lower_bound(null, null, 0, lessThan), null);

    int value = 5;
    EXPECT_EQ(fbl::lower_bound(&value, &value, 4, lessThan), &value);
    EXPECT_EQ(fbl::lower_bound(&value, &value, 5, lessThan), &value);
    EXPECT_EQ(fbl::lower_bound(&value, &value, 6, lessThan), &value);

    EXPECT_EQ(fbl::lower_bound(&value, &value + 1, 4, lessThan), &value);
    EXPECT_EQ(fbl::lower_bound(&value, &value + 1, 5, lessThan), &value);
    EXPECT_EQ(fbl::lower_bound(&value, &value + 1, 6, lessThan), &value + 1);

    constexpr size_t count = 4;
    int values[count] = { 1, 3, 5, 7 };
    int* first = values;
    int* last = values + count;

    EXPECT_EQ(*fbl::lower_bound(first, last, 0, lessThan), 1);
    EXPECT_EQ(*fbl::lower_bound(first, last, 1, lessThan), 1);
    EXPECT_EQ(*fbl::lower_bound(first, last, 2, lessThan), 3);
    EXPECT_EQ(*fbl::lower_bound(first, last, 3, lessThan), 3);
    EXPECT_EQ(*fbl::lower_bound(first, last, 4, lessThan), 5);
    EXPECT_EQ(*fbl::lower_bound(first, last, 5, lessThan), 5);
    EXPECT_EQ(*fbl::lower_bound(first, last, 6, lessThan), 7);
    EXPECT_EQ(*fbl::lower_bound(first, last, 7, lessThan), 7);
    EXPECT_EQ(fbl::lower_bound(first, last, 8, lessThan), last);

    last = first - 1;
    EXPECT_EQ(fbl::lower_bound(first, first, 0, lessThan), first);
    EXPECT_EQ(fbl::lower_bound(first, last, 0, lessThan), last);

    END_TEST;
}

template <typename T>
bool gcd_test() {
    BEGIN_TEST;

    // GCD(0,0) == 0
    T val0 = 0;
    EXPECT_EQ(fbl::gcd(val0, val0), val0);

    // GCD(1,1) == 1
    T val1 = 1;
    EXPECT_EQ(fbl::gcd(val1, val1), val1);

    // GCD(0,1) and GCD(1,0) == 1
    EXPECT_EQ(fbl::gcd(val0, val1), val1);
    EXPECT_EQ(fbl::gcd(val1, val0), val1);

    // GCD(0,X) and GCD(X,0) == X
    T val2 = fbl::numeric_limits<T>::max();
    T val3 = val2 / 8;
    EXPECT_EQ(fbl::gcd(val0, val2), val2);
    EXPECT_EQ(fbl::gcd(val3, val0), val3);

    // GCD(1,X) and GCD(X,1) == 1
    EXPECT_EQ(fbl::gcd(val1, val3), val1);
    EXPECT_EQ(fbl::gcd(val2, val1), val1);

    // GCD(X,X) == X
    EXPECT_EQ(fbl::gcd(val2, val2), val2);

    // Both inputs are close to the limit of T
    T val4 = static_cast<T>(val3 * 7);
    T val5 = static_cast<T>(val3 * 6);
    EXPECT_EQ(fbl::gcd(val4, val5), val3);

    // simple pragmatic usage, with audio frame rates
    if (sizeof(T) > 1) {
        // static_casts are only needed to support int types smaller than 32-bit
        T val441 = static_cast<T>(44100), val48 = static_cast<T>(48000);
        T expected = static_cast<T>(300);
        EXPECT_EQ(fbl::gcd(val441, val48), expected);
    }

    END_TEST;
}

// static_casts can be removed if this function need not support int types smaller than 32-bit
template <typename T>
bool lcm_test() {
    BEGIN_TEST;

    // LCM(0,0) == 0
    T val0 = 0;
    EXPECT_EQ(fbl::lcm(val0, val0), val0);

    // LCM(1,1) == 1
    T val1 = 1;
    EXPECT_EQ(fbl::lcm(val1, val1), val1);

    // LCM(0,1) and LCM(1,0) == 0
    EXPECT_EQ(fbl::lcm(val0, val1), val0);
    EXPECT_EQ(fbl::lcm(val1, val0), val0);

    // LCM(0,X) and LCM(X,0) == 0
    T val2 = fbl::numeric_limits<T>::max();
    T val3 = static_cast<T>(val2 >> (sizeof(T) * 4));
    EXPECT_EQ(fbl::lcm(val0, val3), val0);
    EXPECT_EQ(fbl::lcm(val2, val0), val0);

    // LCM(1,X) and LCM(X,1) == X
    EXPECT_EQ(fbl::lcm(val1, val2), val2);
    EXPECT_EQ(fbl::lcm(val3, val1), val3);

    // LCM(X,X) == X
    EXPECT_EQ(fbl::lcm(val2, val2), val2);

    // product (and LCM) of [val3, val4] are just under the capacity of T
    T val4 = static_cast<T>(val1 + val3);
    EXPECT_EQ(fbl::lcm(val3, val4), val3 * val4);

    // product exceeds capacity of T, but LCM doesn't, since val4 is power of 2
    T val5 = static_cast<T>(val3 << 2);
    EXPECT_EQ(fbl::lcm(val5, val4), val3 * val4);

    // simple pragmatic usage, with audio frame rates
    if (sizeof(T) > 2) {
        T val441 = static_cast<T>(44100), val24 = static_cast<T>(24000);
        T expected = static_cast<T>(3528000);
        EXPECT_EQ(fbl::lcm(val441, val24), expected);
    }

    END_TEST;
}

bool accumulate_test() {
    BEGIN_TEST;

    int* null = nullptr;
    EXPECT_EQ(fbl::accumulate(null, null, 42), 42);

    int value = 5;
    EXPECT_EQ(fbl::accumulate(&value, &value, 42), 42);

    int values[] = { 9, 11, 8, 12 };
    constexpr size_t count = fbl::count_of(values);
    int* first = values;
    int* last = values + count;
    EXPECT_EQ(fbl::accumulate(first, last, 2), 42);

    END_TEST;
}

int accumulate_op(int a, int b) {
    return a + b;
}

bool accumulate_op_test() {
    BEGIN_TEST;

    int* null = nullptr;
    EXPECT_EQ(fbl::accumulate(null, null, 42, accumulate_op), 42);

    int value = 5;
    EXPECT_EQ(fbl::accumulate(&value, &value, 42, accumulate_op), 42);

    int values[] = { 9, 11, 8, 12 };
    constexpr size_t count = fbl::count_of(values);
    int* first = values;
    int* last = values + count;
    EXPECT_EQ(fbl::accumulate(first, last, 2, accumulate_op), 42);

    END_TEST;
}
}  // namespace

BEGIN_TEST_CASE(algorithm_tests)
RUN_NAMED_TEST("min test", min_test)
RUN_NAMED_TEST("max test", max_test)
RUN_NAMED_TEST("clamp test", clamp_test)
RUN_NAMED_TEST("round_up test", round_up_test)
RUN_NAMED_TEST("round_down test", round_down_test)
RUN_NAMED_TEST("is_pow2<uint8_t>",  is_pow2_test<uint8_t>)
RUN_NAMED_TEST("is_pow2<uint16_t>", is_pow2_test<uint16_t>)
RUN_NAMED_TEST("is_pow2<uint32_t>", is_pow2_test<uint32_t>)
RUN_NAMED_TEST("is_pow2<uint64_t>", is_pow2_test<uint64_t>)
RUN_NAMED_TEST("is_pow2<size_t>",   is_pow2_test<size_t>)
RUN_NAMED_TEST("max_element test", max_element_test)
RUN_NAMED_TEST("max_element_compare test", max_element_compare_test)
RUN_NAMED_TEST("min_element test", min_element_test)
RUN_NAMED_TEST("min_element_compare test", min_element_compare_test)
RUN_NAMED_TEST("lower_bound test", lower_bound_test)
RUN_NAMED_TEST("lower_bound_compare test", lower_bound_compare_test)
RUN_NAMED_TEST("gcd_test<uint8_t>",  gcd_test<uint8_t>)
RUN_NAMED_TEST("gcd_test<uint16_t>", gcd_test<uint16_t>)
RUN_NAMED_TEST("gcd_test<uint32_t>", gcd_test<uint32_t>)
RUN_NAMED_TEST("gcd_test<uint64_t>", gcd_test<uint64_t>)
RUN_NAMED_TEST("gcd_test<size_t>",   gcd_test<size_t>)
RUN_NAMED_TEST("lcm_test<uint8_t>",  lcm_test<uint8_t>)
RUN_NAMED_TEST("lcm_test<uint16_t>", lcm_test<uint16_t>)
RUN_NAMED_TEST("lcm_test<uint32_t>", lcm_test<uint32_t>)
RUN_NAMED_TEST("lcm_test<uint64_t>", lcm_test<uint64_t>)
RUN_NAMED_TEST("lcm_test<size_t>",   lcm_test<size_t>)
RUN_NAMED_TEST("accumulate test", accumulate_test)
RUN_NAMED_TEST("accumulate_op test", accumulate_op_test)
END_TEST_CASE(algorithm_tests);
