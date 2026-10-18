/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

// Tests for lk::ref_counted and lk::ref_ptr in <lktl/ref_ptr.h>.

#include <lktl/ref_ptr.h>

#include <kernel/thread.h>
#include <lib/unittest.h>
#include <type_traits>
#include <utility>

namespace {

// counts destructor runs, so every test can check that the last release destroys exactly once
struct widget : lk::ref_counted<widget> {
    static int destroyed;
    int id;

    explicit widget(int i) : id(i) {}
    ~widget() { destroyed++; }
};
int widget::destroyed = 0;

// a hierarchy with no virtual functions: the base's destroy() picks the concrete type
struct object : lk::ref_counted<object> {
    enum class kind : uint8_t { SQUARE, CIRCLE };
    static int destroy_calls;

    explicit object(kind kd) : k(kd) {}
    static void destroy(object *o);

    kind k;
};
int object::destroy_calls = 0;

struct square : object {
    static int destroyed;
    int side = 3;

    square() : object(kind::SQUARE) {}
    ~square() { destroyed++; }
};
int square::destroyed = 0;

struct circle : object {
    static int destroyed;
    int radius = 5;

    circle() : object(kind::CIRCLE) {}
    ~circle() { destroyed++; }
};
int circle::destroyed = 0;

void object::destroy(object *o) {
    destroy_calls++;
    switch (o->k) {
        case kind::SQUARE:
            delete static_cast<square *>(o);
            break;
        case kind::CIRCLE:
            delete static_cast<circle *>(o);
            break;
    }
}

// the other way to make a hierarchy safe: a virtual destructor
struct shape : lk::ref_counted<shape> {
    virtual ~shape() = default;
};
struct triangle : shape {
    static int destroyed;
    ~triangle() override { destroyed++; }
};
int triangle::destroyed = 0;

// an object that is not on the heap: its destroy() records the call and frees nothing
struct fixed : lk::ref_counted<fixed> {
    static int destroy_calls;
    static void destroy(fixed *) { destroy_calls++; }
};
int fixed::destroy_calls = 0;

static_assert(sizeof(lk::ref_ptr<widget>) == sizeof(void *));
static_assert(sizeof(lk::ref_counted<widget>) == sizeof(int32_t));
static_assert(__is_standard_layout(lk::ref_counted<widget>));
static_assert(!std::is_copy_constructible_v<widget>);
static_assert(std::is_nothrow_move_constructible_v<lk::ref_ptr<widget>>);
static_assert(std::is_convertible_v<lk::ref_ptr<square>, lk::ref_ptr<object>>);
static_assert(!std::is_convertible_v<lk::ref_ptr<object>, lk::ref_ptr<square>>);
static_assert(!std::is_convertible_v<widget *, lk::ref_ptr<widget>>);
static_assert(lk::internal::has_destroy<object>::value);
static_assert(!lk::internal::has_destroy<widget>::value);
#if !DEBUG_ASSERT_IMPLEMENTED
static_assert(std::is_trivially_destructible_v<fixed>);
#endif

bool adopt_and_make() {
    BEGIN_TEST;

    widget::destroyed = 0;
    {
        lk::ref_ptr<widget> w = lk::make_ref<widget>(7);
        ASSERT_TRUE(w);
        EXPECT_EQ(7, w->id);
        EXPECT_EQ(7, (*w).id);
        EXPECT_EQ(1, w->ref_count_debug());
    }
    EXPECT_EQ(1, widget::destroyed);

    // adopt_ref by hand, and the unadopted count debug builds start from
    widget *raw = new widget(8);
#if DEBUG_ASSERT_IMPLEMENTED
    EXPECT_TRUE(raw->ref_count_debug() < 0);
#else
    EXPECT_EQ(1, raw->ref_count_debug());
#endif
    {
        lk::ref_ptr<widget> w = lk::adopt_ref(raw);
        EXPECT_EQ(raw, w.get());
        EXPECT_EQ(1, w->ref_count_debug());
    }
    EXPECT_EQ(2, widget::destroyed);

    // nullptr adopts to an empty ref_ptr, which is what a failed nothrow new produces
    lk::ref_ptr<widget> none = lk::adopt_ref<widget>(nullptr);
    EXPECT_FALSE(none);

    // an object that is never adopted can live on the stack and be destroyed normally
    {
        widget local(9);
        EXPECT_EQ(9, local.id);
    }
    EXPECT_EQ(3, widget::destroyed);

    END_TEST;
}

bool copy_and_move() {
    BEGIN_TEST;

    widget::destroyed = 0;
    {
        lk::ref_ptr<widget> a = lk::make_ref<widget>(1);
        widget *raw = a.get();

        lk::ref_ptr<widget> b = a;
        EXPECT_EQ(2, raw->ref_count_debug());
        EXPECT_TRUE(a == b);

        lk::ref_ptr<widget> c = std::move(a);
        EXPECT_FALSE(a);
        EXPECT_EQ(raw, c.get());
        EXPECT_EQ(2, raw->ref_count_debug());

        // a raw pointer to a live object takes a reference of its own
        lk::ref_ptr<widget> d(raw);
        EXPECT_EQ(3, raw->ref_count_debug());

        lk::ref_ptr<widget> e;
        e = d;
        EXPECT_EQ(4, raw->ref_count_debug());
        e = std::move(d);
        EXPECT_FALSE(d);
        EXPECT_EQ(3, raw->ref_count_debug());

        b.reset();
        c = nullptr;
        EXPECT_EQ(1, raw->ref_count_debug());
        EXPECT_EQ(0, widget::destroyed);
    }
    EXPECT_EQ(1, widget::destroyed);

    END_TEST;
}

bool assignment_releases_old() {
    BEGIN_TEST;

    widget::destroyed = 0;
    lk::ref_ptr<widget> a = lk::make_ref<widget>(1);
    lk::ref_ptr<widget> b = lk::make_ref<widget>(2);

    // assigning over the last reference destroys the old object, once
    a = b;
    EXPECT_EQ(1, widget::destroyed);
    EXPECT_EQ(2, a->id);
    EXPECT_EQ(2, a->ref_count_debug());

    // self-assignment by copy and by move keeps the object and the count
    lk::ref_ptr<widget> &alias = a;
    a = alias;
    EXPECT_EQ(2, a->ref_count_debug());
    a = std::move(alias);
    EXPECT_TRUE(a);
    EXPECT_EQ(2, a->ref_count_debug());

    a.swap(b);
    EXPECT_EQ(2, a->ref_count_debug());

    a.reset();
    b.reset();
    EXPECT_EQ(2, widget::destroyed);
    EXPECT_FALSE(a);
    EXPECT_TRUE(a == nullptr);
    EXPECT_TRUE(nullptr == b);

    END_TEST;
}

bool manual_counting() {
    BEGIN_TEST;

    widget::destroyed = 0;
    lk::ref_ptr<widget> w = lk::make_ref<widget>(3);
    widget *raw = w.get();

    raw->add_ref();
    EXPECT_EQ(2, raw->ref_count_debug());
    EXPECT_FALSE(raw->release());
    EXPECT_EQ(0, widget::destroyed);

    // hand the ref_ptr's reference to manual code, which drops it
    raw->add_ref();
    w.reset();
    EXPECT_TRUE(raw->release());
    EXPECT_EQ(1, widget::destroyed);

    END_TEST;
}

bool destroy_customization() {
    BEGIN_TEST;

    object::destroy_calls = 0;
    square::destroyed = 0;
    circle::destroyed = 0;
    {
        // derived converts to base by copy and by move
        lk::ref_ptr<square> s = lk::make_ref<square>();
        lk::ref_ptr<object> o1 = s;
        lk::ref_ptr<object> o2 = lk::make_ref<circle>();
        EXPECT_EQ(2, s->ref_count_debug());
        EXPECT_TRUE(o1 == s);
        EXPECT_TRUE(o1 != o2);

        lk::ref_ptr<object> o3;
        o3 = std::move(s);
        EXPECT_FALSE(s);
        EXPECT_EQ(2, o1->ref_count_debug());

        // back down after checking the kind, by copy and by move
        ASSERT_TRUE(o2->k == object::kind::CIRCLE);
        lk::ref_ptr<circle> c = lk::static_ref_cast<circle>(o2);
        EXPECT_EQ(5, c->radius);
        EXPECT_EQ(2, c->ref_count_debug());
        lk::ref_ptr<circle> c2 = lk::static_ref_cast<circle>(std::move(o2));
        EXPECT_FALSE(o2);
        EXPECT_EQ(2, c2->ref_count_debug());

        EXPECT_EQ(0, object::destroy_calls);
    }
    // each concrete destructor ran once, both through destroy()
    EXPECT_EQ(2, object::destroy_calls);
    EXPECT_EQ(1, square::destroyed);
    EXPECT_EQ(1, circle::destroyed);

    // a virtual destructor works without destroy()
    triangle::destroyed = 0;
    {
        lk::ref_ptr<shape> sh = lk::make_ref<triangle>();
        EXPECT_TRUE(sh);
    }
    EXPECT_EQ(1, triangle::destroyed);

    // destroy() decides not to free an object that is not on the heap
    fixed::destroy_calls = 0;
    {
        fixed local;
        {
            lk::ref_ptr<fixed> f = lk::adopt_ref(&local);
            lk::ref_ptr<fixed> f2 = f;
            EXPECT_EQ(2, local.ref_count_debug());
        }
        EXPECT_EQ(1, fixed::destroy_calls);
        EXPECT_EQ(0, local.ref_count_debug());
    }

    END_TEST;
}

constexpr int kThreads = 4;
constexpr int kRefsPerThread = 20000;

int ref_churn(void *arg) {
    const auto *shared = static_cast<const lk::ref_ptr<widget> *>(arg);
    for (int i = 0; i < kRefsPerThread; i++) {
        lk::ref_ptr<widget> mine = *shared;
        lk::ref_ptr<widget> other = mine;
        if (i % 1000 == 0) {
            thread_yield();
        }
    }
    return 0;
}

bool cross_thread() {
    BEGIN_TEST;

    widget::destroyed = 0;
    lk::ref_ptr<widget> shared = lk::make_ref<widget>(11);
    ASSERT_TRUE(shared);

    thread_t *threads[kThreads];
    for (auto &t : threads) {
        t = thread_create("ref_churn", &ref_churn, &shared, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
        ASSERT_NONNULL(t);
    }
    for (auto t : threads) {
        thread_resume(t);
    }
    for (auto t : threads) {
        thread_join(t, nullptr, INFINITE_TIME);
    }

    // no reference was lost or leaked, and only the last release destroys
    EXPECT_EQ(1, shared->ref_count_debug());
    EXPECT_EQ(0, widget::destroyed);
    shared.reset();
    EXPECT_EQ(1, widget::destroyed);

    END_TEST;
}

BEGIN_TEST_CASE(ref_ptr_tests)
RUN_TEST(adopt_and_make)
RUN_TEST(copy_and_move)
RUN_TEST(assignment_releases_old)
RUN_TEST(manual_counting)
RUN_TEST(destroy_customization)
RUN_TEST(cross_thread)
END_TEST_CASE(ref_ptr_tests)

} // namespace
