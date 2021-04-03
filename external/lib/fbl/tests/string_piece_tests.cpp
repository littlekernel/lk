// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fbl/string_piece.h>

#include <fbl/algorithm.h>
#include <fbl/type_support.h>
#include <unittest/unittest.h>

namespace {

bool empty_string_test() {
    BEGIN_TEST;

    fbl::StringPiece empty;

    EXPECT_NULL(empty.data());

    EXPECT_EQ(0u, empty.length());
    EXPECT_EQ(0u, empty.size());
    EXPECT_TRUE(empty.empty());

    EXPECT_NULL(empty.begin());
    EXPECT_NULL(empty.end());
    EXPECT_NULL(empty.cbegin());
    EXPECT_NULL(empty.cend());

    END_TEST;
}

bool non_empty_string_test() {
    BEGIN_TEST;

    const char data[] = "abc";

    fbl::StringPiece str1(data);

    EXPECT_EQ(data, str1.data());

    EXPECT_EQ(3u, str1.length());
    EXPECT_EQ(3u, str1.size());
    EXPECT_FALSE(str1.empty());

    EXPECT_EQ(data, str1.begin());
    EXPECT_EQ(data + 3u, str1.end());
    EXPECT_EQ(data, str1.cbegin());
    EXPECT_EQ(data + 3u, str1.cend());

    EXPECT_EQ('b', str1[1u]);

    fbl::StringPiece str2(data + 1u, 2u);

    EXPECT_EQ(data + 1u, str2.data());

    EXPECT_EQ(2u, str2.length());
    EXPECT_EQ(2u, str2.size());
    EXPECT_FALSE(str2.empty());

    EXPECT_EQ(data + 1u, str2.begin());
    EXPECT_EQ(data + 3u, str2.end());
    EXPECT_EQ(data + 1u, str2.cbegin());
    EXPECT_EQ(data + 3u, str2.cend());

    EXPECT_EQ('c', str2[1u]);

    END_TEST;
}

bool copy_move_and_assignment_test() {
    BEGIN_TEST;

    const char data[] = "abc";

    {
        fbl::StringPiece abc(data);
        fbl::StringPiece str(abc);
        EXPECT_EQ(data, str.data());
        EXPECT_EQ(3u, str.length());
    }

    {
        fbl::StringPiece abc(data);
        fbl::StringPiece str(fbl::move(abc));
        EXPECT_EQ(data, str.data());
        EXPECT_EQ(3u, str.length());
    }

    {
        fbl::StringPiece abc(data);
        fbl::StringPiece str;
        str = abc;
        EXPECT_EQ(data, str.data());
        EXPECT_EQ(3u, str.length());
    }

    {
        fbl::StringPiece abc(data);
        fbl::StringPiece str;
        str = fbl::move(abc);
        EXPECT_EQ(data, str.data());
        EXPECT_EQ(3u, str.length());
    }

    {
        fbl::StringPiece str;
        str = data;
        EXPECT_EQ(data, str.data());
        EXPECT_EQ(3u, str.length());

        str = nullptr;
        EXPECT_NULL(str.data());
        EXPECT_EQ(0u, str.length());
    }

    END_TEST;
}

bool set_clear_test() {
    BEGIN_TEST;

    const char data[] = "abc";

    fbl::StringPiece str;
    EXPECT_NULL(str.data());
    EXPECT_EQ(0u, str.length());

    str.set(data);
    EXPECT_EQ(data, str.data());
    EXPECT_EQ(3u, str.length());

    str.set(nullptr);
    EXPECT_NULL(str.data());
    EXPECT_EQ(0u, str.length());

    str.set(data, 2);
    EXPECT_EQ(data, str.data());
    EXPECT_EQ(2u, str.length());

    str.clear();
    EXPECT_NULL(str.data());
    EXPECT_EQ(0u, str.length());

    END_TEST;
}

bool compare_test() {
    BEGIN_TEST;

    const char data[] = "abc";
    fbl::StringPiece empty;
    fbl::StringPiece a(data, 1);
    fbl::StringPiece ab(data, 2);
    fbl::StringPiece b(data + 1, 1);
    fbl::StringPiece bc(data + 1, 2);

    EXPECT_EQ(0, empty.compare(empty));
    EXPECT_EQ(-1, empty.compare(a));
    EXPECT_EQ(1, a.compare(empty));

    EXPECT_EQ(0, a.compare(a));
    EXPECT_EQ(0, ab.compare(ab));
    EXPECT_EQ(-1, a.compare(ab));
    EXPECT_EQ(1, ab.compare(a));
    EXPECT_EQ(-1, ab.compare(bc));
    EXPECT_EQ(1, bc.compare(ab));

    EXPECT_TRUE(empty == empty);
    EXPECT_TRUE(empty <= empty);
    EXPECT_TRUE(empty >= empty);
    EXPECT_FALSE(empty != empty);
    EXPECT_FALSE(empty < empty);
    EXPECT_FALSE(empty > empty);
    EXPECT_TRUE(empty < a);
    EXPECT_TRUE(a > empty);

    EXPECT_TRUE(a == a);
    EXPECT_TRUE(ab == ab);
    EXPECT_TRUE(a != ab);
    EXPECT_TRUE(a != b);
    EXPECT_TRUE(ab != a);

    EXPECT_FALSE(a < a);
    EXPECT_FALSE(a > a);
    EXPECT_TRUE(a >= a);
    EXPECT_TRUE(a <= a);

    EXPECT_TRUE(a < ab);
    EXPECT_FALSE(a > ab);
    EXPECT_FALSE(a >= ab);
    EXPECT_TRUE(a <= ab);

    EXPECT_FALSE(ab < a);
    EXPECT_TRUE(ab > a);
    EXPECT_TRUE(ab >= a);
    EXPECT_FALSE(ab <= a);

    EXPECT_TRUE(a < b);
    EXPECT_FALSE(a > b);
    EXPECT_FALSE(a >= b);
    EXPECT_TRUE(a <= b);

    EXPECT_FALSE(b < a);
    EXPECT_TRUE(b > a);
    EXPECT_TRUE(b >= a);
    EXPECT_FALSE(b <= a);

    EXPECT_TRUE(a < bc);
    EXPECT_FALSE(a > bc);
    EXPECT_FALSE(a >= bc);
    EXPECT_TRUE(a <= bc);

    EXPECT_FALSE(bc < a);
    EXPECT_TRUE(bc > a);
    EXPECT_TRUE(bc >= a);
    EXPECT_FALSE(bc <= a);

    END_TEST;
}

constexpr char kFakeStringData[] = "hello";
constexpr size_t kFakeStringLength = fbl::count_of(kFakeStringData);

struct SimpleFakeString {
    const char* data() const { return kFakeStringData; }
    size_t length() const { return kFakeStringLength; }
};

struct OverloadedFakeString {
    const char* data() const { return kFakeStringData; }
    size_t length() const { return kFakeStringLength; }

