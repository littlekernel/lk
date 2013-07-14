/*
 * Copyright (c) 2008-2013 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef __COMPILER_H
#define __COMPILER_H

#ifndef __ASSEMBLY__

#if __GNUC__
#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)
#define __UNUSED __attribute__((__unused__))
#define __PACKED __attribute__((packed))
#define __ALIGNED(x) __attribute__((aligned(x)))
#define __PRINTFLIKE(__fmt,__varargs) __attribute__((__format__ (__printf__, __fmt, __varargs)))
#define __SCANFLIKE(__fmt,__varargs) __attribute__((__format__ (__scanf__, __fmt, __varargs)))
#define __SECTION(x) __attribute((section(x)))
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

#define INCBIN(symname, sizename, filename, section)                    \
    __asm__ (".section " section "; .align 4; .globl "#symname);        \
    __asm__ (""#symname ":\n.incbin \"" filename "\"");                 \
    __asm__ (".section " section "; .align 1;");                        \
    __asm__ (""#symname "_end:");                                       \
    __asm__ (".section " section "; .align 4; .globl "#sizename);       \
    __asm__ (""#sizename ": .long "#symname "_end - "#symname " - 1");  \
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

#if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 1)
#define __EXTERNALLY_VISIBLE __attribute__((externally_visible))
#else
#define __EXTERNALLY_VISIBLE
#endif

#if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5)
#define __UNREACHABLE __builtin_unreachable()
#else
#define __UNREACHABLE
#endif

#if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#define STATIC_ASSERT(e) _Static_assert(e, #e)
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
#define countof(a) (sizeof(a) / sizeof((a)[0]))

/* macro-expanding concat */
#define concat(a, b) __ex_concat(a, b)
#define __ex_concat(a, b) a ## b

/* CPP header guards */
#ifdef __cplusplus
#define __BEGIN_CDECLS  extern "C" {
#define __END_CDECLS    }
#else
#define __BEGIN_CDECLS
#define __END_CDECLS
#endif

#endif
