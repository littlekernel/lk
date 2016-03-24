/*  $NetBSD: cdefs.h,v 1.58 2004/12/11 05:59:00 christos Exp $  */

/*
 * Copyright (c) 1991, 1993
 *  The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Berkeley Software Design, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *  @(#)cdefs.h 8.8 (Berkeley) 1/9/95
 */

/*
 * This is a minimum version of cdefs.h for LK, this might be
 * modified if necessary
 */
#ifndef _SYS_CDEFS_H_
#define _SYS_CDEFS_H_

/*-
 * Deal with _ANSI_SOURCE:
 * If it is defined, and no other compilation environment is explicitly
 * requested, then define our internal feature-test macros to zero.  This
 * makes no difference to the preprocessor (undefined symbols in preprocessing
 * expressions are defined to have value zero), but makes it more convenient for
 * a test program to print out the values.
 *
 * If a program mistakenly defines _ANSI_SOURCE and some other macro such as
 * _POSIX_C_SOURCE, we will assume that it wants the broader compilation
 * environment (and in fact we will never get here).
 */
#if defined(_ANSI_SOURCE)   /* Hide almost everything. */
#define __POSIX_VISIBLE     0
#define __XSI_VISIBLE       0
#define __BSD_VISIBLE       0
#define __ISO_C_VISIBLE     1990
#elif defined(_C99_SOURCE)  /* Localism to specify strict C99 env. */
#define __POSIX_VISIBLE     0
#define __XSI_VISIBLE       0
#define __BSD_VISIBLE       0
#define __ISO_C_VISIBLE     1999
#else               /* Default environment: show everything. */
#define __POSIX_VISIBLE     200809
#define __XSI_VISIBLE       700
#define __BSD_VISIBLE       1
#define __ISO_C_VISIBLE     1999
#endif

/*
 * Default values.
 */
#ifndef __XPG_VISIBLE
# define __XPG_VISIBLE          700
#endif
#ifndef __POSIX_VISIBLE
# define __POSIX_VISIBLE        200809
#endif
#ifndef __ISO_C_VISIBLE
# define __ISO_C_VISIBLE        1999
#endif
#ifndef __BSD_VISIBLE
# define __BSD_VISIBLE          1
#endif


/*
 * Some of the FreeBSD sources used in Bionic need this.
 * Originally, this is used to embed the rcs versions of each source file
 * in the generated binary. We certainly don't want this in Bionic.
 */
#define __FBSDID(s) /* nothing */

#define __pure2 __attribute__((__const__)) /* Android-added: used by FreeBSD libm */

/*
 * Macro to test if we're using a GNU C compiler of a specific vintage
 * or later, for e.g. features that appeared in a particular version
 * of GNU C.  Usage:
 *
 *  #if __GNUC_PREREQ__(major, minor)
 *  ...cool feature...
 *  #else
 *  ...delete feature...
 *  #endif
 */
#ifdef __GNUC__
#define __GNUC_PREREQ__(x, y)                       \
    ((__GNUC__ == (x) && __GNUC_MINOR__ >= (y)) ||          \
     (__GNUC__ > (x)))
#else
#define __GNUC_PREREQ__(x, y)   0
#endif

#if defined(__cplusplus)
#define __BEGIN_DECLS       extern "C" {
#define __END_DECLS     }
#define __static_cast(x,y)  static_cast<x>(y)
#else
#define __BEGIN_DECLS
#define __END_DECLS
#define __static_cast(x,y)  (x)y
#endif

#if defined(__cplusplus)
#define __inline    inline      /* convert to C++ keyword */
#else
#if !defined(__GNUC__) && !defined(__lint__)
#define __inline            /* delete GCC keyword */
#endif /* !__GNUC__  && !__lint__ */
#endif /* !__cplusplus */

#if __GNUC_PREREQ__(3, 1)
#define __always_inline __attribute__((__always_inline__))
#else
#define __always_inline
#endif



#endif /* !_SYS_CDEFS_H_ */
