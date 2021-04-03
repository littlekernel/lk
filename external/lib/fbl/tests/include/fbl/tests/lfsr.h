// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <stdint.h>
#include <fbl/type_support.h>

namespace fbl {
namespace tests {

namespace internal {

// A templated implementation of a linear feedback shift register for various
// core state sizes.  With proper selection of the generator, the LFSR will be a
// maximum-cycle LFSR meaning that it will cycle through all of its core states
// (except for all zeros) exactly once before repeating.
template <typename CoreType, CoreType generator>
class Lfsr {
public:
    static_assert(is_unsigned_integer<CoreType>::value,
                 "LFSR core type must be an unsigned integer!");

    constexpr explicit Lfsr(CoreType initial_core) : core_(initial_core) { }

    template <typename T>
    void SetCore(T val) {
        static_assert(is_unsigned_integer<T>::value,
                     "LFSR initializer type must be an unsigned integer!");
        core_ = static_cast<CoreType>(val);
    };

    CoreType PeekCore() const {
        return core_;
    }

    CoreType GetNext() {
        CoreType ret  = 0u;
        CoreType flag = 1u;

        for (size_t i = 0; i < (sizeof(size_t) << 3); ++i) {
            bool bit = core_ & 1u;
            core_ = static_cast<CoreType>(core_ >> 1u);
            if (bit) {
                core_ ^= generator;
                ret   |= flag;
            }

            flag = static_cast<CoreType>(flag << 1u);
        }

        return ret;
    }

private:
    CoreType core_;
};

}  // namespace internal

// User-facing implementation of LFSRs of various sizes with pre-selected
// maximum-cycle generators.
template <typename T, typename Enable = void>
class Lfsr;

// MAKE_LFSR
//
// Temporary macro which deals with the boilerplate declaration of an LFSR of a
// particular core size with a particular generator, exposing the constructor in
// the process.
#define MAKE_LFSR(_bits, _gen)                                                  \
template <typename T>                                                           \
class Lfsr<T,                                                                   \
           typename enable_if<is_unsigned_integer<T>::value &&                  \
                              ((sizeof(T) << 3) == _bits)>::type                \
          > : public internal::Lfsr<uint ## _bits ## _t, _gen> {                \
public:                                                                         \
    using CoreType = uint ## _bits ## _t;                                       \
                                                                                \
    template <typename U>                                                       \
    constexpr explicit Lfsr(U initial_core)                                     \
        : internal::Lfsr<CoreType, _gen>(static_cast<CoreType>(initial_core)) { \
        static_assert(is_unsigned_integer<U>::value,                            \
                     "LFSR initializer type must be an unsigned integer!");     \
    }                                                                           \
                                                                                \
    constexpr Lfsr() : internal::Lfsr<CoreType, _gen>(1u) { }                   \
}

MAKE_LFSR(8,  0xB8);
MAKE_LFSR(16, 0xB400);
MAKE_LFSR(32, 0xA3000000);
MAKE_LFSR(64, 0xD800000000000000);

#undef MAKE_LFSR

}  // namespace tests
}  // namespace fbl
