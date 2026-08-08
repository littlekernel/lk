//
// Copyright (c) 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

// Tests for lib/libcpp's <utility>.

#include <lib/unittest.h>
#include <utility>

namespace {

struct Movable {
    int v = 0;
    Movable() = default;
    explicit Movable(int x) : v(x) {}
    Movable(Movable &&o) : v(o.v) { o.v = -1; }
    Movable &operator=(Movable &&o) {
        v = o.v;
        o.v = -1;
        return *this;
    }
};

// compile-time surface checks
constexpr std::pair<int, long> kPair{1, 2L};
static_assert(kPair.first == 1 && kPair.second == 2L);
static_assert(std::get<0>(kPair) == 1 && std::get<1>(kPair) == 2L);
static_assert(std::get<int>(kPair) == 1 && std::get<long>(kPair) == 2L);
static_assert(std::tuple_size<std::pair<int, long>>::value == 2);
static_assert(std::is_same_v<std::tuple_element_t<1, std::pair<int, long>>, long>);
static_assert(std::is_same_v<std::tuple_element_t<0, const std::pair<int, long>>, const int>);

constexpr auto kMade = std::make_pair(3, 4.0f);
static_assert(std::is_same_v<decltype(kMade), const std::pair<int, float>>);

constexpr std::pair kCtad{5, 'a'};
static_assert(std::is_same_v<decltype(kCtad), const std::pair<int, char>>);

static_assert(std::pair<int, int>{1, 2} == std::pair<int, int>{1, 2});
static_assert(std::pair<int, int>{1, 2} != std::pair<int, int>{1, 3});
static_assert(std::pair<int, int>{1, 2} < std::pair<int, int>{1, 3});
static_assert(std::pair<int, int>{1, 2} < std::pair<int, int>{2, 0});
static_assert(std::pair<int, int>{2, 0} >= std::pair<int, int>{1, 9});

static_assert(std::make_index_sequence<4>::size() == 4);
static_assert(std::is_same_v<std::make_index_sequence<2>, std::index_sequence<0, 1>>);
static_assert(std::index_sequence_for<int, char, long>::size() == 3);

static_assert(std::is_same_v<decltype(std::declval<int &>()), int &>);

// the in_place tag objects and their types
static_assert(std::is_same_v<decltype(std::in_place), const std::in_place_t &> ||
              std::is_same_v<std::remove_cvref_t<decltype(std::in_place)>, std::in_place_t>);
static_assert(std::is_same_v<std::remove_cvref_t<decltype(std::in_place_type<int>)>,
                             std::in_place_type_t<int>>);
static_assert(std::is_same_v<std::remove_cvref_t<decltype(std::in_place_index<3>)>,
                             std::in_place_index_t<3>>);

// get on rvalue/const-rvalue pairs, and get<T> on const
constexpr const std::pair<int, long> kConstPair{8, 9L};
static_assert(std::get<int>(kConstPair) == 8);
static_assert(std::get<0>(std::pair<int, long>{4, 5L}) == 4);
static_assert(std::get<1>(static_cast<const std::pair<int, long> &&>(kConstPair)) == 9L);

template <std::size_t... I>
constexpr int sum_seq(std::integer_sequence<std::size_t, I...>) {
    return (int(I) + ... + 0);
}
static_assert(sum_seq(std::make_index_sequence<5>{}) == 10);

bool move_and_forward() {
    BEGIN_TEST;

    Movable a(1);
    Movable b(std::move(a));
    EXPECT_EQ(1, b.v, "");
    EXPECT_EQ(-1, a.v, "moved-from source should be marked");

    END_TEST;
}

bool swap_and_exchange() {
    BEGIN_TEST;

    Movable a(1), b(2);
    std::swap(a, b);
    EXPECT_EQ(2, a.v, "");
    EXPECT_EQ(1, b.v, "");

    int old = std::exchange(a.v, 7);
    EXPECT_EQ(2, old, "");
    EXPECT_EQ(7, a.v, "");

    int arr1[3] = {1, 2, 3};
    int arr2[3] = {4, 5, 6};
    std::swap(arr1, arr2);
    EXPECT_EQ(4, arr1[0], "");
    EXPECT_EQ(3, arr2[2], "");

    END_TEST;
}

bool pair_runtime() {
    BEGIN_TEST;

    // structured bindings via the tuple protocol
    std::pair<int, char> pr{42, 'x'};
    auto &[n, c] = pr;
    n = 43;
    EXPECT_EQ(43, pr.first, "structured binding must alias the pair");
    EXPECT_EQ('x', c, "");

    // converting construction and assignment
    std::pair<long, float> conv = std::pair<int, float>{1, 2.0f};
    EXPECT_EQ(1L, conv.first, "");
    conv = std::pair<int, float>{3, 4.0f};
    EXPECT_EQ(3L, conv.first, "");

    // member and free swap
    std::pair<int, int> x{1, 2}, y{3, 4};
    x.swap(y);
    EXPECT_EQ(3, x.first, "");
    swap(x, y); // ADL
    EXPECT_EQ(1, x.first, "");

    // pair holding a move-only member
    std::pair<Movable, int> mp{Movable{5}, 6};
    auto mp2 = std::move(mp);
    EXPECT_EQ(5, mp2.first.v, "");
    EXPECT_EQ(-1, mp.first.v, "");

    END_TEST;
}

bool as_const_works() {
    BEGIN_TEST;

    int x = 1;
    EXPECT_TRUE((std::is_same_v<decltype(std::as_const(x)), const int &>), "");
    EXPECT_EQ(1, std::as_const(x), "");

    END_TEST;
}

BEGIN_TEST_CASE(libcpp_utility_tests)
RUN_TEST(move_and_forward)
RUN_TEST(swap_and_exchange)
RUN_TEST(pair_runtime)
RUN_TEST(as_const_works)
END_TEST_CASE(libcpp_utility_tests)

} // namespace
