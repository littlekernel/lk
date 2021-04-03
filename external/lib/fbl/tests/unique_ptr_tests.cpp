// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>

#include <fbl/alloc_checker.h>
#include <fbl/type_support.h>
#include <fbl/unique_ptr.h>
#include <unittest/unittest.h>

static int destroy_count = 0;

struct DeleteCounter {
    DeleteCounter() = default;
    DeleteCounter(int value) : value(value) {}

    static void operator delete(void* ptr) {
        destroy_count++;
        ::operator delete(ptr);
    }

    static void operator delete[](void* ptr) {
        destroy_count++;
        ::operator delete[](ptr);
    }

    int value = 0;
};

using CountingPtr    = fbl::unique_ptr<DeleteCounter>;
using CountingArrPtr = fbl::unique_ptr<DeleteCounter[]>;

static_assert(fbl::is_standard_layout<int>::value,
              "fbl::unique_ptr<T>'s should have a standard layout");
static_assert(fbl::is_standard_layout<CountingPtr>::value,
              "fbl::unique_ptr<T>'s should have a standard layout");
static_assert(fbl::is_standard_layout<int[]>::value,
              "fbl::unique_ptr<T[]>'s should have a standard layout");
static_assert(fbl::is_standard_layout<CountingArrPtr>::value,
              "fbl::unique_ptr<T[]>'s should have a standard layout");

static bool uptr_test_scoped_destruction() {
    BEGIN_TEST;
    destroy_count = 0;

    fbl::AllocChecker ac;
    // Construct and let a unique_ptr fall out of scope.
    {
        CountingPtr ptr(new (&ac) DeleteCounter);
        EXPECT_TRUE(ac.check());
    }

    EXPECT_EQ(1, destroy_count);
    END_TEST;
}

static bool uptr_test_move() {
    BEGIN_TEST;
    destroy_count = 0;

    fbl::AllocChecker ac;
    // Construct and move into another unique_ptr.
    {
        CountingPtr ptr(new (&ac) DeleteCounter);
        EXPECT_TRUE(ac.check());

        CountingPtr ptr2 = fbl::move(ptr);
        EXPECT_NULL(ptr, "expected ptr to be null");
    }

    EXPECT_EQ(1, destroy_count);

    END_TEST;
}

static bool uptr_test_null_scoped_destruction() {
    BEGIN_TEST;
    destroy_count = 0;

    // Construct a null unique_ptr and let it fall out of scope - should not call
    // deleter.
    {
        CountingPtr ptr(nullptr);
    }

    EXPECT_EQ(0, destroy_count);

    END_TEST;
}

static bool uptr_test_diff_scope_swap() {
    BEGIN_TEST;
    destroy_count = 0;

    // Construct a pair of unique_ptrs in different scopes, swap them, and verify
    // that the values change places and that the values are destroyed at the
    // correct times.

    fbl::AllocChecker ac;
    {
        CountingPtr ptr1(new (&ac) DeleteCounter(4));
        EXPECT_TRUE(ac.check());
        {
            CountingPtr ptr2(new (&ac) DeleteCounter(7));
            EXPECT_TRUE(ac.check());

            ptr1.swap(ptr2);
            EXPECT_EQ(7, ptr1->value);
            EXPECT_EQ(4, ptr2->value);
        }
        EXPECT_EQ(1, destroy_count);
    }
    EXPECT_EQ(2, destroy_count);

    END_TEST;
}

static bool uptr_test_bool_op() {
    BEGIN_TEST;
    destroy_count = 0;

    fbl::AllocChecker ac;

    CountingPtr foo(new (&ac) DeleteCounter);
    EXPECT_TRUE(ac.check());
    EXPECT_TRUE(static_cast<bool>(foo));

    foo.reset();
    EXPECT_EQ(1, destroy_count);
    EXPECT_FALSE(static_cast<bool>(foo));

    END_TEST;
}

