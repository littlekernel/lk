// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>
#include <unittest/unittest.h>
#include <fbl/type_support.h>

enum Category { CAT_LVALUE, CAT_RVALUE };

static int category(const int& arg) { return CAT_LVALUE; }

static int category(int&& arg) { return CAT_RVALUE; }

template <typename T>
static int passing(T&& t)
{
    return category(t);
}

template <typename T>
static int moving(T&& t)
{
    return category(fbl::move(t));
}

template <typename T>
static int forwarding(T&& t)
{
    return category(fbl::forward<T>(t));
}

template <typename T>
static int forward_copy(T&& t)
{
    return category(fbl::forward<T&>(t));
}

struct A {
    A(int&& n) : category(CAT_RVALUE) {}
    A(int& n) : category(CAT_LVALUE) {}
    int category;
};

template <typename T, typename U>
static T make_object(U&& u)
{
    return T(fbl::forward<U>(u));
}

static bool forward_test()
{
    BEGIN_TEST;
    int val = 42;
    int& ref = val;
    const int& cref = val;

    EXPECT_EQ(CAT_LVALUE, passing(42));
    EXPECT_EQ(CAT_LVALUE, passing(val));
    EXPECT_EQ(CAT_LVALUE, passing(ref));
    EXPECT_EQ(CAT_LVALUE, passing(cref));
    EXPECT_EQ(CAT_LVALUE, passing(val + 1));

    EXPECT_EQ(CAT_RVALUE, moving(42));
    EXPECT_EQ(CAT_RVALUE, moving(val));
    EXPECT_EQ(CAT_RVALUE, moving(ref));
    EXPECT_EQ(CAT_LVALUE, moving(cref));
    EXPECT_EQ(CAT_RVALUE, moving(val + 1));

    EXPECT_EQ(CAT_RVALUE, forwarding(42));
    EXPECT_EQ(CAT_LVALUE, forwarding(val));
    EXPECT_EQ(CAT_LVALUE, forwarding(ref));
    EXPECT_EQ(CAT_LVALUE, forward_copy(cref));
    EXPECT_EQ(CAT_RVALUE, forwarding(val + 1));

    EXPECT_EQ(CAT_LVALUE, forward_copy(42));
    EXPECT_EQ(CAT_LVALUE, forward_copy(val));
    EXPECT_EQ(CAT_LVALUE, forward_copy(ref));
    EXPECT_EQ(CAT_LVALUE, forward_copy(cref));
    EXPECT_EQ(CAT_LVALUE, forward_copy(val + 1));

    auto a1 = make_object<A>(42);
    auto a2 = make_object<A>(val);

    EXPECT_EQ(CAT_RVALUE, a1.category);
    EXPECT_EQ(CAT_LVALUE, a2.category);

    END_TEST;
}

BEGIN_TEST_CASE(forward_tests)
RUN_NAMED_TEST("Forward test", forward_test)
END_TEST_CASE(forward_tests);
