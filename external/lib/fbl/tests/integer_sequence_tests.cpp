// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fbl/integer_sequence.h>
#include <fbl/limits.h>
#include <fbl/type_support.h>
#include <stdint.h>
#include <unittest/unittest.h>

namespace {

template <typename T, T... Is>
T Max(fbl::integer_sequence<T, Is...>) {
    T sequence[sizeof...(Is)] = {Is...};
    T value = fbl::numeric_limits<T>::min();
    for (size_t i = 0; i < sizeof...(Is); i++) {
        if (sequence[i] > value)
            value = sequence[i];
    }
    return value;
}

template <typename T, T... Is>
T Min(fbl::integer_sequence<T, Is...>) {
    T sequence[sizeof...(Is)] = {Is...};
    T value = fbl::numeric_limits<T>::max();
    for (size_t i = 0; i < sizeof...(Is); i++) {
        if (sequence[i] < value)
            value = sequence[i];
    }
    return value;
}

template <typename T, size_t N>
bool bounds_test() {
    BEGIN_TEST;

    const T min = Min(fbl::make_integer_sequence<T, N>{});
    EXPECT_EQ(static_cast<T>(0), min);

    const T max = Max(fbl::make_integer_sequence<T, N>{});
    EXPECT_EQ(N - 1, max);

    const size_t size = fbl::make_integer_sequence<T, N>{}.size();
    EXPECT_EQ(static_cast<size_t>(N), size);

    END_TEST;
}

bool sequence_test() {
    BEGIN_TEST;

    {
        using A = fbl::make_integer_sequence<int8_t, 0>;
        using B = fbl::integer_sequence<int8_t>;
        EXPECT_TRUE((fbl::is_same<A, B>::value));
    }

    {
        using A = fbl::make_integer_sequence<int8_t, 1>;
        using B = fbl::integer_sequence<int8_t, 0>;
        EXPECT_TRUE((fbl::is_same<A, B>::value));
    }

    {
        using A = fbl::make_integer_sequence<int8_t, 2>;
        using B = fbl::integer_sequence<int8_t, 0, 1>;
        EXPECT_TRUE((fbl::is_same<A, B>::value));
    }

    {
        using A = fbl::make_integer_sequence<int8_t, 3>;
        using B = fbl::integer_sequence<int8_t, 0, 1, 2>;
        EXPECT_TRUE((fbl::is_same<A, B>::value));
    }

    {
        using A = fbl::make_integer_sequence<uint8_t, 0>;
        using B = fbl::integer_sequence<uint8_t>;
        EXPECT_TRUE((fbl::is_same<A, B>::value));
    }

    {
        using A = fbl::make_integer_sequence<uint8_t, 1>;
        using B = fbl::integer_sequence<uint8_t, 0>;
        EXPECT_TRUE((fbl::is_same<A, B>::value));
    }

    {
        using A = fbl::make_integer_sequence<uint8_t, 2>;
        using B = fbl::integer_sequence<uint8_t, 0, 1>;
        EXPECT_TRUE((fbl::is_same<A, B>::value));
    }

    {
        using A = fbl::make_integer_sequence<uint8_t, 3>;
        using B = fbl::integer_sequence<uint8_t, 0, 1, 2>;
        EXPECT_TRUE((fbl::is_same<A, B>::value));
    }

    {
        using A = fbl::make_integer_sequence<int, 0>;
        using B = fbl::integer_sequence<int>;
        EXPECT_TRUE((fbl::is_same<A, B>::value));
    }

    {
        using A = fbl::make_integer_sequence<int, 1>;
        using B = fbl::integer_sequence<int, 0>;
        EXPECT_TRUE((fbl::is_same<A, B>::value));
    }

    {
        using A = fbl::make_integer_sequence<int, 2>;
        using B = fbl::integer_sequence<int, 0, 1>;
        EXPECT_TRUE((fbl::is_same<A, B>::value));
    }

    {
        using A = fbl::make_integer_sequence<int, 3>;
        using B = fbl::integer_sequence<int, 0, 1, 2>;
        EXPECT_TRUE((fbl::is_same<A, B>::value));
    }

    {
        using A = fbl::make_index_sequence<0>;
        using B = fbl::index_sequence<>;
        using C = fbl::integer_sequence<size_t>;
        EXPECT_TRUE((fbl::is_same<A, B>::value));
        EXPECT_TRUE((fbl::is_same<B, C>::value));
    }

    {
        using A = fbl::make_index_sequence<1>;
        using B = fbl::index_sequence<0>;
        using C = fbl::integer_sequence<size_t, 0>;
        EXPECT_TRUE((fbl::is_same<A, B>::value));
        EXPECT_TRUE((fbl::is_same<B, C>::value));
    }

    {
        using A = fbl::make_index_sequence<2>;
        using B = fbl::index_sequence<0, 1>;
        using C = fbl::integer_sequence<size_t, 0, 1>;
        EXPECT_TRUE((fbl::is_same<A, B>::value));
        EXPECT_TRUE((fbl::is_same<B, C>::value));
    }

    {
        using A = fbl::make_index_sequence<3>;
        using B = fbl::index_sequence<0, 1, 2>;
        using C = fbl::integer_sequence<size_t, 0, 1, 2>;
        EXPECT_TRUE((fbl::is_same<A, B>::value));
        EXPECT_TRUE((fbl::is_same<B, C>::value));
    }

    END_TEST;
}

} // anonymous namespace