static bool uptr_test_comparison() {
    BEGIN_TEST;

    fbl::AllocChecker ac;
    // Test comparison operators.
    fbl::unique_ptr<DeleteCounter> null_unique;
    fbl::unique_ptr<DeleteCounter> lesser_unique(new (&ac) DeleteCounter(1));
    EXPECT_TRUE(ac.check());

    fbl::unique_ptr<DeleteCounter> greater_unique(new (&ac) DeleteCounter(2));
    EXPECT_TRUE(ac.check());

    EXPECT_NE(lesser_unique.get(), greater_unique.get());
    if (lesser_unique.get() > greater_unique.get())
        lesser_unique.swap(greater_unique);

    // Comparison against nullptr
    EXPECT_TRUE(   null_unique == nullptr);
    EXPECT_TRUE( lesser_unique != nullptr);
    EXPECT_TRUE(greater_unique != nullptr);

    EXPECT_TRUE(nullptr ==    null_unique);
    EXPECT_TRUE(nullptr !=  lesser_unique);
    EXPECT_TRUE(nullptr != greater_unique);

    // Comparison against other unique_ptr<>s
    EXPECT_TRUE( lesser_unique  ==  lesser_unique);
    EXPECT_FALSE( lesser_unique == greater_unique);
    EXPECT_FALSE(greater_unique ==  lesser_unique);
    EXPECT_TRUE(greater_unique  == greater_unique);

    EXPECT_FALSE( lesser_unique !=  lesser_unique);
    EXPECT_TRUE ( lesser_unique != greater_unique, "");
    EXPECT_TRUE (greater_unique !=  lesser_unique, "");
    EXPECT_FALSE(greater_unique != greater_unique);

    EXPECT_FALSE( lesser_unique <   lesser_unique);
    EXPECT_TRUE ( lesser_unique <  greater_unique, "");
    EXPECT_FALSE(greater_unique <   lesser_unique);
    EXPECT_FALSE(greater_unique <  greater_unique);

    EXPECT_FALSE( lesser_unique >   lesser_unique);
    EXPECT_FALSE( lesser_unique >  greater_unique);
    EXPECT_TRUE (greater_unique >   lesser_unique, "");
    EXPECT_FALSE(greater_unique >  greater_unique);

    EXPECT_TRUE ( lesser_unique <=  lesser_unique, "");
    EXPECT_TRUE ( lesser_unique <= greater_unique, "");
    EXPECT_FALSE(greater_unique <=  lesser_unique);
    EXPECT_TRUE (greater_unique <= greater_unique, "");

    EXPECT_TRUE ( lesser_unique >=  lesser_unique, "");
    EXPECT_FALSE( lesser_unique >= greater_unique);
    EXPECT_TRUE (greater_unique >=  lesser_unique, "");
    EXPECT_TRUE (greater_unique >= greater_unique, "");

    END_TEST;
}

static bool uptr_test_array_scoped_destruction() {
    BEGIN_TEST;
    destroy_count = 0;

    fbl::AllocChecker ac;
    // Construct and let a unique_ptr fall out of scope.
    {
        CountingArrPtr ptr(new (&ac) DeleteCounter[1]);
        EXPECT_TRUE(ac.check());
    }
    EXPECT_EQ(1, destroy_count);

    END_TEST;
}

static bool uptr_test_array_move() {
    BEGIN_TEST;
    destroy_count = 0;

    fbl::AllocChecker ac;
    // Construct and move into another unique_ptr.
    {
        CountingArrPtr ptr(new (&ac) DeleteCounter[1]);
        EXPECT_TRUE(ac.check());

        CountingArrPtr ptr2 = fbl::move(ptr);
        EXPECT_NULL(ptr, "expected ptr to be null");
    }
    EXPECT_EQ(1, destroy_count);

    END_TEST;
}

static bool uptr_test_array_null_scoped_destruction() {
    BEGIN_TEST;
    destroy_count = 0;

    // Construct a null unique_ptr and let it fall out of scope - should not call
    // deleter.
    {
        CountingArrPtr ptr(nullptr);
    }
    EXPECT_EQ(0, destroy_count);

    END_TEST;
}

static bool uptr_test_array_diff_scope_swap() {
    BEGIN_TEST;
    destroy_count = 0;

    // Construct a pair of unique_ptrs in different scopes, swap them, and verify
    // that the values change places and that the values are destroyed at the
    // correct times.
    fbl::AllocChecker ac;

    {
        CountingArrPtr ptr1(new (&ac) DeleteCounter[1]);
        EXPECT_TRUE(ac.check());

        ptr1[0] = 4;
        {
            CountingArrPtr ptr2(new (&ac) DeleteCounter[1]);
            EXPECT_TRUE(ac.check());

            ptr2[0] = 7;
            ptr1.swap(ptr2);
            EXPECT_EQ(7, ptr1[0].value);
            EXPECT_EQ(4, ptr2[0].value);
        }
        EXPECT_EQ(1, destroy_count);
    }
    EXPECT_EQ(2, destroy_count);

    END_TEST;
}

