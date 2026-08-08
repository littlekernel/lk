//
// Copyright (c) 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

// Tests for lib/libcpp's <span>.

#include <lib/unittest.h>
#include <span>

namespace {

constexpr int kArr[4] = {1, 2, 3, 4};

// CTAD from a C array gives a fixed extent
constexpr std::span kSpan{kArr};
static_assert(std::is_same_v<decltype(kSpan), const std::span<const int, 4>>);
static_assert(kSpan.extent == 4);
static_assert(kSpan.size() == 4);
static_assert(kSpan.size_bytes() == 4 * sizeof(int));
static_assert(!kSpan.empty());
static_assert(kSpan[0] == 1 && kSpan[3] == 4);
static_assert(kSpan.front() == 1 && kSpan.back() == 4);
static_assert(*kSpan.begin() == 1);
static_assert(kSpan.end() - kSpan.begin() == 4);

// subviews preserve static extents where possible
static_assert(kSpan.first<2>().extent == 2);
static_assert(kSpan.first<2>()[1] == 2);
static_assert(kSpan.last<2>()[0] == 3);
static_assert(kSpan.subspan<1>().extent == 3);
static_assert(kSpan.subspan<1>()[0] == 2);
static_assert(kSpan.subspan<1, 2>().extent == 2);
static_assert(kSpan.subspan<1, 2>()[1] == 3);

// dynamic-extent spans
constexpr std::span<const int> kDyn{kArr, 3};
static_assert(kDyn.extent == std::dynamic_extent);
static_assert(kDyn.size() == 3);
static_assert(kDyn.first(2).size() == 2);
static_assert(kDyn.last(2)[0] == 2);
static_assert(kDyn.subspan(1).size() == 2);
static_assert(kDyn.subspan(1, 1)[0] == 2);

// default construction
static_assert(std::span<int>().empty());
static_assert(std::span<int, 0>().empty());

// remaining observers
static_assert(*kSpan.cbegin() == 1);
static_assert(kSpan.cend() - kSpan.cbegin() == 4);
static_assert(kSpan.data() == kArr);
static_assert(kDyn.first<2>().extent == 2, "templated subview of a dynamic span");
static_assert(kDyn.last<1>()[0] == 3);
static_assert(kDyn.subspan<1>().size() == 2);

bool span_runtime() {
    BEGIN_TEST;

    int buf[5] = {10, 20, 30, 40, 50};

    // from pointer + size
    std::span<int> s(buf, 5);
    EXPECT_EQ(5U, s.size(), "");
    EXPECT_EQ(10, s[0], "");

    // mutation through the span
    s[0] = 11;
    EXPECT_EQ(11, buf[0], "");

    // from pointer pair
    std::span<int> s2(buf + 1, buf + 4);
    EXPECT_EQ(3U, s2.size(), "");
    EXPECT_EQ(20, s2.front(), "");

    // from std::array
    std::array<int, 3> arr{{7, 8, 9}};
    std::span<int, 3> s3(arr);
    EXPECT_EQ(3U, s3.size(), "");
    EXPECT_EQ(9, s3.back(), "");

    // converting span<int> -> span<const int>
    std::span<const int> cs = s;
    EXPECT_EQ(5U, cs.size(), "");

    // fixed extent from dynamic source
    std::span<int, 5> fixed(buf, 5);
    EXPECT_EQ(5U, fixed.size(), "");

    // iteration
    int sum = 0;
    for (int v : s2) {
        sum += v;
    }
    EXPECT_EQ(90, sum, "");

    // as_bytes / as_writable_bytes
    auto bytes = std::as_bytes(s3);
    EXPECT_EQ(3 * sizeof(int), bytes.size(), "");
    EXPECT_TRUE((std::is_same_v<decltype(bytes), std::span<const std::byte, 3 * sizeof(int)>>), "");

    auto wbytes = std::as_writable_bytes(s3);
    wbytes[0] = std::byte{0xff};
    EXPECT_TRUE(arr[0] != 7, "writing through byte view must hit the array");

    END_TEST;
}

BEGIN_TEST_CASE(libcpp_span_tests)
RUN_TEST(span_runtime)
END_TEST_CASE(libcpp_span_tests)

} // namespace