    // These are decoys to verify that the conversion operator only considers
    // the const overloads of these members.
    void data();
    void length();
};

struct EmptyString {
    const char* data() const { return nullptr; }
    size_t length() const { return 0u; }
};

bool conversion_from_string_like_object() {
    BEGIN_TEST;

    {
        SimpleFakeString str;
        fbl::StringPiece p(str);
        EXPECT_EQ(kFakeStringData, p.data());
        EXPECT_EQ(kFakeStringLength, p.length());
    }

    {
        OverloadedFakeString str;
        fbl::StringPiece p(str);
        EXPECT_EQ(kFakeStringData, p.data());
        EXPECT_EQ(kFakeStringLength, p.length());
    }

    {
        EmptyString str;
        fbl::StringPiece p(str);
        EXPECT_NULL(p.data());
        EXPECT_EQ(0u, p.length());
    }

    END_TEST;
}

bool assignment_from_string_like_object() {
    BEGIN_TEST;

    {
        SimpleFakeString str;
        fbl::StringPiece p;
        p = str;
        EXPECT_EQ(kFakeStringData, p.data());
        EXPECT_EQ(kFakeStringLength, p.length());
    }

    {
        OverloadedFakeString str;
        fbl::StringPiece p;
        p = str;
        EXPECT_EQ(kFakeStringData, p.data());
        EXPECT_EQ(kFakeStringLength, p.length());
    }

    {
        EmptyString str;
        fbl::StringPiece p("abc");
        p = str;
        EXPECT_NULL(p.data());
        EXPECT_EQ(0u, p.length());
    }

    END_TEST;
}

} // namespace

BEGIN_TEST_CASE(string_piece_tests)
RUN_TEST(empty_string_test)
RUN_TEST(non_empty_string_test)
RUN_TEST(copy_move_and_assignment_test)
RUN_TEST(set_clear_test)
RUN_TEST(compare_test)
RUN_TEST(conversion_from_string_like_object)
RUN_TEST(assignment_from_string_like_object)
END_TEST_CASE(string_piece_tests)
