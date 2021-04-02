// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

// When building with the standard C++ library available, use its declarations.
// Otherwise code that includes both this file and <new> might have conflicts.
#if __has_include(<new>)

#include <new>

// if we're linking with zxcpp, use its new implementation
#elif __has_include(<zxcpp/new.h>)

#include <zxcpp/new.h>

#else

// No standard library in this build, so declare them locally.

#include <stddef.h>

namespace std {
struct nothrow_t {};
} // namespace std

// Define placement allocation functions inline for optimal code generation.
inline void* operator new(size_t size, void* ptr) noexcept { return ptr; }
inline void* operator new[](size_t size, void* ptr) noexcept { return ptr; }

// Declare (but don't define) non-throwing allocation functions.
// Note: This library does not provide an implementation of these functions.
namespace std {
struct nothrow_t;
} // namespace std

#if !_KERNEL

void* operator new(size_t size, const std::nothrow_t&) noexcept;
void* operator new[](size_t size, const std::nothrow_t&) noexcept;

#else   //  _KERNEL

void* operator new(size_t size, void* caller, const std::nothrow_t&) noexcept;
void* operator new[](size_t size, void* caller, const std::nothrow_t&) noexcept;

#endif  //  !_KERNEL

#endif  //  __has_include(<new>)
