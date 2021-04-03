// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>

#include <fbl/alloc_checker.h>
#include <fbl/type_support.h>
#include <fbl/unique_free_ptr.h>
#include <unittest/unittest.h>

static_assert(fbl::is_standard_layout<fbl::unique_free_ptr<int>>::value,
              "fbl::unique_free_ptr<int>'s should have a standard layout");

// These tests mostly serve to exercise the unique_free_ptr type and work well with heap checkers as
// the side effects of calling libc free() cannot be directly observed.
static bool ufptr_test_move() {
    BEGIN_TEST;

    // Construct and move into another unique_ptr.
    {
        fbl::unique_free_ptr<int> ptr(static_cast<int*>(malloc(sizeof(int))));

        fbl::unique_free_ptr<int> ptr2 = fbl::move(ptr);
        EXPECT_NULL(ptr, "expected ptr to be null");
    }

    END_TEST;
}

static bool ufptr_test_null_scoped_destruction() {
    BEGIN_TEST;

    // Construct a null unique_ptr and let it fall out of scope - should not call
    // deleter.
    {
        fbl::unique_free_ptr<int> ptr(nullptr);
    }

    END_TEST;
}

static bool ufptr_test_diff_scope_swap() {
    BEGIN_TEST;

    // Construct a pair of unique_ptrs in different scopes, swap them, and verify
    // that the values change places and that the values are destroyed at the
    // correct times.

    {
        fbl::unique_free_ptr<int> ptr1(static_cast<int*>(malloc(sizeof(int))));
        *ptr1 = 4;
        {
            fbl::unique_free_ptr<int> ptr2(static_cast<int*>(malloc(sizeof(int))));
            *ptr2 = 7;

            ptr1.swap(ptr2);
            EXPECT_EQ(7, *ptr1);
            EXPECT_EQ(4, *ptr2);
        }
    }

    END_TEST;
}

static bool ufptr_test_bool_op() {
    BEGIN_TEST;


    fbl::unique_free_ptr<int> foo(static_cast<int*>(malloc(sizeof(int))));
    EXPECT_TRUE(static_cast<bool>(foo));

    foo.reset();
    EXPECT_FALSE(static_cast<bool>(foo));

    END_TEST;
}

static bool ufptr_test_comparison() {
    BEGIN_TEST;

    // Test comparison operators.
    fbl::unique_free_ptr<int> null_unique;
    fbl::unique_free_ptr<int> lesser_unique(static_cast<int*>(malloc(sizeof(int))));
    *lesser_unique = 1;

    fbl::unique_free_ptr<int> greater_unique(static_cast<int*>(malloc(sizeof(int))));
    *greater_unique = 2;

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

    // Comparison against other unique_free_ptr<>s
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

BEGIN_TEST_CASE(unique_free_ptr)
RUN_NAMED_TEST("Move",                             ufptr_test_move)
RUN_NAMED_TEST("nullptr Scoped Destruction",       ufptr_test_null_scoped_destruction)
RUN_NAMED_TEST("Different Scope Swapping",         ufptr_test_diff_scope_swap)
RUN_NAMED_TEST("operator bool",                    ufptr_test_bool_op)
RUN_NAMED_TEST("comparison operators",             ufptr_test_comparison)
END_TEST_CASE(unique_free_ptr);