BEGIN_TEST_CASE(integer_sequence_tests)
RUN_NAMED_TEST("bounds_test<int8_t, 1>", (bounds_test<int8_t, 1>))
RUN_NAMED_TEST("bounds_test<int8_t, 2>", (bounds_test<int8_t, 2>))
RUN_NAMED_TEST("bounds_test<int8_t, 4>", (bounds_test<int8_t, 4>))
RUN_NAMED_TEST("bounds_test<int8_t, 8>", (bounds_test<int8_t, 8>))
RUN_NAMED_TEST("bounds_test<int8_t, 64>", (bounds_test<int8_t, 64>))
RUN_NAMED_TEST("bounds_test<int8_t, 127>", (bounds_test<int8_t, 127>))
RUN_NAMED_TEST("bounds_test<uint8_t, 1>", (bounds_test<uint8_t, 1>))
RUN_NAMED_TEST("bounds_test<uint8_t, 2>", (bounds_test<uint8_t, 2>))
RUN_NAMED_TEST("bounds_test<uint8_t, 4>", (bounds_test<uint8_t, 4>))
RUN_NAMED_TEST("bounds_test<uint8_t, 8>", (bounds_test<uint8_t, 8>))
RUN_NAMED_TEST("bounds_test<uint8_t, 64>", (bounds_test<uint8_t, 64>))
RUN_NAMED_TEST("bounds_test<uint8_t, 256>", (bounds_test<uint8_t, 256>))
RUN_NAMED_TEST("bounds_test<int, 1>", (bounds_test<int, 1>))
RUN_NAMED_TEST("bounds_test<int, 2>", (bounds_test<int, 2>))
RUN_NAMED_TEST("bounds_test<int, 4>", (bounds_test<int, 4>))
RUN_NAMED_TEST("bounds_test<int, 8>", (bounds_test<int, 8>))
RUN_NAMED_TEST("bounds_test<int, 64>", (bounds_test<int, 64>))
RUN_NAMED_TEST("bounds_test<int, 1024>", (bounds_test<int, 1024>))
RUN_NAMED_TEST("bounds_test<size_t, 1>", (bounds_test<size_t, 1>))
RUN_NAMED_TEST("bounds_test<size_t, 2>", (bounds_test<size_t, 2>))
RUN_NAMED_TEST("bounds_test<size_t, 4>", (bounds_test<size_t, 4>))
RUN_NAMED_TEST("bounds_test<size_t, 8>", (bounds_test<size_t, 8>))
RUN_NAMED_TEST("bounds_test<size_t, 64>", (bounds_test<size_t, 64>))
RUN_NAMED_TEST("bounds_test<size_t, 1024>", (bounds_test<size_t, 1024>))
RUN_NAMED_TEST("sequence_test", sequence_test)
END_TEST_CASE(integer_sequence_tests)
