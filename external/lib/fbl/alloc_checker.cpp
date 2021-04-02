// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fbl/alloc_checker.h>

#include <fbl/new.h>
#include <zircon/assert.h>
#include <zircon/compiler.h>

namespace fbl {
namespace {

enum : unsigned {
    alloc_armed = 1,
    alloc_ok = 2,
};

void panic_if_armed(unsigned state) {
#if LK_DEBUGLEVEL > 1
    if (state & alloc_armed)
        ZX_PANIC("AllocChecker::check() needs to be called\n");
#endif
}

void* checked(size_t size, AllocChecker* ac, void* mem) {
    ac->arm(size, mem != nullptr);
    return mem;
}

} // namespace

AllocChecker::AllocChecker()
    : state_(0u) {
}

AllocChecker::~AllocChecker() {
    panic_if_armed(state_);
}

void AllocChecker::arm(size_t size, bool result) {
    panic_if_armed(state_);
    state_ = alloc_armed |
             ((size == 0u) ? alloc_ok : (result ? alloc_ok : 0u));
}

bool AllocChecker::check() {
    state_ &= ~alloc_armed;
    return (state_ & alloc_ok) == alloc_ok;
}

// The std::nothrow_t overloads of operator new and operator new[] are
// the standard C++ library interfaces that return nullptr instead of
// using exceptions, i.e. the same semantics as malloc.  We define our
// checked versions in terms of those rather than calling malloc
// directly to maintain the invariant that only allocations done via
// new are freed via delete, only allocations done via new[] are freed
// via delete[], and only allocations done via the C malloc family
// functions are freed via the C free function.  The non-throwing
// operator new and operator new[] we call might be trivial ones like
// zxcpp's that actually just call malloc, or they might be ones that
// enforce this invariant (such as the ASan allocator).

} // namespace fbl

#if !_KERNEL

void* operator new(size_t size, fbl::AllocChecker* ac) noexcept {
    return fbl::checked(size, ac, operator new(size, std::nothrow_t()));
}

void* operator new[](size_t size, fbl::AllocChecker* ac) noexcept {
    return fbl::checked(size, ac, operator new[](size, std::nothrow_t()));
}

#else // _KERNEL

void* operator new(size_t size, fbl::AllocChecker* ac) noexcept {
    return fbl::checked(size, ac, operator new(size, __GET_CALLER(), std::nothrow_t()));
}

void* operator new[](size_t size, fbl::AllocChecker* ac) noexcept {
    return fbl::checked(size, ac, operator new[](size, __GET_CALLER(), std::nothrow_t()));
}

#endif // !_KERNEL