static bool uptr_test_array_bool_op() {
    BEGIN_TEST;
    destroy_count = 0;

    fbl::AllocChecker ac;

    CountingArrPtr foo(new (&ac) DeleteCounter[1]);
    EXPECT_TRUE(ac.check());
    EXPECT_TRUE(static_cast<bool>(foo));

    foo.reset();
    EXPECT_EQ(1, destroy_count);
    EXPECT_FALSE(static_cast<bool>(foo));

    END_TEST;
}

static bool uptr_test_array_comparison() {
    BEGIN_TEST;

    fbl::AllocChecker ac;

    fbl::unique_ptr<DeleteCounter[]> null_unique;
    fbl::unique_ptr<DeleteCounter[]> lesser_unique(new (&ac) DeleteCounter[1]);
    EXPECT_TRUE(ac.check());
    fbl::unique_ptr<DeleteCounter[]> greater_unique(new (&ac) DeleteCounter[2]);
    EXPECT_TRUE(ac.check());

    EXPECT_NE(lesser_unique.get(), greater_unique.get());
    if (lesser_unique.get() > greater_unique.get())
        lesser_unique.swap(greater_unique);

    // Comparison against nullptr
    EXPECT_TRUE(   null_unique == nullptr);
    EXPECT_TRUE( lesser_unique != nullptr);
    EXPECT_TRUE(greater_unique != nullptr);

    EXPECT_TRUE(nullptr ==    null_unique);
    EXPECT_TRUE(nullptr !=  lesser_unique);
    EXPECT_TRUE(nullptr != greater_unique);

    // Comparison against other unique_ptr<>s
    EXPECT_TRUE( lesser_unique  ==  lesser_unique);
    EXPECT_FALSE( lesser_unique == greater_unique);
    EXPECT_FALSE(greater_unique ==  lesser_unique);
    EXPECT_TRUE(greater_unique  == greater_unique);

    EXPECT_FALSE( lesser_unique !=  lesser_unique);
    EXPECT_TRUE ( lesser_unique != greater_unique, "");
    EXPECT_TRUE (greater_unique !=  lesser_unique, "");
    EXPECT_FALSE(greater_unique != greater_unique);

    EXPECT_FALSE( lesser_unique <   lesser_unique);
    EXPECT_TRUE ( lesser_unique <  greater_unique, "");
    EXPECT_FALSE(greater_unique <   lesser_unique);
    EXPECT_FALSE(greater_unique <  greater_unique);

    EXPECT_FALSE( lesser_unique >   lesser_unique);
    EXPECT_FALSE( lesser_unique >  greater_unique);
    EXPECT_TRUE (greater_unique >   lesser_unique, "");
    EXPECT_FALSE(greater_unique >  greater_unique);

    EXPECT_TRUE ( lesser_unique <=  lesser_unique, "");
    EXPECT_TRUE ( lesser_unique <= greater_unique, "");
    EXPECT_FALSE(greater_unique <=  lesser_unique);
    EXPECT_TRUE (greater_unique <= greater_unique, "");

    EXPECT_TRUE ( lesser_unique >=  lesser_unique, "");
    EXPECT_FALSE( lesser_unique >= greater_unique);
    EXPECT_TRUE (greater_unique >=  lesser_unique, "");
    EXPECT_TRUE (greater_unique >= greater_unique, "");

    END_TEST;
}

namespace upcasting {

class A {
public:
    virtual ~A() { stuff_ = 0; }

private:
    volatile uint32_t stuff_;
};

class B {
public:
    ~B() { stuff_ = 1; }

private:
    volatile uint32_t stuff_;
};

class C : public A, public B {
public:
    ~C() { stuff_ = 2; }

private:
    volatile uint32_t stuff_;
};

class D {
public:
    virtual ~D() { stuff_ = 3; }

private:
    volatile uint32_t stuff_;
};

template <typename UptrType>
static bool handoff_fn(UptrType&& ptr) {
    BEGIN_TEST;
    EXPECT_NONNULL(ptr);
    END_TEST;
}

class OverloadTestHelper {
public:
    enum class Result {
        None,
        ClassA,
        ClassB,
        ClassD,
    };

