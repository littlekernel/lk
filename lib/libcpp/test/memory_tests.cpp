//
// Copyright (c) 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

// Tests for lib/libcpp's <memory>.

#include <lib/unittest.h>
#include <memory>

namespace {

struct Counted {
    static inline int live = 0;
    int v;
    explicit Counted(int x = 0) : v(x) { live++; }
    virtual ~Counted() { live--; }
};

struct Derived : Counted {
    explicit Derived(int x) : Counted(x) {}
};

struct CountingDeleter {
    int *count;
    void operator()(Counted *p) const {
        (*count)++;
        delete p;
    }
};

// stateless deleters take no space thanks to the EBO
static_assert(sizeof(std::unique_ptr<int>) == sizeof(int *));
static_assert(sizeof(std::unique_ptr<int[]>) == sizeof(int *));
// stateful ones do
static_assert(sizeof(std::unique_ptr<Counted, CountingDeleter>) > sizeof(Counted *));

bool unique_ptr_basics() {
    BEGIN_TEST;

    EXPECT_EQ(0, Counted::live, "");
    {
        auto p = std::make_unique<Counted>(5);
        ASSERT_TRUE(static_cast<bool>(p), "");
        EXPECT_EQ(1, Counted::live, "");
        EXPECT_EQ(5, p->v, "");
        EXPECT_EQ(5, (*p).v, "");
        EXPECT_TRUE(p != nullptr, "");
        EXPECT_FALSE(p == nullptr, "");

        // reset replaces and destroys
        p.reset(new Counted(6));
        EXPECT_EQ(1, Counted::live, "");
        EXPECT_EQ(6, p->v, "");

        // release hands over ownership
        Counted *raw = p.release();
        EXPECT_FALSE(static_cast<bool>(p), "");
        EXPECT_EQ(1, Counted::live, "");
        delete raw;
        EXPECT_EQ(0, Counted::live, "");

        // assigning nullptr resets
        p.reset(new Counted(7));
        p = nullptr;
        EXPECT_EQ(0, Counted::live, "");
    }
    EXPECT_EQ(0, Counted::live, "");

    END_TEST;
}

bool unique_ptr_move_and_convert() {
    BEGIN_TEST;

    auto a = std::make_unique<Counted>(1);
    auto b = std::move(a);
    EXPECT_FALSE(static_cast<bool>(a), "moved-from must be empty");
    ASSERT_TRUE(static_cast<bool>(b), "");
    EXPECT_EQ(1, b->v, "");

    // move assignment destroys the old target value
    auto c = std::make_unique<Counted>(2);
    c = std::move(b);
    EXPECT_EQ(1, Counted::live, "");
    EXPECT_EQ(1, c->v, "");

    // converting move: unique_ptr<Derived> -> unique_ptr<Base>
    std::unique_ptr<Counted> base = std::make_unique<Derived>(9);
    EXPECT_EQ(9, base->v, "");
    EXPECT_EQ(2, Counted::live, "");

    // swap
    c.swap(base);
    EXPECT_EQ(9, c->v, "");
    EXPECT_EQ(1, base->v, "");
    swap(c, base); // ADL
    EXPECT_EQ(1, c->v, "");

    // comparisons
    EXPECT_TRUE(c == c, "");
    EXPECT_TRUE(c != base, "");

    c.reset();
    base.reset();
    EXPECT_EQ(0, Counted::live, "");

    END_TEST;
}

// a deleter that provides its own pointer type; unique_ptr must pick it up
struct BufDeleter {
    using pointer = char *;
    static inline int frees = 0;
    void operator()(char *p) const {
        frees++;
        delete[] p;
    }
};
static_assert(std::is_same_v<std::unique_ptr<int, BufDeleter>::pointer, char *>,
              "D::pointer must override element_type*");

bool unique_ptr_deleter_pointer_hook() {
    BEGIN_TEST;

    BufDeleter::frees = 0;
    {
        std::unique_ptr<int, BufDeleter> p(new char[16]);
        EXPECT_TRUE(static_cast<bool>(p), "");
        EXPECT_TRUE(p.get() != nullptr, "");
    }
    EXPECT_EQ(1, BufDeleter::frees, "");

    END_TEST;
}

bool unique_ptr_converting_assign() {
    BEGIN_TEST;

    std::unique_ptr<Counted> base;
    base = std::make_unique<Derived>(3);
    ASSERT_TRUE(static_cast<bool>(base), "");
    EXPECT_EQ(3, base->v, "");
    EXPECT_EQ(1, Counted::live, "");
    base.reset();
    EXPECT_EQ(0, Counted::live, "");

    END_TEST;
}

bool unique_ptr_custom_deleter() {
    BEGIN_TEST;

    int deletions = 0;
    {
        std::unique_ptr<Counted, CountingDeleter> p(new Counted(1),
                                                    CountingDeleter{&deletions});
        EXPECT_EQ(1, Counted::live, "");
    }
    EXPECT_EQ(1, deletions, "custom deleter must run exactly once");
    EXPECT_EQ(0, Counted::live, "");

    // deleter travels with moves
    int d2 = 0;
    std::unique_ptr<Counted, CountingDeleter> x(new Counted(2), CountingDeleter{&d2});
    auto y = std::move(x);
    y.reset();
    EXPECT_EQ(1, d2, "");

    END_TEST;
}

bool unique_ptr_array() {
    BEGIN_TEST;

    {
        auto arr = std::make_unique<Counted[]>(4);
        EXPECT_EQ(4, Counted::live, "");
        EXPECT_EQ(0, arr[0].v, "make_unique<T[]> value-initializes");
        arr[2].v = 22;
        EXPECT_EQ(22, arr[2].v, "");
    }
    EXPECT_EQ(0, Counted::live, "array form must delete[]");

    std::unique_ptr<int[]> ints(new int[8]{1, 2, 3});
    EXPECT_EQ(2, ints[1], "");
    ints.reset();
    EXPECT_FALSE(static_cast<bool>(ints), "");

    END_TEST;
}

bool addressof_and_destroy() {
    BEGIN_TEST;

    struct Overloaded {
        int x;
        Overloaded *operator&() = delete; // hostile operator&
    };
    Overloaded o{42};
    EXPECT_TRUE(std::addressof(o)->x == 42, "");

    alignas(Counted) unsigned char storage[sizeof(Counted) * 2];
    Counted *objs = reinterpret_cast<Counted *>(storage);
    new (&objs[0]) Counted(1);
    new (&objs[1]) Counted(2);
    EXPECT_EQ(2, Counted::live, "");
    std::destroy(objs, objs + 2);
    EXPECT_EQ(0, Counted::live, "");

    END_TEST;
}

BEGIN_TEST_CASE(libcpp_memory_tests)
RUN_TEST(unique_ptr_basics)
RUN_TEST(unique_ptr_move_and_convert)
RUN_TEST(unique_ptr_deleter_pointer_hook)
RUN_TEST(unique_ptr_converting_assign)
RUN_TEST(unique_ptr_custom_deleter)
RUN_TEST(unique_ptr_array)
RUN_TEST(addressof_and_destroy)
END_TEST_CASE(libcpp_memory_tests)

} // namespace
