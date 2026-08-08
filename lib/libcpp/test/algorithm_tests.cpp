//
// Copyright (c) 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

// Tests for lib/libcpp's <algorithm>.

#include <algorithm>
#include <array>
#include <lib/unittest.h>
#include <rand.h>

namespace {

// min/max/minmax/clamp at compile time
static_assert(std::min(3, 5) == 3);
static_assert(std::max(3, 5) == 5);
static_assert(std::min({4, 2, 9, 7}) == 2);
static_assert(std::max({4, 2, 9, 7}) == 9);
static_assert(std::minmax(5, 3).first == 3 && std::minmax(5, 3).second == 5);
static_assert(std::clamp(10, 0, 5) == 5);
static_assert(std::clamp(-1, 0, 5) == 0);
static_assert(std::clamp(3, 0, 5) == 3);
static_assert(std::min(3, 5, [](int a, int b) { return a > b; }) == 5, "comparator form");

// comparator forms of the rest of the family
struct Desc {
    constexpr bool operator()(int a, int b) const { return a > b; }
};
static_assert(std::max(3, 5, Desc{}) == 3);
static_assert(std::min({4, 2, 9}, Desc{}) == 9);
static_assert(std::max({4, 2, 9}, Desc{}) == 2);
static_assert(std::minmax(3, 5, Desc{}).first == 5);
static_assert(std::clamp(1, 5, 3, Desc{}) == 3, "descending-order clamp");

constexpr int kSorted[] = {1, 3, 5, 7, 9};
constexpr int kOther[] = {1, 3, 5, 7, 10};

// queries at compile time
static_assert(std::all_of(kSorted, kSorted + 5, [](int v) { return v > 0; }));
static_assert(std::any_of(kSorted, kSorted + 5, [](int v) { return v == 7; }));
static_assert(std::none_of(kSorted, kSorted + 5, [](int v) { return v < 0; }));
static_assert(std::find(kSorted, kSorted + 5, 5) == kSorted + 2);
static_assert(std::find(kSorted, kSorted + 5, 6) == kSorted + 5);
static_assert(std::find_if(kSorted, kSorted + 5, [](int v) { return v > 4; }) == kSorted + 2);
static_assert(std::find_if_not(kSorted, kSorted + 5, [](int v) { return v < 4; }) == kSorted + 2);
static_assert(std::count(kSorted, kSorted + 5, 3) == 1);
static_assert(std::count_if(kSorted, kSorted + 5, [](int v) { return v > 4; }) == 3);
static_assert(std::equal(kSorted, kSorted + 4, kOther));
static_assert(!std::equal(kSorted, kSorted + 5, kOther, kOther + 5));
static_assert(std::mismatch(kSorted, kSorted + 5, kOther).first == kSorted + 4);
static_assert(std::lexicographical_compare(kSorted, kSorted + 5, kOther, kOther + 5));
static_assert(!std::lexicographical_compare(kOther, kOther + 5, kSorted, kSorted + 5));
static_assert(std::min_element(kOther, kOther + 5) == kOther);
static_assert(std::max_element(kOther, kOther + 5) == kOther + 4);
static_assert(std::is_sorted(kSorted, kSorted + 5));

// binary search family at compile time
static_assert(std::lower_bound(kSorted, kSorted + 5, 5) == kSorted + 2);
static_assert(std::lower_bound(kSorted, kSorted + 5, 6) == kSorted + 3);
static_assert(std::upper_bound(kSorted, kSorted + 5, 5) == kSorted + 3);
static_assert(std::upper_bound(kSorted, kSorted + 5, 0) == kSorted);
static_assert(std::binary_search(kSorted, kSorted + 5, 7));
static_assert(!std::binary_search(kSorted, kSorted + 5, 6));

// comparator forms over a descending-sorted range
constexpr int kDesc[] = {9, 7, 5, 3, 1};
static_assert(std::lower_bound(kDesc, kDesc + 5, 5, Desc{}) == kDesc + 2);
static_assert(std::upper_bound(kDesc, kDesc + 5, 5, Desc{}) == kDesc + 3);
static_assert(std::binary_search(kDesc, kDesc + 5, 3, Desc{}));
static_assert(std::min_element(kDesc, kDesc + 5, Desc{}) == kDesc, "min by > is the max");
static_assert(std::max_element(kDesc, kDesc + 5, Desc{}) == kDesc + 4);
static_assert(std::is_sorted(kDesc, kDesc + 5, Desc{}));
static_assert(std::lexicographical_compare(kDesc, kDesc + 5, kSorted, kSorted + 5, Desc{}));
static_assert(std::equal(kSorted, kSorted + 5, kSorted,
                         [](int a, int b) { return a == b; }));
static_assert(std::mismatch(kSorted, kSorted + 5, kOther,
                            [](int a, int b) { return a == b; })
                  .first == kSorted + 4);

bool copy_fill_transform() {
    BEGIN_TEST;

    int src[5] = {1, 2, 3, 4, 5};
    int dst[5] = {};

    int *end = std::copy(src, src + 5, dst);
    EXPECT_TRUE(end == dst + 5, "");
    EXPECT_TRUE(std::equal(src, src + 5, dst), "");

    std::fill(dst, dst + 5, 9);
    EXPECT_EQ(9, dst[0], "");
    EXPECT_EQ(9, dst[4], "");

    std::fill_n(dst, 2, 1);
    EXPECT_EQ(1, dst[1], "");
    EXPECT_EQ(9, dst[2], "");

    std::copy_n(src, 3, dst);
    EXPECT_EQ(3, dst[2], "");

    // overlapping copy toward the back must use copy_backward
    int buf[6] = {1, 2, 3, 4, 5, 6};
    std::copy_backward(buf, buf + 5, buf + 6);
    EXPECT_EQ(1, buf[0], "");
    EXPECT_EQ(1, buf[1], "");
    EXPECT_EQ(5, buf[5], "");

    std::transform(src, src + 5, dst, [](int v) { return v * 10; });
    EXPECT_EQ(50, dst[4], "");

    int sums[5];
    std::transform(src, src + 5, dst, sums, [](int a, int b) { return a + b; });
    EXPECT_EQ(55, sums[4], "");

    std::array<int, 3> mv_src{{1, 2, 3}};
    std::array<int, 3> mv_dst{};
    std::move(mv_src.begin(), mv_src.end(), mv_dst.begin());
    EXPECT_EQ(3, mv_dst[2], "");

    END_TEST;
}

bool reverse_remove_swap() {
    BEGIN_TEST;

    int a[5] = {1, 2, 3, 4, 5};
    std::reverse(a, a + 5);
    EXPECT_EQ(5, a[0], "");
    EXPECT_EQ(1, a[4], "");

    int b[4] = {1, 2, 3, 4};
    std::reverse(b, b + 4);
    EXPECT_EQ(4, b[0], "");
    EXPECT_EQ(1, b[3], "");

    int r[6] = {1, 2, 1, 3, 1, 4};
    int *new_end = std::remove(r, r + 6, 1);
    EXPECT_EQ(3, new_end - r, "");
    EXPECT_EQ(2, r[0], "");
    EXPECT_EQ(3, r[1], "");
    EXPECT_EQ(4, r[2], "");

    int ri[5] = {1, 2, 3, 4, 5};
    new_end = std::remove_if(ri, ri + 5, [](int v) { return v % 2 == 0; });
    EXPECT_EQ(3, new_end - ri, "");
    EXPECT_EQ(5, ri[2], "");

    int x[3] = {1, 2, 3};
    int y[3] = {4, 5, 6};
    std::swap_ranges(x, x + 3, y);
    EXPECT_EQ(4, x[0], "");
    EXPECT_EQ(3, y[2], "");

    END_TEST;
}

bool sort_basics() {
    BEGIN_TEST;

    int a[8] = {5, 2, 8, 1, 9, 3, 7, 2};
    std::sort(a, a + 8);
    EXPECT_TRUE(std::is_sorted(a, a + 8), "");
    EXPECT_EQ(1, a[0], "");
    EXPECT_EQ(9, a[7], "");
    EXPECT_EQ(2, std::count(a, a + 8, 2), "duplicates preserved");

    // descending comparator
    std::sort(a, a + 8, [](int l, int r) { return l > r; });
    EXPECT_EQ(9, a[0], "");
    EXPECT_EQ(1, a[7], "");

    // degenerate sizes
    int single[1] = {42};
    std::sort(single, single + 1);
    EXPECT_EQ(42, single[0], "");
    std::sort(single, single); // empty range

    // already sorted / reverse sorted
    int inc[5] = {1, 2, 3, 4, 5};
    std::sort(inc, inc + 5);
    EXPECT_TRUE(std::is_sorted(inc, inc + 5), "");
    int dec[5] = {5, 4, 3, 2, 1};
    std::sort(dec, dec + 5);
    EXPECT_TRUE(std::is_sorted(dec, dec + 5), "");

    END_TEST;
}

bool sort_larger_random() {
    BEGIN_TEST;

    // a few hundred pseudo-random elements, checked for sortedness and
    // content preservation via a sum
    constexpr int kN = 384;
    static int buf[kN];
    int64_t sum_before = 0;
    for (int i = 0; i < kN; i++) {
        buf[i] = static_cast<int>(rand() & 0xffff);
        sum_before += buf[i];
    }

    std::sort(buf, buf + kN);

    int64_t sum_after = 0;
    for (int i = 0; i < kN; i++) {
        sum_after += buf[i];
    }
    EXPECT_TRUE(std::is_sorted(buf, buf + kN), "");
    EXPECT_TRUE(sum_before == sum_after, "sort must permute, not alter");

    // binary search over the sorted result agrees with linear search
    for (int probe = 0; probe < 8; probe++) {
        const int v = buf[(probe * 53) % kN];
        EXPECT_TRUE(std::binary_search(buf, buf + kN, v), "");
        int *lb = std::lower_bound(buf, buf + kN, v);
        EXPECT_TRUE(lb == std::find(buf, buf + kN, v), "");
    }

    END_TEST;
}

bool for_each_and_minmax_elements() {
    BEGIN_TEST;

    int a[4] = {3, 1, 4, 1};
    int sum = 0;
    std::for_each(a, a + 4, [&sum](int v) { sum += v; });
    EXPECT_EQ(9, sum, "");

    EXPECT_TRUE(std::min_element(a, a + 4) == a + 1, "first of equal minima");
    EXPECT_TRUE(std::max_element(a, a + 4) == a + 2, "");

    END_TEST;
}

BEGIN_TEST_CASE(libcpp_algorithm_tests)
RUN_TEST(copy_fill_transform)
RUN_TEST(reverse_remove_swap)
RUN_TEST(sort_basics)
RUN_TEST(sort_larger_random)
RUN_TEST(for_each_and_minmax_elements)
END_TEST_CASE(libcpp_algorithm_tests)

} // namespace