    void PassByMove(fbl::unique_ptr<A>&&) { result_ = Result::ClassA; }
    void PassByMove(fbl::unique_ptr<D>&&) { result_ = Result::ClassD; }

#if TEST_WILL_NOT_COMPILE || 0
    // Enabling this overload should cause the overload test to fail to compile
    // due to ambiguity (it does not know whether to cast fbl::unique_ptr<C> to
    // fbl::unique_ptr<A> or fbl::unique_ptr<B>)
    void PassByMove(fbl::unique_ptr<B>&&) { result_ = Result::ClassB; }
#endif

    Result result() const { return result_; }

private:
    Result result_ = Result::None;
};


template <typename Base,
          typename Derived>
static bool test_upcast() {
    BEGIN_TEST;

    fbl::AllocChecker ac;

    fbl::unique_ptr<Derived> derived_ptr;

    // Construct unique_ptr<Base> with a move and implicit cast
    derived_ptr.reset(new (&ac) Derived());
    ASSERT_TRUE(ac.check());
    {
        EXPECT_NONNULL(derived_ptr);

        fbl::unique_ptr<Base> base_ptr(fbl::move(derived_ptr));

        EXPECT_NULL(derived_ptr);
        EXPECT_NONNULL(base_ptr);
    }

    // Assign unique_ptr<Base> at declaration time with a fbl::move
    derived_ptr.reset(new (&ac) Derived());
    ASSERT_TRUE(ac.check());
    {
        EXPECT_NONNULL(derived_ptr);

        fbl::unique_ptr<Base> base_ptr = fbl::move(derived_ptr);

        EXPECT_NULL(derived_ptr);
        EXPECT_NONNULL(base_ptr);
    }

    // Assign unique_ptr<Base> after declaration with a fbl::move
    derived_ptr.reset(new (&ac) Derived());
    ASSERT_TRUE(ac.check());
    {
        fbl::unique_ptr<Base> base_ptr;
        base_ptr = fbl::move(derived_ptr);
    }

    // Pass the pointer to a function with a move and an implicit cast
    derived_ptr.reset(new (&ac) Derived());
    ASSERT_TRUE(ac.check());
    {
        EXPECT_NONNULL(derived_ptr);

        bool test_res = handoff_fn<fbl::unique_ptr<Base>>(fbl::move(derived_ptr));

        EXPECT_NULL(derived_ptr);
        EXPECT_TRUE(test_res);
    }

#if TEST_WILL_NOT_COMPILE || 0
    // Construct unique_ptr<Base> without a move.
    derived_ptr.reset(new (&ac) Derived());
    ASSERT_TRUE(ac.check());
    {
        fbl::unique_ptr<Base> base_ptr(derived_ptr);
    }
#endif

#if TEST_WILL_NOT_COMPILE || 0
    // Assign unique_ptr<Base> at declaration time without a fbl::move.
    derived_ptr.reset(new (&ac) Derived());
    ASSERT_TRUE(ac.check());
    {
        fbl::unique_ptr<Base> base_ptr = derived_ptr;
    }
#endif

#if TEST_WILL_NOT_COMPILE || 0
    // Assign unique_ptr<Base> after declaration without a fbl::move.
    derived_ptr.reset(new (&ac) Derived());
    ASSERT_TRUE(ac.check());
    {
        fbl::unique_ptr<Base> base_ptr;
        base_ptr = derived_ptr;
    }
#endif

#if TEST_WILL_NOT_COMPILE || 0
    // Pass the pointer to a function with an implicit cast but without a move.
    derived_ptr.reset(new (&ac) Derived());
    ASSERT_TRUE(ac.check());
    {
        bool test_res = handoff_fn<fbl::unique_ptr<Base>>(derived_ptr);
        EXPECT_FALSE(test_res);
    }
#endif

    END_TEST;
}

static bool uptr_upcasting() {
    BEGIN_TEST;

    bool test_res;

    // This should work.  C derives from A, A has a virtual destructor, and
    // everything is using the default deleter.
    test_res = test_upcast<A, C>();
    EXPECT_TRUE(test_res);

#if TEST_WILL_NOT_COMPILE || 0
    // This should not work.  C derives from B, but B has no virtual destructor.
    test_res = test_upcast<B, C>();
    EXPECT_FALSE(test_res);
#endif

    // This should work even though B has no virtual destructor.
    {
        fbl::AllocChecker ac;
        fbl::unique_ptr<B> ptr(new (&ac) B());
        ASSERT_TRUE(ac.check());

        fbl::unique_ptr<const B> const_ptr;
        const_ptr = fbl::move(ptr);

        EXPECT_NULL(ptr);
        EXPECT_NONNULL(const_ptr);
    }

#if TEST_WILL_NOT_COMPILE || 0
    // This should not work.  D has a virtual destructor, but it is not a base
    // class of C.
    test_res = test_upcast<D, C>();
    EXPECT_FALSE(test_res);
#endif

    // Test overload resolution.  Make a C and the try to pass it to
    // OverloadTestHelper's various overloaded methods.  The compiler should
    // know which version to pick, and it should pick the unique_ptr<A> version,
    // not the unique_ptr<D> version.  If the TEST_WILL_NOT_COMPILE check is
    // enabled in OverloadTestHelper, a unique_ptr<B> version will be enabled as
    // well.  This should cause the build to break because of ambiguity.
    fbl::AllocChecker ac;
    fbl::unique_ptr<C> ptr(new (&ac) C());
    ASSERT_TRUE(ac.check());

    {
        // Now test pass by move.
        OverloadTestHelper helper;
        helper.PassByMove(fbl::move(ptr));

        EXPECT_NULL(ptr);
        EXPECT_EQ(OverloadTestHelper::Result::ClassA, helper.result());
    }

    END_TEST;
}

} // namespace upcasting

