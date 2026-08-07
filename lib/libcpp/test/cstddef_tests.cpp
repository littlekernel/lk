//
// Copyright (c) 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

// Tests for the c-wrapper headers, chiefly <cstddef>'s std::byte.

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <lib/unittest.h>

namespace {

constexpr std::byte kB1{0x12};
constexpr std::byte kB2{0x0f};
static_assert(std::to_integer<int>(kB1 & kB2) == 0x02);
static_assert(std::to_integer<int>(kB1 | kB2) == 0x1f);
static_assert(std::to_integer<int>(kB1 ^ kB1) == 0);
static_assert(std::to_integer<unsigned>(kB1 << 1) == 0x24);
static_assert(std::to_integer<unsigned>(kB1 >> 4) == 0x01);
static_assert(std::to_integer<int>(~std::byte{0}) == 0xff);

static_assert(sizeof(std::uint64_t) == 8);
static_assert(sizeof(std::intptr_t) == sizeof(void *));

bool byte_compound_ops() {
    BEGIN_TEST;

    std::byte b{0x0f};
    b <<= 4;
    EXPECT_EQ(0xf0, std::to_integer<int>(b), "");
    b |= std::byte{0x01};
    EXPECT_EQ(0xf1, std::to_integer<int>(b), "");
    b &= std::byte{0x0f};
    EXPECT_EQ(0x01, std::to_integer<int>(b), "");
    b ^= std::byte{0x01};
    EXPECT_EQ(0, std::to_integer<int>(b), "");

    END_TEST;
}

bool std_qualified_libc() {
    BEGIN_TEST;

    char buf[8];
    std::memset(buf, 'a', sizeof(buf));
    EXPECT_EQ('a', buf[7], "");
    EXPECT_EQ(0, std::memcmp(buf, buf, sizeof(buf)), "");

    END_TEST;
}

BEGIN_TEST_CASE(libcpp_cstddef_tests)
RUN_TEST(byte_compound_ops)
RUN_TEST(std_qualified_libc)
END_TEST_CASE(libcpp_cstddef_tests)

} // namespace
