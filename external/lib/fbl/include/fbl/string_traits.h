// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This header provides functions which make it easier to work generically
// with string-like objects such as fbl::StringPiece, fbl::String, std::string,
// and std::string_view.

#pragma once

#include <stddef.h>

#include <fbl/type_support.h>

namespace fbl {
namespace internal {
DECLARE_HAS_MEMBER_FN_WITH_SIGNATURE(has_data, data, const char* (C::*)() const);
DECLARE_HAS_MEMBER_FN_WITH_SIGNATURE(has_length, length, size_t (C::*)() const);
} // namespace internal

// Gets the character data from a string-like object.
template <typename T>
constexpr const char* GetStringData(const T& value) {
    return value.data();
}

// Gets the length (in characters) of a string-like object.
template <typename T>
constexpr size_t GetStringLength(const T& value) {
    return value.length();
}

// is_string_like<T>
//
// Evaluates to true_type if GetStringData() and GetStringLength() are supported
// instances of type T.
template <typename T>
using is_string_like = integral_constant<bool,
                                         internal::has_data<T>::value &&
                                             internal::has_length<T>::value>;

} // namespace fbl
