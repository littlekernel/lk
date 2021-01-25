/*
 * Copyright (c) 2008-2014 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <limits.h> // for ULONG_MAX

typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;
typedef signed char        int8_t;
typedef short              int16_t;
typedef int                int32_t;
typedef long long          int64_t;

#define INT8_MIN    CHAR_MIN
#define INT16_MIN   SHRT_MIN
#define INT32_MIN   INT_MIN

#if defined(LLONG_MIN)
#define INT64_MIN   LLONG_MIN
#elif defined(__LONG_LONG_MAX__)
#define INT64_MIN (-__LONG_LONG_MAX__-1LL)
#endif

#define INT8_MAX    CHAR_MAX
#define INT16_MAX   SHRT_MAX
#define INT32_MAX   INT_MAX

#if defined(LLONG_MAX)
#define INT64_MAX   LLONG_MAX
#elif defined(__LONG_LONG_MAX__)
#define INT64_MAX  __LONG_LONG_MAX__
#endif

#define UINT8_MAX   UCHAR_MAX
#define UINT16_MAX  USHRT_MAX
#define UINT32_MAX  UINT_MAX

#if defined(ULLONG_MAX)
#define UINT64_MAX  ULLONG_MAX
#elif defined(__LONG_LONG_MAX__)
#define UINT64_MAX (__LONG_LONG_MAX__*2ULL + 1ULL)
#endif

typedef int8_t int_least8_t;
typedef int16_t int_least16_t;
typedef int32_t int_least32_t;
typedef int64_t int_least64_t;
typedef uint8_t uint_least8_t;
typedef uint16_t uint_least16_t;
typedef uint32_t uint_least32_t;
typedef uint64_t uint_least64_t;

#define INT_LEAST8_MIN  INT8_MIN
#define INT_LEAST16_MIN INT16_MIN
#define INT_LEAST32_MIN INT32_MIN
#define INT_LEAST64_MIN INT64_MIN
#define INT_LEAST8_MAX  INT8_MAX
#define INT_LEAST16_MAX INT16_MAX
#define INT_LEAST32_MAX INT32_MAX
#define INT_LEAST64_MAX INT64_MAX

#define UINT_LEAST8_MAX  UINT8_MAX
#define UINT_LEAST16_MAX UINT16_MAX
#define UINT_LEAST32_MAX UINT32_MAX
#define UINT_LEAST64_MAX UINT64_MAX

typedef int8_t int_fast8_t;
typedef int16_t int_fast16_t;
typedef int32_t int_fast32_t;
typedef int64_t int_fast64_t;
typedef uint8_t uint_fast8_t;
typedef uint16_t uint_fast16_t;
typedef uint32_t uint_fast32_t;
typedef uint64_t uint_fast64_t;

#define INT_FAST8_MIN  INT8_MIN
#define INT_FAST16_MIN INT16_MIN
#define INT_FAST32_MIN INT32_MIN
#define INT_FAST64_MIN INT64_MIN
#define INT_FAST8_MAX  INT8_MAX
#define INT_FAST16_MAX INT16_MAX
#define INT_FAST32_MAX INT32_MAX
#define INT_FAST64_MAX INT64_MAX

#define UINT_FAST8_MAX  UINT8_MAX
#define UINT_FAST16_MAX UINT16_MAX
#define UINT_FAST32_MAX UINT32_MAX
#define UINT_FAST64_MAX UINT64_MAX

typedef long intptr_t;
typedef unsigned long uintptr_t;

#define INTPTR_MIN        LONG_MIN
#define INTPTR_MAX        LONG_MAX
#define UINTPTR_MAX       ULONG_MAX

typedef long long intmax_t;
typedef unsigned long long uintmax_t;

#define INTMAX_MAX        LLONG_MAX
#define INTMAX_MIN        LLONG_MIN
#define UINTMAX_MAX       ULLONG_MAX

#define SIZE_MAX ULONG_MAX

#define INT8_C(c)         (c)
#define INT16_C(c)        (c)
#define INT32_C(c)        (c)
#define INT64_C(c)        (c ## LL)

#define UINT8_C(c)        (c)
#define UINT16_C(c)       (c)
#define UINT32_C(c)       (c ## U)
#define UINT64_C(c)       (c ## ULL)

#define INTMAX_C(c)       INT64_C(c)
#define UINTMAX_C(c)      UINT64_C(c)

