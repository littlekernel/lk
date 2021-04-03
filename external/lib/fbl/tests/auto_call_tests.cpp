// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fbl/auto_call.h>
#include <fbl/type_support.h>
#include <unittest/unittest.h>

static volatile int test_func_count;

__NO_INLINE static void test_func()
{
    test_func_count++;
}

static bool auto_call_test()
{
//    extern int foo();
//    int a;
//
//    auto ac = MakeAutoCall([&](){ a = 1; });
//    auto ac2 = MakeAutoCall(foo);
//
//    auto func = [&](){ a = 2; };
//    AutoCall<decltype(bleh)> ac3(func);
//    AutoCall<decltype(&foo)> ac4(&foo);
//
//    // abort the call
//    ac2.cancel();

    BEGIN_TEST;

    // mark as volatile to make sure it generates code
    volatile int a;

    // call a lambda
    {
        a = 0;
        auto ac = fbl::MakeAutoCall([&](){ a++; });
        EXPECT_EQ(a, 0, "autocall hasn't run");
    }
    EXPECT_EQ(a, 1, "autocall has run");

    {
        a = 0;
        auto ac = fbl::MakeAutoCall([&](){ a++; });
        EXPECT_EQ(a, 0, "autocall hasn't run");

        ac.cancel();
        EXPECT_EQ(a, 0, "autocall still hasn't run");

        ac.call();
        EXPECT_EQ(a, 0, "autocall still hasn't run");
    }
    EXPECT_EQ(a, 0, "autocall still hasn't run");

    {
        a = 0;
        auto ac = fbl::MakeAutoCall([&](){ a++; });
        EXPECT_EQ(a, 0, "autocall hasn't run");

        ac.call();
        EXPECT_EQ(a, 1, "autocall should have run\n");

        ac.cancel();
        EXPECT_EQ(a, 1, "autocall ran only once\n");
    }
    EXPECT_EQ(a, 1, "autocall ran only once\n");

    // call a regular function
    {
        test_func_count = 0;
        auto ac = fbl::MakeAutoCall(&test_func);
        EXPECT_EQ(test_func_count, 0, "autocall hasn't run");
    }
    EXPECT_EQ(test_func_count, 1, "autocall has run");

    // move constructor
    {
        a = 0;
        auto ac = fbl::MakeAutoCall([&](){ a++; });
        auto ac2 = fbl::move(ac);
        EXPECT_EQ(a, 0, "autocall hasn't run");
    }
    EXPECT_EQ(a, 1, "autocall has run once");

    // move assignment
    {
        test_func_count = 0;
        auto ac = fbl::MakeAutoCall(&test_func);
        auto ac2 = fbl::MakeAutoCall(&test_func);
        EXPECT_EQ(test_func_count, 0, "autocall hasn't run");
        ac2 = fbl::move(ac);
        EXPECT_EQ(test_func_count, 1, "autocall has run once");
    }
    EXPECT_EQ(test_func_count, 2, "autocall has run twice");
    END_TEST;
}

BEGIN_TEST_CASE(auto_call_tests)
RUN_NAMED_TEST("Auto call test", auto_call_test)
END_TEST_CASE(auto_call_tests);
