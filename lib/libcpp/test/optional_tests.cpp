//
// Copyright (c) 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

// Tests for lib/libcpp's <optional>.

#include <lib/unittest.h>
#include <optional>

namespace {

// counts constructions/destructions so leaks in the storage management show up
struct Counted {
    static inline int live = 0;
    int v;
    explicit Counted(int x) : v(x) { live++; }
    Counted(const Counted &o) : v(o.v) { live++; }
    Counted(Counted &&o) : v(o.v) {
        o.v = -1;
        live++;
    }
    Counted &operator=(const Counted &) = default;
    Counted &operator=(Counted &&o) {
        v = o.v;
        o.v = -1;
        return *this;
    }
    ~Counted() { live--; }
};

// compile-time surface
constexpr std::optional<int> kEmpty;
static_assert(!kEmpty.has_value());
static_assert(!kEmpty);
static_assert(kEmpty == std::nullopt);
static_assert(std::nullopt == kEmpty);
static_assert(kEmpty.value_or(42) == 42);

constexpr std::optional<int> kFive = 5;
static_assert(kFive.has_value());
static_assert(*kFive == 5);
static_assert(kFive.value() == 5);
static_assert(kFive.value_or(42) == 5);
static_assert(kFive != std::nullopt);

constexpr std::optional<int> kInPlace(std::in_place, 7);
static_assert(*kInPlace == 7);

// trivial destructibility is preserved for trivial T
static_assert(std::is_trivially_destructible_v<std::optional<int>>);
static_assert(!std::is_trivially_destructible_v<std::optional<Counted>>);

// comparisons
static_assert(kFive == 5 && 5 == kFive);
static_assert(kFive != 6 && 6 != kFive);
static_assert(kFive < 6 && 4 < kFive);
static_assert(kEmpty < kFive);
static_assert(kEmpty < 0, "empty compares less than any value");
static_assert(kFive > kEmpty);
static_assert(std::optional<int>(5) == kFive);
static_assert(std::optional<int>(4) < kFive);
static_assert(kEmpty == std::optional<int>());

// CTAD + make_optional, both forms
static_assert(std::is_same_v<decltype(std::make_optional(3)), std::optional<int>>);
static_assert(*std::make_optional(3) == 3);
static_assert(*std::make_optional<int>(4) == 4);
static_assert(std::make_optional<std::pair<int, int>>(1, 2)->second == 2,
              "variadic make_optional forwards to in_place construction");

// nullopt ordering operators
static_assert(!(kEmpty < std::nullopt));
static_assert(kEmpty <= std::nullopt);
static_assert(kEmpty >= std::nullopt);
static_assert(std::nullopt < kFive);
static_assert(kFive > std::nullopt);
static_assert(std::nullopt <= kFive);
static_assert(!(std::nullopt >= kFive));

// const access forms
static_assert(*kFive == 5);
static_assert(kFive.operator->() != nullptr);

bool optional_basics() {
    BEGIN_TEST;

    std::optional<int> o;
    EXPECT_FALSE(o.has_value(), "");
    o = 3;
    ASSERT_TRUE(o.has_value(), "");
    EXPECT_EQ(3, *o, "");
    o.reset();
    EXPECT_FALSE(o.has_value(), "");
    o.emplace(9);
    EXPECT_EQ(9, o.value(), "");

    END_TEST;
}

bool optional_object_lifetime() {
    BEGIN_TEST;

    EXPECT_EQ(0, Counted::live, "");
    {
        std::optional<Counted> a(std::in_place, 1);
        EXPECT_EQ(1, Counted::live, "");
        EXPECT_EQ(1, a->v, "");

        // copy construction
        std::optional<Counted> b = a;
        EXPECT_EQ(2, Counted::live, "");
        EXPECT_EQ(1, b->v, "");

        // move construction leaves the source engaged but moved-from
        std::optional<Counted> c = std::move(a);
        EXPECT_EQ(3, Counted::live, "");
        EXPECT_EQ(1, c->v, "");
        EXPECT_TRUE(a.has_value(), "");
        EXPECT_EQ(-1, a->v, "");

        // reset destroys
        c.reset();
        EXPECT_EQ(2, Counted::live, "");

        // assignment into empty constructs, into engaged assigns
        c = b;
        EXPECT_EQ(3, Counted::live, "");
        c = std::optional<Counted>(std::in_place, 5);
        EXPECT_EQ(3, Counted::live, "");
        EXPECT_EQ(5, c->v, "");

        // assigning nullopt destroys
        b = std::nullopt;
        EXPECT_EQ(2, Counted::live, "");
    }
    EXPECT_EQ(0, Counted::live, "destructors must run when optionals go out of scope");

    END_TEST;
}

bool optional_emplace_and_swap() {
    BEGIN_TEST;

    std::optional<Counted> x(std::in_place, 1);
    std::optional<Counted> y;

    // engaged <-> empty swap moves the value across
    x.swap(y);
    EXPECT_FALSE(x.has_value(), "");
    ASSERT_TRUE(y.has_value(), "");
    EXPECT_EQ(1, y->v, "");

    // engaged <-> engaged swap
    x.emplace(2);
    swap(x, y); // ADL
    EXPECT_EQ(1, x->v, "");
    EXPECT_EQ(2, y->v, "");

    x.reset();
    y.reset();
    EXPECT_EQ(0, Counted::live, "");

    // emplace over an engaged optional destroys the old value first
    std::optional<Counted> z(std::in_place, 3);
    z.emplace(4);
    EXPECT_EQ(1, Counted::live, "");
    EXPECT_EQ(4, z->v, "");
    z.reset();
    EXPECT_EQ(0, Counted::live, "");

    END_TEST;
}

bool optional_value_or_move() {
    BEGIN_TEST;

    std::optional<Counted> m(std::in_place, 6);
    Counted out = std::move(m).value_or(Counted{0});
    EXPECT_EQ(6, out.v, "");
    EXPECT_EQ(-1, m->v, "value_or on rvalue optional must move out");
    m.reset();

    EXPECT_EQ(1, Counted::live, "");

    // value() on an rvalue optional moves as well
    std::optional<Counted> r(std::in_place, 8);
    Counted moved = std::move(r).value();
    EXPECT_EQ(8, moved.v, "");
    EXPECT_EQ(-1, r->v, "");
    r.reset();

    END_TEST;
}

BEGIN_TEST_CASE(libcpp_optional_tests)
RUN_TEST(optional_basics)
RUN_TEST(optional_object_lifetime)
RUN_TEST(optional_emplace_and_swap)
RUN_TEST(optional_value_or_move)
END_TEST_CASE(libcpp_optional_tests)

} // namespace
