//
// Copyright (c) 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
#pragma once

#include <stddef.h>

// Internal support header for LK's freestanding C++ standard library subset.
//
// Primary templates for the tuple protocol, shared by <utility> (std::pair)
// and <array>. Specializing these (plus providing get<I>) is what makes
// structured bindings work on a type. There is no full <tuple> in this
// library; only the protocol machinery lives here.

namespace std {

// Hoist the core stddef types into std here so that every header building on
// the tuple protocol (<utility>, <array>) exposes std::size_t the way hosted
// implementations do in practice. <cstddef> re-declares these, which is fine.
using ::size_t;
using ::ptrdiff_t;

template <typename T>
struct tuple_size;

template <size_t I, typename T>
struct tuple_element;

template <size_t I, typename T>
using tuple_element_t = typename tuple_element<I, T>::type;

template <typename T>
struct tuple_size<const T> : tuple_size<T> {};

template <size_t I, typename T>
struct tuple_element<I, const T> {
    using type = const typename tuple_element<I, T>::type;
};

} // namespace std

// vim: syntax=cpp
