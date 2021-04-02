// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <stdlib.h>

namespace fbl {

template <typename T>
struct default_delete {
    inline void operator()(T* ptr) const {
        enum { type_must_be_complete = sizeof(T) };
        delete ptr;
    }
};

template <typename T>
struct default_delete<T[]> {
    inline void operator()(T* ptr) const {
        enum { type_must_be_complete = sizeof(T) };
        delete[] ptr;
    }

private:
    // Disable this whenever U != T as C++14 and before do. See
    // http://cplusplus.github.io/LWG/lwg-defects.html#938 for motivation.
    // C++17 has more complex rules for when U(*)[] is implicitly convertible to
    // T(*)[] but we don't do that.
    template <typename U>
    void operator()(U* ptr) const = delete;
};

template <typename T, size_t n>
struct default_delete<T[n]> {
    // Disallow things like unique_ptr<int[10]>.
    static_assert(sizeof(T) == -1, "do not use array with size as type in fbl pointers");
};

} // namespace fbl
