// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <string.h>

#include <fbl/string_traits.h>

namespace fbl {

constexpr static size_t constexpr_strlen(const char* str) {
#if defined(_MSC_VER)
#error "__builtin_strlen not defined for MSVC++"
#else
    return __builtin_strlen(str);
#endif
}

// A string-like object that points to a sized piece of memory.
//
// fbl::StringPiece is designed to resemble std::string_view.
//
// |length()| does NOT include a trailing NUL and no guarantee is made that
// you can check |data()[length()]| to see if a NUL is there.
//
// Basically, these aren't C strings, don't think otherwise.
// The string piece does not own the data it points to.
class StringPiece {
public:
    constexpr StringPiece()
        : data_(nullptr), length_(0u) {}

    constexpr StringPiece(const char* data)
        : data_(data), length_(data != nullptr ? constexpr_strlen(data) : 0u) {}

    constexpr StringPiece(const char* data, size_t length)
        : data_(data), length_(length) {}

    constexpr StringPiece(const StringPiece& other) = default;
    constexpr StringPiece(StringPiece&& other) = default;

    // Creates a string piece from a string-like object.
    //
    // Works with various string types including fbl::String, fbl::StringView,
    // std::string, and std::string_view.
    template <typename T, typename = typename enable_if<is_string_like<T>::value>::type>
    constexpr StringPiece(const T& value)
        : StringPiece(GetStringData(value), GetStringLength(value)) {}

    constexpr const char* data() const { return data_; }

    constexpr size_t length() const { return length_; }
    constexpr size_t size() const { return length_; }
    constexpr bool empty() const { return length_ == 0u; }

    constexpr const char* begin() const { return data_; }
    constexpr const char* cbegin() const { return data_; }
    constexpr const char* end() const { return data_ + length_; }
    constexpr const char* cend() const { return data_ + length_; }

    constexpr const char& operator[](size_t pos) const { return data_[pos]; }

    int compare(const StringPiece& other) const;

    void clear() {
        data_ = nullptr;
        length_ = 0u;
    }

    constexpr StringPiece& operator=(const StringPiece& other) = default;
    constexpr StringPiece& operator=(StringPiece&& other) = default;

    template <typename T, typename = typename enable_if<is_string_like<T>::value>::type>
    constexpr StringPiece& operator=(const T& value) {
        set(GetStringData(value), GetStringLength(value));
        return *this;
    }

    void set(const char* data) {
        data_ = data;
        length_ = data != nullptr ? constexpr_strlen(data) : 0u;
    }

    void set(const char* data, size_t length) {
        data_ = data;
        length_ = length;
    }

private:
    // Pointer to string data, not necessarily null terminated.
    const char* data_;

    // Length of the string data.
    size_t length_;
};

bool operator==(const StringPiece& lhs, const StringPiece& rhs);

inline bool operator!=(const StringPiece& lhs, const StringPiece& rhs) {
    return !(lhs == rhs);
}

inline bool operator<(const StringPiece& lhs, const StringPiece& rhs) {
    return lhs.compare(rhs) < 0;
}

inline bool operator>(const StringPiece& lhs, const StringPiece& rhs) {
    return lhs.compare(rhs) > 0;
}

inline bool operator<=(const StringPiece& lhs, const StringPiece& rhs) {
    return lhs.compare(rhs) <= 0;
}

inline bool operator>=(const StringPiece& lhs, const StringPiece& rhs) {
    return lhs.compare(rhs) >= 0;
}

} // namespace fbl
