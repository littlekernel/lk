//
// Copyright (c) 2008 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
#pragma once

#include <stddef.h>

// Fake std::nothrow_t without a standard C++ library.
namespace std {
    struct nothrow_t {};
} // namespace std

void* operator new(size_t, const std::nothrow_t&) noexcept;
void* operator new[](size_t, const std::nothrow_t&) noexcept;

inline void* operator new(size_t, void *ptr) noexcept { return ptr; }
inline void* operator new[](size_t, void *ptr) noexcept { return ptr; }

void operator delete(void *p);
void operator delete[](void *p);
void operator delete(void *p, size_t);
void operator delete[](void *p, size_t);

// vim: syntax=cpp
