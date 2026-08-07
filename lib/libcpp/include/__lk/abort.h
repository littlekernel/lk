//
// Copyright (c) 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
#pragma once

// Internal support header for LK's freestanding C++ standard library subset.
//
// The kernel is built with -fno-exceptions, so where a hosted standard
// library implementation would throw, this implementation aborts instead:
//
//  - __LK_CXX_PANIC(msg): used for hard contract violations that a hosted
//    implementation reports by throwing (std::optional::value() on an empty
//    optional, out of range substr/at, allocation failure in the throwing
//    operator new). Always enabled, at every DEBUG level.
//
//  - __LK_CXX_DEBUG_ASSERT(cond): used for conditions that are plain
//    undefined behavior even in hosted implementations (operator[] out of
//    range, front() on an empty view). Compiled out at DEBUG=0, mirroring
//    the kernel's DEBUG_ASSERT policy.
//
// The abort routine is out of line so std headers do not need to pull in
// lk/debug.h.

extern "C" [[noreturn]] void __lk_cxx_abort(const char *msg);

#define __LK_CXX_PANIC(msg) ::__lk_cxx_abort(msg)

#if LK_DEBUGLEVEL > 0
#define __LK_CXX_DEBUG_ASSERT(cond) \
    ((cond) ? (void)0 : ::__lk_cxx_abort("C++ DEBUG ASSERT FAILED: " #cond))
#else
#define __LK_CXX_DEBUG_ASSERT(cond) ((void)0)
#endif

// vim: syntax=cpp
