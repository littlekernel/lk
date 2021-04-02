// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <fbl/type_support.h>
#include <stdlib.h>
#include <zircon/compiler.h>

namespace fbl {

// Utility to capture a sequence of integers of type T.
template <typename T, T... Is>
struct integer_sequence {
    static_assert(is_integral<T>::value, "T must be an integral type!");

    using type = integer_sequence;
    using value_type = T;
    static constexpr size_t size() { return sizeof...(Is); }
};

namespace internal {

#if __has_feature(__make_integer_seq)

// Builds an integer_sequence<T, T... Is> with Is = [0, N) using a compiler builtin template.
template <typename T, size_t N>
struct build_integer_sequence {
    using type = __make_integer_seq<integer_sequence, T, N>;
};

#else

// Merges two integer sequences, renumbering the second sequence to immediately follow the first.
template <typename T, typename S1, typename S2>
struct merge_sequence_and_renumber;

template <typename T, T... I1, T... I2>
struct merge_sequence_and_renumber<T, integer_sequence<T, I1...>,
                                   integer_sequence<T, I2...>>
    : integer_sequence<T, I1..., (sizeof...(I1) + I2)...> {};

// Builds an integer_sequence<T, T... Is> with Is = [0, N) using log N recursive instantiations.
template <typename T, size_t N, typename = void>
struct build_integer_sequence
    : merge_sequence_and_renumber<
          T, typename build_integer_sequence<T, N / 2>::type,
          typename build_integer_sequence<T, N - N / 2>::type> {
    static_assert(is_integral<T>::value, "T must be an integral type!");
    static_assert(0 <= N, "N must not be negative!");
};

// Identity sequences that terminate the recursive case.
template <typename T, size_t N>
struct build_integer_sequence<T, N, typename enable_if<N == 0>::type>
    : integer_sequence<T> {};

template <typename T, size_t N>
struct build_integer_sequence<T, N, typename enable_if<N == 1>::type>
    : integer_sequence<T, 0> {};

#endif

} // namespace internal

// Makes an integer_sequence<T, T... Is> with Is = [0, N).
template <typename T, size_t N>
using make_integer_sequence = typename internal::build_integer_sequence<T, N>::type;

// Alias of integer_sequence<T, Is...> with T = size_t.
template <size_t... Is>
using index_sequence = integer_sequence<size_t, Is...>;

// Makes an integer_sequence<T, T... Is> with T = size_t and Is = [0, N).
template <size_t N>
using make_index_sequence = make_integer_sequence<size_t, N>;

// Converts any type parameter pack into an index sequence of the same length.
template <typename... T>
using index_sequence_for = make_index_sequence<sizeof...(T)>;

} // namespace fbl
