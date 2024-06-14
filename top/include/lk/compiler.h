//
// Copyright (c) 2008-2013 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
//
// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#define __CONCAT1(x, y) x ## y
#define __CONCAT(x, y) __CONCAT1(x, y)

#ifndef __ASSEMBLY__

#if __GNUC__ || __clang__
#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)
#define __UNUSED __attribute__((__unused__))
#if __clang__
// Per https://clang.llvm.org/docs/AttributeReference.html#used
// __used__ does not prevent linkers from removing unused sections
// (if --gc-sections is passed). Need to specify "retain" as well.
#define __USED __attribute__((__used__, retain))
#else
#define __USED __attribute__((__used__))
#endif
#define __PACKED __attribute__((packed))
#define __ALIGNED(x) __attribute__((aligned(x)))
#define __PRINTFLIKE(__fmt,__varargs) __attribute__((__format__ (__printf__, __fmt, __varargs)))
#define __SCANFLIKE(__fmt,__varargs) __attribute__((__format__ (__scanf__, __fmt, __varargs)))
#define __SECTION(x) __USED __attribute((section(x)))
#define __PURE __attribute((pure))
#define __CONST __attribute((const))
#define __NO_RETURN __attribute__((noreturn))
#define __MALLOC __attribute__((malloc))
#define __WEAK __attribute__((weak))
#define __GNU_INLINE __attribute__((gnu_inline))
#define __GET_CALLER(x) __builtin_return_address(0)
#define __GET_FRAME(x) __builtin_frame_address(0)
#define __NAKED __attribute__((naked))
#define __ISCONSTANT(x) __builtin_constant_p(x)
#define __NO_INLINE __attribute((noinline))
#define __SRAM __NO_INLINE __SECTION(".sram.text")
#define __CONSTRUCTOR __attribute__((constructor))
#define __DESTRUCTOR __attribute__((destructor))
#define __RESTRICT __restrict

