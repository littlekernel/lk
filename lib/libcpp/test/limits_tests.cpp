//
// Copyright (c) 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

// Tests for lib/libcpp's <limits>. Mostly compile-time.

#include <lib/unittest.h>
#include <limits>
#include <stddef.h>
#include <stdint.h>

namespace {

using std::numeric_limits;

static_assert(numeric_limits<int>::is_specialized);
static_assert(!numeric_limits<void *>::is_specialized);

static_assert(numeric_limits<bool>::digits == 1);
static_assert(numeric_limits<bool>::max() == true);

static_assert(numeric_limits<int>::max() == INT32_MAX);
static_assert(numeric_limits<int>::min() == INT32_MIN);
static_assert(numeric_limits<int>::lowest() == numeric_limits<int>::min());
static_assert(numeric_limits<int>::digits == 31);
static_assert(numeric_limits<int>::digits10 == 9);
static_assert(numeric_limits<int>::is_signed);
static_assert(numeric_limits<int>::is_integer);
static_assert(!numeric_limits<int>::is_modulo);

static_assert(numeric_limits<unsigned int>::max() == UINT32_MAX);
static_assert(numeric_limits<unsigned int>::digits == 32);
static_assert(numeric_limits<unsigned int>::is_modulo);
static_assert(!numeric_limits<unsigned int>::is_signed);

static_assert(numeric_limits<signed char>::min() == -128);
static_assert(numeric_limits<unsigned char>::max() == 255);
static_assert(numeric_limits<long long>::max() == INT64_MAX);
static_assert(numeric_limits<unsigned long long>::max() == UINT64_MAX);

static_assert(numeric_limits<char16_t>::max() == 65535);
static_assert(numeric_limits<char32_t>::max() == 4294967295U);
static_assert(numeric_limits<wchar_t>::max() > 0);

// cv-qualified variants forward to the base specialization
static_assert(numeric_limits<const int>::max() == numeric_limits<int>::max());
static_assert(numeric_limits<volatile unsigned>::is_modulo);

static_assert(numeric_limits<float>::is_specialized);
static_assert(numeric_limits<float>::is_iec559);
static_assert(numeric_limits<float>::digits == 24);
static_assert(numeric_limits<float>::max_digits10 == 9);
static_assert(numeric_limits<float>::infinity() > numeric_limits<float>::max());
static_assert(numeric_limits<float>::lowest() == -numeric_limits<float>::max());
static_assert(numeric_limits<float>::denorm_min() > 0.0f);
static_assert(numeric_limits<float>::denorm_min() < numeric_limits<float>::min());

static_assert(numeric_limits<double>::digits == 53);
static_assert(numeric_limits<double>::max_digits10 == 17);
static_assert(numeric_limits<double>::epsilon() > 0.0);
static_assert(numeric_limits<double>::quiet_NaN() != numeric_limits<double>::quiet_NaN());

// clang/x86 kernel TUs build without x87 support and reject any use of long
// double, so the specialization is only present (and only checked) where the
// type is usable; float-compiled TUs (LK_FLOAT_TU) get it everywhere.
#if !(defined(__clang__) && (defined(__i386__) || defined(__x86_64__)) && !defined(LK_FLOAT_TU))
static_assert(numeric_limits<long double>::is_specialized);
#endif

bool limits_smoke() {
    BEGIN_TEST;

    // spot-check a few values at runtime as well
    EXPECT_EQ(INT32_MAX, numeric_limits<int>::max(), "");
    EXPECT_TRUE(numeric_limits<size_t>::max() == SIZE_MAX, "");

    END_TEST;
}

BEGIN_TEST_CASE(libcpp_limits_tests)
RUN_TEST(limits_smoke)
END_TEST_CASE(libcpp_limits_tests)

} // namespace
