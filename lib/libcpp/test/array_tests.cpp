//
// Copyright (c) 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

// Tests for lib/libcpp's <array>.

#include <array>
#include <lib/unittest.h>

namespace {

// aggregate init and constexpr access
constexpr std::array<int, 3> kA{{1, 2, 3}};
static_assert(kA.size() == 3);
static_assert(!kA.empty());
static_assert(kA[0] == 1 && kA[2] == 3);
static_assert(kA.front() == 1 && kA.back() == 3);
static_assert(kA.at(1) == 2);
static_assert(std::get<1>(kA) == 2);
static_assert(*kA.begin() == 1);
static_assert(kA.end() - kA.begin() == 3);

// brace-elision aggregate init also works
constexpr std::array<int, 2> kB{4, 5};
static_assert(kB[1] == 5);

// CTAD
constexpr std::array kC{1, 2, 3, 4};
static_assert(std::is_same_v<decltype(kC), const std::array<int, 4>>);
static_assert(kC.size() == 4);

// comparisons
static_assert(std::array<int, 2>{{1, 2}} == std::array<int, 2>{{1, 2}});
static_assert(std::array<int, 2>{{1, 2}} != std::array<int, 2>{{1, 3}});
static_assert(std::array<int, 2>{{1, 2}} < std::array<int, 2>{{1, 3}});
static_assert(std::array<int, 2>{{2, 0}} > std::array<int, 2>{{1, 9}});

// tuple protocol
static_assert(std::tuple_size<std::array<int, 3>>::value == 3);
static_assert(std::is_same_v<std::tuple_element_t<2, std::array<char, 3>>, char>);

// zero-length arrays
constexpr std::array<int, 0> kZ{};
static_assert(kZ.empty());
static_assert(kZ.size() == 0);
static_assert(kZ.max_size() == 0);
static_assert(kZ.begin() == kZ.end());
static_assert(kZ.cbegin() == kZ.cend());
static_assert(kZ.data() == nullptr);

// remaining observers on the general form
static_assert(kA.max_size() == 3);
static_assert(kA.cend() - kA.cbegin() == 3);
static_assert(kA.data() != nullptr);
static_assert(std::array<int, 2>{{1, 2}} <= std::array<int, 2>{{1, 2}});
static_assert(std::array<int, 2>{{1, 3}} >= std::array<int, 2>{{1, 2}});

bool array_runtime() {
    BEGIN_TEST;

    std::array<int, 4> a{{9, 8, 7, 6}};
    EXPECT_EQ(4U, a.size(), "");
    EXPECT_EQ(9, a[0], "");
    EXPECT_EQ(6, a.back(), "");
    EXPECT_TRUE(a.data() == &a[0], "");

    // range-for over the array
    int sum = 0;
    for (int v : a) {
        sum += v;
    }
    EXPECT_EQ(30, sum, "");

    // mutation through iterators, fill, swap
    *a.begin() = 1;
    EXPECT_EQ(1, a[0], "");

    a.fill(5);
    EXPECT_EQ(5, a[3], "");

    std::array<int, 2> x{{1, 2}}, y{{3, 4}};
    x.swap(y);
    EXPECT_EQ(3, x[0], "");
    swap(x, y); // ADL free swap
    EXPECT_EQ(1, x[0], "");

    // structured bindings
    std::array<int, 2> sb{{10, 20}};
    auto &[first, second] = sb;
    first = 11;
    EXPECT_EQ(11, sb[0], "");
    EXPECT_EQ(20, second, "");

    // get on rvalue
    int from_rvalue = std::get<1>(std::array<int, 2>{{10, 20}});
    EXPECT_EQ(20, from_rvalue, "");

    END_TEST;
}

BEGIN_TEST_CASE(libcpp_array_tests)
RUN_TEST(array_runtime)
END_TEST_CASE(libcpp_array_tests)

} // namespace