#define INCBIN(symname, sizename, filename, section)                    \
    __asm__ (".section " section "; .balign 4; .globl "#symname);       \
    __asm__ (""#symname ":\n.incbin \"" filename "\"");                 \
    __asm__ (".balign 1; "#symname "_end:");                            \
    __asm__ (".balign 4; .globl "#sizename);                            \
    __asm__ (""#sizename ": .long "#symname "_end - "#symname);         \
    __asm__ (".previous");                                              \
    extern unsigned char symname[];                                     \
    extern unsigned int sizename

#define INCFILE(symname, sizename, filename) INCBIN(symname, sizename, filename, ".rodata")

/* look for gcc 3.0 and above */
#if (__GNUC__ > 3) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 0)
#define __ALWAYS_INLINE __attribute__((always_inline))
#else
#define __ALWAYS_INLINE
#endif

/* look for gcc 3.1 and above */
#if !defined(__DEPRECATED) // seems to be built in in some versions of the compiler
#if (__GNUC__ > 3) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1)
#define __DEPRECATED __attribute((deprecated))
#else
#define __DEPRECATED
#endif
#endif

/* look for gcc 3.3 and above */
#if (__GNUC__ > 3) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 3)
/* the may_alias attribute was introduced in gcc 3.3; before that, there
 * was no way to specify aliasiang rules on a type-by-type basis */
#define __MAY_ALIAS __attribute__((may_alias))

/* nonnull was added in gcc 3.3 as well */
#define __NONNULL(x) __attribute((nonnull x))
#else
#define __MAY_ALIAS
#define __NONNULL(x)
#endif

/* look for gcc 3.4 and above */
#if (__GNUC__ > 3) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)
#define __WARN_UNUSED_RESULT __attribute((warn_unused_result))
#else
#define __WARN_UNUSED_RESULT
#endif

#if defined(__clang__)
/* Clang does not support externally_visible, used should be similar. */
#define __EXTERNALLY_VISIBLE __attribute__((used))
#elif (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 1)
#define __EXTERNALLY_VISIBLE __attribute__((externally_visible))
#else
#define __EXTERNALLY_VISIBLE
#endif

#if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5) || defined(__clang__)
#define __UNREACHABLE __builtin_unreachable()
#else
#define __UNREACHABLE
#endif

#if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6) || defined(__clang__)
#ifdef __cplusplus
#define STATIC_ASSERT(e) static_assert(e, #e)
#else
#define STATIC_ASSERT(e) _Static_assert(e, #e)
#endif
#else
#define STATIC_ASSERT(e) extern char (*ct_assert(void)) [sizeof(char[1 - 2*!(e)])]
#endif

/* compiler fence */
#define CF do { __asm__ volatile("" ::: "memory"); } while(0)

#define __WEAK_ALIAS(x) __attribute__((weak, alias(x)))
#define __ALIAS(x) __attribute__((alias(x)))

#define __EXPORT __attribute__ ((visibility("default")))
#define __LOCAL  __attribute__ ((visibility("hidden")))

#define __THREAD __thread

#define __offsetof(type, field) __builtin_offsetof(type, field)

#if defined(__cplusplus) && __cplusplus >= 201703L
#define __FALLTHROUGH [[fallthrough]]
#elif defined(__cplusplus) && defined(__clang__)
#define __FALLTHROUGH [[clang::fallthrough]]
#elif __GNUC__ >= 7
#define __FALLTHROUGH __attribute__((__fallthrough__))
#else
#define __FALLTHROUGH do {} while (0)
#endif

#ifndef __has_attribute
#define __has_attribute(x)  0
#endif
#ifndef __has_extension
#define __has_extension     __has_feature
#endif
#ifndef __has_feature
#define __has_feature(x)    0
#endif
#ifndef __has_include
#define __has_include(x)    0
#endif
#ifndef __has_builtin
#define __has_builtin(x)    0
#endif

#ifndef __clang__
#define __LEAF_FN __attribute__((__leaf__))
#define __OPTIMIZE(x) __attribute__((optimize(x)))
#define __THREAD_ANNOTATION(x)
#else
#define __LEAF_FN
#define __OPTIMIZE(x)
#define __THREAD_ANNOTATION(x) __attribute__((x))
#endif

// Publicly exposed thread annotation macros. These have a long and ugly name to
// minimize the chance of collision with consumers of Zircon's public headers.
#define __TA_CAPABILITY(x) __THREAD_ANNOTATION(__capability__(x))
#define __TA_GUARDED(x) __THREAD_ANNOTATION(__guarded_by__(x))
#define __TA_ACQUIRE(...) __THREAD_ANNOTATION(__acquire_capability__(__VA_ARGS__))
#define __TA_TRY_ACQUIRE(...) __THREAD_ANNOTATION(__try_acquire_capability__(__VA_ARGS__))
#define __TA_ACQUIRED_BEFORE(...) __THREAD_ANNOTATION(__acquired_before__(__VA_ARGS__))
#define __TA_ACQUIRED_AFTER(...) __THREAD_ANNOTATION(__acquired_after__(__VA_ARGS__))
#define __TA_RELEASE(...) __THREAD_ANNOTATION(__release_capability__(__VA_ARGS__))
#define __TA_REQUIRES(...) __THREAD_ANNOTATION(__requires_capability__(__VA_ARGS__))
#define __TA_EXCLUDES(...) __THREAD_ANNOTATION(__locks_excluded__(__VA_ARGS__))
#define __TA_RETURN_CAPABILITY(x) __THREAD_ANNOTATION(__lock_returned__(x))
#define __TA_SCOPED_CAPABILITY __THREAD_ANNOTATION(__scoped_lockable__)
#define __TA_NO_THREAD_SAFETY_ANALYSIS __THREAD_ANNOTATION(__no_thread_safety_analysis__)

#else

#define likely(x)       (x)
#define unlikely(x)     (x)
#define __UNUSED
#define __PACKED
#define __ALIGNED(x)
#define __PRINTFLIKE(__fmt,__varargs)
#define __SCANFLIKE(__fmt,__varargs)
#define __SECTION(x)
#define __PURE
#define __CONST
#define __NONNULL(x)
#define __DEPRECATED
#define __WARN_UNUSED_RESULT
#define __ALWAYS_INLINE
#define __MAY_ALIAS
#define __NO_RETURN
#endif

#endif

/* TODO: add type check */
#if !defined(countof)
#define countof(a) (sizeof(a) / sizeof((a)[0]))
#endif

/* CPP header guards */
#ifdef __cplusplus
#define __BEGIN_CDECLS  extern "C" {
#define __END_CDECLS    }
#else
#define __BEGIN_CDECLS
#define __END_CDECLS
#endif
