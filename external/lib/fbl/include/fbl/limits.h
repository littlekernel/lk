// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <limits.h>
#include <fbl/type_support.h>

namespace fbl {

template <class T>
class numeric_limits {};

#define SPECIALIZE_INT(type, sign, modulo, digits_, min_, max_)                                    \
    template <>                                                                                    \
    class numeric_limits<type> {                                                                   \
    public:                                                                                        \
        static constexpr bool is_specialized = true;                                               \
        static constexpr bool is_signed = (sign);                                                  \
        static constexpr bool is_integer = true;                                                   \
        static constexpr bool is_exact = true;                                                     \
        static constexpr bool has_infinity = false;                                                \
        static constexpr bool has_quiet_NaN = false;                                               \
        static constexpr bool has_signaling_NaN = false;                                           \
        static constexpr bool has_denorm = false;                                                  \
        static constexpr bool has_denorm_loss = false;                                             \
        static constexpr bool round_style = false;                                                 \
        static constexpr bool is_iec559 = false;                                                   \
        static constexpr bool is_bounded = true;                                                   \
        static constexpr bool is_modulo = (modulo);                                                \
        static constexpr int digits = (digits_);                                                   \
        static constexpr int digits10 = (digits_) * 3 / 10;                                        \
        static constexpr int max_digits10 = 0;                                                     \
        static constexpr int radix = 2;                                                            \
        static constexpr int min_exponent = 0;                                                     \
        static constexpr int min_exponent10 = 0;                                                   \
        static constexpr int max_exponent = 0;                                                     \
        static constexpr int max_exponent10 = 0;                                                   \
        static constexpr bool traps = false;                                                       \
        static constexpr bool tinyness_before = false;                                             \
        static constexpr type min() {                                                              \
            return (min_);                                                                         \
        }                                                                                          \
        static constexpr type lowest() {                                                           \
            return (min_);                                                                         \
        }                                                                                          \
        static constexpr type max() {                                                              \
            return (max_);                                                                         \
        }                                                                                          \
        static constexpr type epsilon() {                                                          \
            return 0;                                                                              \
        }                                                                                          \
        static constexpr type round_error() {                                                      \
            return 0;                                                                              \
        }                                                                                          \
        static constexpr type infinity() {                                                         \
            return 0;                                                                              \
        }                                                                                          \
        static constexpr type quiet_NaN() {                                                        \
            return 0;                                                                              \
        }                                                                                          \
        static constexpr type signaling_NaN() {                                                    \
            return 0;                                                                              \
        }                                                                                          \
        static constexpr type denorm_min() {                                                       \
            return 0;                                                                              \
        }                                                                                          \
    }

#define SPECIALIZE_SIGNED(type, min, max)                                                          \
    SPECIALIZE_INT(type, true, false, CHAR_BIT * sizeof(type) - 1, min, max)

#define SPECIALIZE_UNSIGNED(type, min, max)                                                        \
    SPECIALIZE_INT(type, false, true, CHAR_BIT * sizeof(type), min, max)

SPECIALIZE_INT(bool, false, true, 1, false, true);
#if CHAR_MIN == 0
SPECIALIZE_SIGNED(char, CHAR_MIN, CHAR_MAX);
#else
SPECIALIZE_UNSIGNED(char, CHAR_MIN, CHAR_MAX);
#endif
SPECIALIZE_SIGNED(signed char, SCHAR_MIN, SCHAR_MAX);
SPECIALIZE_UNSIGNED(unsigned char, 0, UCHAR_MAX);

SPECIALIZE_SIGNED(short, SHRT_MIN, SHRT_MAX);
SPECIALIZE_UNSIGNED(unsigned short, 0, USHRT_MAX);
SPECIALIZE_SIGNED(int, INT_MIN, INT_MAX);
SPECIALIZE_UNSIGNED(unsigned int, 0, UINT_MAX);
SPECIALIZE_SIGNED(long, LONG_MIN, LONG_MAX);
SPECIALIZE_UNSIGNED(unsigned long, 0, ULONG_MAX);
SPECIALIZE_SIGNED(long long, LLONG_MIN, LLONG_MAX);
SPECIALIZE_UNSIGNED(unsigned long long, 0, ULLONG_MAX);

#undef SPECIALIZE_SIGNED
#undef SPECIALIZE_UNSIGNED
#undef SPECIALIZE_INT

}  // namespace fbl