static bool uptr_test_make_unique() {
    BEGIN_TEST;

    // no alloc checker
    destroy_count = 0;
    {
        CountingPtr ptr = fbl::make_unique<DeleteCounter>(42);
        EXPECT_EQ(42, ptr->value, "value");
    }
    EXPECT_EQ(1, destroy_count);

    // with alloc checker
    destroy_count = 0;
    {
        fbl::AllocChecker ac;
        CountingPtr ptr = fbl::make_unique_checked<DeleteCounter>(&ac, 4242);
        EXPECT_TRUE(ac.check());
        EXPECT_EQ(4242, ptr->value, "value");
    }
    EXPECT_EQ(1, destroy_count);

    END_TEST;
}

static bool uptr_test_make_unique_array() {
    BEGIN_TEST;

    constexpr size_t array_size = 4;

    // no alloc checker
    destroy_count = 0;
    {
        CountingArrPtr ptr = fbl::make_unique<DeleteCounter[]>(array_size);
        EXPECT_NONNULL(ptr);
        for (size_t i = 0; i < array_size; ++i) {
            EXPECT_EQ(0, ptr[i].value);
        }
    }
    EXPECT_EQ(1, destroy_count);

    END_TEST;
}

BEGIN_TEST_CASE(unique_ptr)
RUN_NAMED_TEST("Scoped Destruction",               uptr_test_scoped_destruction)
RUN_NAMED_TEST("Move",                             uptr_test_move)
RUN_NAMED_TEST("nullptr Scoped Destruction",       uptr_test_null_scoped_destruction)
RUN_NAMED_TEST("Different Scope Swapping",         uptr_test_diff_scope_swap)
RUN_NAMED_TEST("operator bool",                    uptr_test_bool_op)
RUN_NAMED_TEST("comparison operators",             uptr_test_comparison)
RUN_NAMED_TEST("Array Scoped Destruction",         uptr_test_array_scoped_destruction)
RUN_NAMED_TEST("Array Move",                       uptr_test_array_move)
RUN_NAMED_TEST("Array nullptr Scoped Destruction", uptr_test_array_null_scoped_destruction)
RUN_NAMED_TEST("Array Different Scope Swapping",   uptr_test_array_diff_scope_swap)
RUN_NAMED_TEST("Array operator bool",              uptr_test_array_bool_op)
RUN_NAMED_TEST("Array comparison operators",       uptr_test_array_comparison)
RUN_NAMED_TEST("Upcast tests",                     upcasting::uptr_upcasting)
RUN_NAMED_TEST("Make unique",                      uptr_test_make_unique)
RUN_NAMED_TEST("Make unique array",                uptr_test_make_unique_array)
END_TEST_CASE(unique_ptr);
