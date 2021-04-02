// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <stdint.h>

#include <zircon/assert.h>

namespace fbl {

namespace internal {

static constexpr bool magic_validate(const char* str) {
    return
            str[0] != '\0' &&
            str[1] != '\0' &&
            str[2] != '\0' &&
            str[3] != '\0' &&
            str[4] == '\0';
}

} // namespace internal

// Function for generating canary magic values from strings
static constexpr uint32_t magic(const char* str) {
    if (!internal::magic_validate(str))
        return UINT32_MAX;
    uint32_t res = 0;
    for (size_t i = 0; i < 4; ++i) {
        res = (res << 8) + str[i];
    }
    return res;
}

// An embeddable structure guard.  To use this, choose a 4-byte guard value.
//
// If the value is ASCII, you can use fbl::magic() to convert it to a 32-bit
// integer.  Note that the string *must* be exactly 4-bytes, excluding the
// trailing nul-byte.  This is checked by a static_assert in Canary.
// For this example, we'll use the guard string "guar".  Add a member of type
// fbl::Canary<fbl::magic("guar")> to the class you'd like to guard.  It will
// automatically initialize itself with the guard value during construction and
// check it during destruction.  You can check the value during the lifetime of
// your object by calling the Assert() method.
//
// If the value is not ASCII, you can directly use an integer literal to
// instantiate the class.  The value must be storable in a uint32_t.  For this
// example, we'll use 0x12345678.  Add a member of type fbl::Canary<0x12345678>
// to the class you'd like to guard.  As above, it will be automatically checked
// on destruction and can be manually asserted against.
template <uint32_t magic>
class Canary {
public:
    static_assert(magic < UINT32_MAX, "Invalid canary value, must not be UINT32_MAX");

    constexpr Canary() : magic_(magic) { }

    ~Canary() {
        Assert();
        magic_ = 0;
    }

    void Assert() const {
        ZX_DEBUG_ASSERT_MSG(magic_ == magic,
                            "Invalid canary (expt: %08x, got: %08x)\n",
                            static_cast<uint32_t>(magic), magic_);
    }

private:
     volatile uint32_t magic_;
};

}  // namespace fbl
