// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fbl/string.h>

#include <fbl/algorithm.h>
#include <fbl/type_support.h>
#include <unittest/unittest.h>

namespace fbl {
namespace tests {
struct StringTestHelper {
    static unsigned int GetRefCount(const String& str) {
        return str.ref_count();
    }
};
} // namespace tests
} // namespace fbl

using fbl::tests::StringTestHelper;

namespace {

bool empty_string_test() {
    BEGIN_TEST;

    {
        fbl::String empty;

        EXPECT_STR_EQ("", empty.data());
        EXPECT_STR_EQ("", empty.c_str());

        EXPECT_EQ(0u, empty.length());
        EXPECT_EQ(0u, empty.size());
        EXPECT_TRUE(empty.empty());

        EXPECT_STR_EQ("", empty.begin());
        EXPECT_EQ(0u, empty.end() - empty.begin());
        EXPECT_STR_EQ("", empty.cbegin());
        EXPECT_EQ(0u, empty.cend() - empty.cbegin());

        EXPECT_EQ(0, empty[0u]);
    }

    {
        fbl::String empty("");

        EXPECT_STR_EQ("", empty.data());
        EXPECT_STR_EQ("", empty.c_str());

        EXPECT_EQ(0u, empty.length());
        EXPECT_EQ(0u, empty.size());
        EXPECT_TRUE(empty.empty());

        EXPECT_STR_EQ("", empty.begin());
        EXPECT_EQ(0u, empty.end() - empty.begin());
        EXPECT_STR_EQ("", empty.cbegin());
        EXPECT_EQ(0u, empty.cend() - empty.cbegin());

        EXPECT_EQ(0, empty[0u]);
    }

    {
        fbl::String empty("abcde", size_t(0u));

        EXPECT_STR_EQ("", empty.data());
        EXPECT_STR_EQ("", empty.c_str());

        EXPECT_EQ(0u, empty.length());
        EXPECT_EQ(0u, empty.size());
        EXPECT_TRUE(empty.empty());

        EXPECT_STR_EQ("", empty.begin());
        EXPECT_EQ(0u, empty.end() - empty.begin());
        EXPECT_STR_EQ("", empty.cbegin());
        EXPECT_EQ(0u, empty.cend() - empty.cbegin());

        EXPECT_EQ(0, empty[0u]);
    }

    {
        fbl::String empty(0u, 'x');

        EXPECT_STR_EQ("", empty.data());
        EXPECT_STR_EQ("", empty.c_str());

        EXPECT_EQ(0u, empty.length());
        EXPECT_EQ(0u, empty.size());
        EXPECT_TRUE(empty.empty());

        EXPECT_STR_EQ("", empty.begin());
        EXPECT_EQ(0u, empty.end() - empty.begin());
        EXPECT_STR_EQ("", empty.cbegin());
        EXPECT_EQ(0u, empty.cend() - empty.cbegin());

        EXPECT_EQ(0, empty[0u]);
    }

    {
        fbl::String empty(fbl::StringPiece("abcde", 0u));

        EXPECT_STR_EQ("", empty.data());
        EXPECT_STR_EQ("", empty.c_str());

        EXPECT_EQ(0u, empty.length());
        EXPECT_EQ(0u, empty.size());
        EXPECT_TRUE(empty.empty());

        EXPECT_STR_EQ("", empty.begin());
        EXPECT_EQ(0u, empty.end() - empty.begin());
        EXPECT_STR_EQ("", empty.cbegin());
        EXPECT_EQ(0u, empty.cend() - empty.cbegin());

        EXPECT_EQ(0, empty[0u]);
    }

    END_TEST;
}

bool non_empty_string_test() {
    BEGIN_TEST;

    {
        fbl::String str("abc");

        EXPECT_STR_EQ("abc", str.data());

        EXPECT_EQ(3u, str.length());
        EXPECT_EQ(3u, str.size());
        EXPECT_FALSE(str.empty());

        EXPECT_STR_EQ("abc", str.begin());
        EXPECT_EQ(3u, str.end() - str.begin());
        EXPECT_STR_EQ("abc", str.cbegin());
        EXPECT_EQ(3u, str.cend() - str.cbegin());

        EXPECT_EQ('b', str[1u]);
    }

    {
        fbl::String str("abc", 2u);

        EXPECT_STR_EQ("ab", str.data());

        EXPECT_EQ(2u, str.length());
        EXPECT_EQ(2u, str.size());
        EXPECT_FALSE(str.empty());

        EXPECT_STR_EQ("ab", str.begin());
        EXPECT_EQ(2u, str.end() - str.begin());
        EXPECT_STR_EQ("ab", str.cbegin());
        EXPECT_EQ(2u, str.cend() - str.cbegin());

        EXPECT_EQ('b', str[1u]);
    }

    {
        fbl::String str(10u, 'x');

        EXPECT_STR_EQ("xxxxxxxxxx", str.data());

        EXPECT_EQ(10u, str.length());
        EXPECT_EQ(10u, str.size());
        EXPECT_FALSE(str.empty());

        EXPECT_STR_EQ("xxxxxxxxxx", str.begin());
        EXPECT_EQ(10u, str.end() - str.begin());
        EXPECT_STR_EQ("xxxxxxxxxx", str.cbegin());
        EXPECT_EQ(10u, str.cend() - str.cbegin());

        EXPECT_EQ('x', str[1u]);
    }

    {
        fbl::String str(fbl::StringPiece("abcdef", 2u));

        EXPECT_STR_EQ("ab", str.data());

        EXPECT_EQ(2u, str.length());
        EXPECT_EQ(2u, str.size());
        EXPECT_FALSE(str.empty());

        EXPECT_STR_EQ("ab", str.begin());
        EXPECT_EQ(2u, str.end() - str.begin());
        EXPECT_STR_EQ("ab", str.cbegin());
        EXPECT_EQ(2u, str.cend() - str.cbegin());

        EXPECT_EQ('b', str[1u]);
    }

    END_TEST;
}

bool copy_move_and_assignment_test() {
    BEGIN_TEST;

    {
        fbl::String abc("abc");
        fbl::String copy(abc);
        EXPECT_STR_EQ("abc", abc.data());
        EXPECT_EQ(abc.data(), copy.data());
        EXPECT_EQ(3u, copy.length());
    }

    {
        fbl::String abc("abc");
        fbl::String copy(abc);
        fbl::String move(fbl::move(copy));
        EXPECT_STR_EQ("abc", abc.data());
        EXPECT_STR_EQ("", copy.data());
        EXPECT_EQ(abc.data(), move.data());
        EXPECT_EQ(3u, move.length());
    }

    {
        fbl::String abc("abc");
        fbl::String str;
        str = abc;
        EXPECT_STR_EQ("abc", abc.data());
        EXPECT_EQ(abc.data(), str.data());
        EXPECT_EQ(3u, str.length());
    }

    {
        fbl::String abc("abc");
        fbl::String copy(abc);
        fbl::String str;
        str = fbl::move(copy);
        EXPECT_STR_EQ("abc", abc.data());
        EXPECT_STR_EQ("", copy.data());
        EXPECT_EQ(abc.data(), str.data());
        EXPECT_EQ(3u, str.length());
    }

    {
        fbl::String str;
        str = "abc";
        EXPECT_STR_EQ("abc", str.data());
        EXPECT_EQ(3u, str.length());

        str = "";
        EXPECT_STR_EQ("", str.data());
        EXPECT_EQ(0u, str.length());

        fbl::String copy(str);
        EXPECT_STR_EQ("", copy.data());
        EXPECT_EQ(0u, copy.length());

        fbl::String move(copy);
        EXPECT_STR_EQ("", copy.data());
        EXPECT_EQ(0u, copy.length());
        EXPECT_STR_EQ("", move.data());
        EXPECT_EQ(0u, move.length());
    }

    END_TEST;
}

bool set_clear_test() {
    BEGIN_TEST;

    fbl::String str;
    EXPECT_STR_EQ("", str.data());
    EXPECT_EQ(0u, str.length());

    str.Set("abc");
    EXPECT_STR_EQ("abc", str.data());
    EXPECT_EQ(3u, str.length());

    str.Set("");
    EXPECT_STR_EQ("", str.data());
    EXPECT_EQ(0u, str.length());

    str.Set("abc", 2u);
    EXPECT_STR_EQ("ab", str.data());
    EXPECT_EQ(2u, str.length());

    str.Set(0u, 'x');
    EXPECT_STR_EQ("", str.data());
    EXPECT_EQ(0u, str.length());

    str.Set(10u, 'x');
    EXPECT_STR_EQ("xxxxxxxxxx", str.data());
    EXPECT_EQ(10u, str.length());

    str.Set(fbl::StringPiece("abcdef", 0u));
    EXPECT_STR_EQ("", str.data());
    EXPECT_EQ(0u, str.length());

    str.Set(fbl::StringPiece("abc", 2u));
    EXPECT_STR_EQ("ab", str.data());
    EXPECT_EQ(2u, str.length());

    str.clear();
    EXPECT_STR_EQ("", str.data());
    EXPECT_EQ(0u, str.length());

    END_TEST;
}

bool compare_test() {
    BEGIN_TEST;

    const char data[] = "abc";
    fbl::String empty;
    fbl::String a(data, 1);
    fbl::String ab(data, 2);
    fbl::String b(data + 1, 1);
    fbl::String bc(data + 1, 2);

    EXPECT_EQ(0, empty.compare(empty));
    EXPECT_LT(empty.compare(a), 0);
    EXPECT_GT(a.compare(empty), 0);

    EXPECT_EQ(0, a.compare(a));
    EXPECT_EQ(0, ab.compare(ab));
    EXPECT_LT(a.compare(ab), 0);
    EXPECT_GT(ab.compare(a), 0);
    EXPECT_LT(ab.compare(bc), 0);
    EXPECT_GT(bc.compare(ab), 0);

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

bool concat_test() {
    BEGIN_TEST;

    {
        fbl::String empty = fbl::String::Concat({});
        EXPECT_STR_EQ("", empty.c_str());
        EXPECT_EQ(0u, empty.length());
    }

    {
        fbl::String empty = fbl::String::Concat({""});
        EXPECT_STR_EQ("", empty.c_str());
        EXPECT_EQ(0u, empty.length());
    }

    {
        fbl::String empty = fbl::String::Concat({"", "", "", ""});
        EXPECT_STR_EQ("", empty.c_str());
        EXPECT_EQ(0u, empty.length());
    }

    {
        fbl::String str = fbl::String::Concat({"abc"});
        EXPECT_STR_EQ("abc", str.c_str());
        EXPECT_EQ(3u, str.length());
    }

    {
        fbl::String str = fbl::String::Concat({"abc", "def"});
        EXPECT_STR_EQ("abcdef", str.c_str());
        EXPECT_EQ(6u, str.length());
    }

    {
        fbl::String str = fbl::String::Concat({"abc", "", "def"});
        EXPECT_STR_EQ("abcdef", str.c_str());
        EXPECT_EQ(6u, str.length());
    }

    {
        fbl::String str = fbl::String::Concat({"abc", "def", ""});
        EXPECT_STR_EQ("abcdef", str.c_str());
        EXPECT_EQ(6u, str.length());
    }

    {
        fbl::String str = fbl::String::Concat({"", "abc", "def"});
        EXPECT_STR_EQ("abcdef", str.c_str());
        EXPECT_EQ(6u, str.length());
    }

    {
        fbl::String str = fbl::String::Concat({"abc", "def", "g", "hi", "jklmnop"});
        EXPECT_STR_EQ("abcdefghijklmnop", str.c_str());
        EXPECT_EQ(16u, str.length());
    }

    END_TEST;
}

bool alloc_checker_test() {
    BEGIN_TEST;

    // Empty constructor

    {
        fbl::AllocChecker ac;
        fbl::String empty("", &ac);
        EXPECT_TRUE(ac.check());
        EXPECT_STR_EQ("", empty.data());
        EXPECT_EQ(0u, empty.length());
    }

    {
        fbl::AllocChecker ac;
        fbl::String empty("abcdef", 0u, &ac);
        EXPECT_TRUE(ac.check());
        EXPECT_STR_EQ("", empty.data());
        EXPECT_EQ(0u, empty.length());
    }

    {
        fbl::AllocChecker ac;
        fbl::String str(0u, 'x', &ac);
        EXPECT_TRUE(ac.check());
        EXPECT_STR_EQ("", str.data());
        EXPECT_EQ(0u, str.length());
    }

    {
        fbl::AllocChecker ac;
        fbl::String empty(fbl::StringPiece("abcdef", 0u), &ac);
        EXPECT_TRUE(ac.check());
        EXPECT_STR_EQ("", empty.data());
        EXPECT_EQ(0u, empty.length());
    }

    // Empty setter

    {
        fbl::AllocChecker ac;
        fbl::String empty("?");
        empty.Set("", &ac);
        EXPECT_TRUE(ac.check());
        EXPECT_STR_EQ("", empty.data());
        EXPECT_EQ(0u, empty.length());
    }

    {
        fbl::AllocChecker ac;
        fbl::String empty("?");
        empty.Set("abcdef", 0u, &ac);
        EXPECT_TRUE(ac.check());
        EXPECT_STR_EQ("", empty.data());
        EXPECT_EQ(0u, empty.length());
    }

    {
        fbl::AllocChecker ac;
        fbl::String str;
        str.Set(0u, 'x', &ac);
        EXPECT_TRUE(ac.check());
        EXPECT_STR_EQ("", str.data());
        EXPECT_EQ(0u, str.length());
    }

    {
        fbl::AllocChecker ac;
        fbl::String empty("?");
        empty.Set(fbl::StringPiece("abcdef", 0u), &ac);
        EXPECT_TRUE(ac.check());
        EXPECT_STR_EQ("", empty.data());
        EXPECT_EQ(0u, empty.length());
    }

    // Non-empty constructor

    {
        fbl::AllocChecker ac;
        fbl::String str("abc", &ac);
        EXPECT_TRUE(ac.check());
        EXPECT_STR_EQ("abc", str.data());
        EXPECT_EQ(3u, str.length());
    }

    {
        fbl::AllocChecker ac;
        fbl::String str("abcdef", 5u, &ac);
        EXPECT_TRUE(ac.check());
        EXPECT_STR_EQ("abcde", str.data());
        EXPECT_EQ(5u, str.length());
    }

    {
        fbl::AllocChecker ac;
        fbl::String str(10u, 'x', &ac);
        EXPECT_TRUE(ac.check());
        EXPECT_STR_EQ("xxxxxxxxxx", str.data());
        EXPECT_EQ(10u, str.length());
    }

    {
        fbl::AllocChecker ac;
        fbl::String str(fbl::StringPiece("abcdef", 5u), &ac);
        EXPECT_TRUE(ac.check());
        EXPECT_STR_EQ("abcde", str.data());
        EXPECT_EQ(5u, str.length());
    }

    // Non-empty setter

    {
        fbl::AllocChecker ac;
        fbl::String str;
        str.Set("abc", &ac);
        EXPECT_TRUE(ac.check());
        EXPECT_STR_EQ("abc", str.data());
        EXPECT_EQ(3u, str.length());
    }

    {
        fbl::AllocChecker ac;
        fbl::String str;
        str.Set("abcdef", 5u, &ac);
        EXPECT_TRUE(ac.check());
        EXPECT_STR_EQ("abcde", str.data());
        EXPECT_EQ(5u, str.length());
    }

    {
        fbl::AllocChecker ac;
        fbl::String str;
        str.Set(10u, 'x', &ac);
        EXPECT_TRUE(ac.check());
        EXPECT_STR_EQ("xxxxxxxxxx", str.data());
        EXPECT_EQ(10u, str.length());
    }

    {
        fbl::AllocChecker ac;
        fbl::String str;
        str.Set(fbl::StringPiece("abcdef", 5u), &ac);
        EXPECT_TRUE(ac.check());
        EXPECT_STR_EQ("abcde", str.data());
        EXPECT_EQ(5u, str.length());
    }

    // Concat

    {
        fbl::AllocChecker ac;
        fbl::String empty = fbl::String::Concat({}, &ac);
        EXPECT_TRUE(ac.check());
        EXPECT_STR_EQ("", empty.c_str());
        EXPECT_EQ(0u, empty.length());
    }

    {
        fbl::AllocChecker ac;
        fbl::String str = fbl::String::Concat({"abc", "def", "g", "hi", "jklmnop"}, &ac);
        EXPECT_TRUE(ac.check());
        EXPECT_STR_EQ("abcdefghijklmnop", str.c_str());
        EXPECT_EQ(16u, str.length());
    }

    END_TEST;
}

bool to_string_piece_test() {
    BEGIN_TEST;

    {
        fbl::String empty;
        fbl::StringPiece piece(empty.ToStringPiece());
        EXPECT_EQ(empty.data(), piece.data());
        EXPECT_EQ(0u, piece.length());
    }

    {
        fbl::String str("abc");
        fbl::StringPiece piece(str.ToStringPiece());
        EXPECT_EQ(str.data(), piece.data());
        EXPECT_EQ(3u, piece.length());
    }

    END_TEST;
}

bool swap_test() {
    BEGIN_TEST;

    fbl::String empty;
    fbl::String abc("abc");
    fbl::String def("def");

    abc.swap(def);
    empty.swap(abc);

    EXPECT_STR_EQ("def", empty.data());
    EXPECT_STR_EQ("", abc.data());
    EXPECT_STR_EQ("abc", def.data());

    END_TEST;
}

bool ref_count_test() {
    BEGIN_TEST;

    // Empty strings

    {
        fbl::String empty;
        unsigned int initial_ref_count = StringTestHelper::GetRefCount(empty);
        EXPECT_GT(initial_ref_count, 1u);
        {
            fbl::String copy(empty);
            EXPECT_EQ(empty.data(), copy.data());
            EXPECT_EQ(initial_ref_count + 1u, StringTestHelper::GetRefCount(empty));
            {
                fbl::String another_empty("");
                EXPECT_EQ(empty.data(), another_empty.data());
                EXPECT_EQ(initial_ref_count + 2u, StringTestHelper::GetRefCount(empty));
                {
                    fbl::String assigned_from_empty = another_empty;
                    EXPECT_EQ(empty.data(), assigned_from_empty.data());
                    EXPECT_EQ(initial_ref_count + 3u, StringTestHelper::GetRefCount(empty));

                    assigned_from_empty = "";
                    EXPECT_EQ(empty.data(), assigned_from_empty.data());
                    EXPECT_EQ(initial_ref_count + 3u, StringTestHelper::GetRefCount(empty));

                    assigned_from_empty = empty;
                    EXPECT_EQ(empty.data(), assigned_from_empty.data());
                    EXPECT_EQ(initial_ref_count + 3u, StringTestHelper::GetRefCount(empty));

                    assigned_from_empty.clear();
                    EXPECT_EQ(empty.data(), assigned_from_empty.data());
                    EXPECT_EQ(initial_ref_count + 3u, StringTestHelper::GetRefCount(empty));
                }
                EXPECT_EQ(initial_ref_count + 2u, StringTestHelper::GetRefCount(empty));
            }
            EXPECT_EQ(initial_ref_count + 1u, StringTestHelper::GetRefCount(empty));
        }
        EXPECT_EQ(initial_ref_count, StringTestHelper::GetRefCount(empty));
    }

    // C-string initialized strings.

    {
        fbl::String abc("abc");
        EXPECT_EQ(1u, StringTestHelper::GetRefCount(abc));
        {
            fbl::String copy1(abc);
            EXPECT_EQ(abc.data(), copy1.data());
            EXPECT_EQ(2u, StringTestHelper::GetRefCount(abc));
            {
                fbl::String copy2(abc);
                EXPECT_EQ(abc.data(), copy2.data());
                EXPECT_EQ(3u, StringTestHelper::GetRefCount(abc));
                {
                    fbl::String assigned_from_abc = abc;
                    EXPECT_EQ(abc.data(), assigned_from_abc.data());
                    EXPECT_EQ(4u, StringTestHelper::GetRefCount(abc));

                    assigned_from_abc = "";
                    EXPECT_STR_EQ("", assigned_from_abc.data());
                    EXPECT_EQ(3u, StringTestHelper::GetRefCount(abc));

                    assigned_from_abc = abc;
                    EXPECT_EQ(abc.data(), assigned_from_abc.data());
                    EXPECT_EQ(4u, StringTestHelper::GetRefCount(abc));

                    assigned_from_abc.clear();
                    EXPECT_STR_EQ("", assigned_from_abc.data());
                    EXPECT_EQ(3u, StringTestHelper::GetRefCount(abc));
                }
                EXPECT_EQ(3u, StringTestHelper::GetRefCount(abc));
            }
            EXPECT_EQ(2u, StringTestHelper::GetRefCount(abc));
        }
        EXPECT_EQ(1u, StringTestHelper::GetRefCount(abc));
    }

    // Repeated character initialized strings.

    {
        fbl::String xs(10u, 'x');
        EXPECT_EQ(1u, StringTestHelper::GetRefCount(xs));
        {
            fbl::String copy1(xs);
            EXPECT_EQ(xs.data(), copy1.data());
            EXPECT_EQ(2u, StringTestHelper::GetRefCount(xs));
            {
                fbl::String copy2(xs);
                EXPECT_EQ(xs.data(), copy2.data());
                EXPECT_EQ(3u, StringTestHelper::GetRefCount(xs));
                {
                    fbl::String assigned_from_xs = xs;
                    EXPECT_EQ(xs.data(), assigned_from_xs.data());
                    EXPECT_EQ(4u, StringTestHelper::GetRefCount(xs));

                    assigned_from_xs = "";
                    EXPECT_STR_EQ("", assigned_from_xs.data());
                    EXPECT_EQ(3u, StringTestHelper::GetRefCount(xs));

                    assigned_from_xs = xs;
                    EXPECT_EQ(xs.data(), assigned_from_xs.data());
                    EXPECT_EQ(4u, StringTestHelper::GetRefCount(xs));

                    assigned_from_xs.clear();
                    EXPECT_STR_EQ("", assigned_from_xs.data());
                    EXPECT_EQ(3u, StringTestHelper::GetRefCount(xs));
                }
                EXPECT_EQ(3u, StringTestHelper::GetRefCount(xs));
            }
            EXPECT_EQ(2u, StringTestHelper::GetRefCount(xs));
        }
        EXPECT_EQ(1u, StringTestHelper::GetRefCount(xs));
    }

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
        fbl::String p(str);
        EXPECT_STR_EQ(kFakeStringData, p.data());
        EXPECT_EQ(kFakeStringLength, p.length());
    }

    {
        OverloadedFakeString str;
        fbl::String p(str);
        EXPECT_STR_EQ(kFakeStringData, p.data());
        EXPECT_EQ(kFakeStringLength, p.length());
    }

    {
        EmptyString str;
        fbl::String p(str);
        EXPECT_STR_EQ("", p.data());
        EXPECT_EQ(0u, p.length());
    }

    END_TEST;
}

bool assignment_from_string_like_object() {
    BEGIN_TEST;

    {
        SimpleFakeString str;
        fbl::String p;
        p = str;
        EXPECT_STR_EQ(kFakeStringData, p.data());
        EXPECT_EQ(kFakeStringLength, p.length());
    }

    {
        OverloadedFakeString str;
        fbl::String p;
        p = str;
        EXPECT_STR_EQ(kFakeStringData, p.data());
        EXPECT_EQ(kFakeStringLength, p.length());
    }

    {
        EmptyString str;
        fbl::String p("abc");
        p = str;
        EXPECT_STR_EQ("", p.data());
        EXPECT_EQ(0u, p.length());
    }

    END_TEST;
}

} // namespace

BEGIN_TEST_CASE(string_tests)
RUN_TEST(empty_string_test)
RUN_TEST(non_empty_string_test)
RUN_TEST(copy_move_and_assignment_test)
RUN_TEST(set_clear_test)
RUN_TEST(compare_test)
RUN_TEST(concat_test)
RUN_TEST(alloc_checker_test)
RUN_TEST(to_string_piece_test)
RUN_TEST(swap_test)
RUN_TEST(ref_count_test)
RUN_TEST(conversion_from_string_like_object)
RUN_TEST(assignment_from_string_like_object)
END_TEST_CASE(string_tests)
