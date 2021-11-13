//
// Copyright (c) 2008 Travis Geiselbrecht
// Copyright 2016 The Fuchsia Authors
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
//
#pragma once

#include <lk/compiler.h>
#include <lk/debug.h>

// Assert that |x| is true, else panic.
//
// ASSERT is always enabled and |x| will be evaluated regardless of any build arguments.
#define ASSERT(x)                          \
  do {                                     \
    if (unlikely(!(x))) {                  \
      assert_fail(__FILE__, __LINE__, #x); \
    }                                      \
  } while (0)

// Assert that |x| is true, else panic with the given message.
//
// ASSERT_MSG is always enabled and |x| will be evaluated regardless of any build arguments.
#define ASSERT_MSG(x, msg, msgargs...)                         \
  do {                                                         \
    if (unlikely(!(x))) {                                      \
      assert_fail_msg(__FILE__, __LINE__, #x, msg, ##msgargs); \
    }                                                          \
  } while (0)


// DEBUG_ASSERT_IMPLEMENTED is intended to be used to conditionalize code that is logically part of
// a debug assert. It's useful for performing complex consistency checks that are difficult to work
// into a DEBUG_ASSERT statement.
#define DEBUG_ASSERT_IMPLEMENTED (LK_DEBUGLEVEL > 1)

// Assert that |x| is true, else panic.
//
// Depending on build arguments, DEBUG_ASSERT may or may not be enabled. When disabled, |x| will not
// be evaluated.
#define DEBUG_ASSERT(x)                               \
  do {                                                \
    if (DEBUG_ASSERT_IMPLEMENTED && unlikely(!(x))) { \
      assert_fail(__FILE__, __LINE__, #x);            \
    }                                                 \
  } while (0)

// Assert that |x| is true, else panic with the given message.
//
// Depending on build arguments, DEBUG_ASSERT_MSG may or may not be enabled. When disabled, |x| will
// not be evaluated.
#define DEBUG_ASSERT_MSG(x, msg, msgargs...)                   \
  do {                                                         \
    if (DEBUG_ASSERT_IMPLEMENTED && unlikely(!(x))) {          \
      assert_fail_msg(__FILE__, __LINE__, #x, msg, ##msgargs); \
    }                                                          \
  } while (0)

// implement _COND versions of DEBUG_ASSERT which only emit the body if
// DEBUG_ASSERT_IMPLEMENTED is set
#if DEBUG_ASSERT_IMPLEMENTED
#define DEBUG_ASSERT_COND(x) DEBUG_ASSERT(x)
#define DEBUG_ASSERT_MSG_COND(x, msg, msgargs...) DEBUG_ASSERT_MSG(x, msg, msgargs)
#else
#define DEBUG_ASSERT_COND(x) \
  do {                       \
  } while (0)
#define DEBUG_ASSERT_MSG_COND(x, msg, msgargs...) \
  do {                                            \
  } while (0)
#endif

// make sure static_assert() is defined, even in C
#if !defined(__cplusplus) && !defined(static_assert)
#define static_assert(e, msg) _Static_assert(e, msg)
#endif

// Use DEBUG_ASSERT or ASSERT instead.
//
// assert() is defined only because third-party code used in the kernel expects it.
#define assert(x) DEBUG_ASSERT(x)

__BEGIN_CDECLS

// The following functions are called when an assert fails.
void assert_fail(const char *file, int line, const char *expression) __NO_RETURN __NO_INLINE;
void assert_fail_msg(const char *file, int line, const char *expression, const char *fmt,
                     ...) __PRINTFLIKE(4, 5) __NO_RETURN __NO_INLINE;

__END_CDECLS
