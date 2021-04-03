// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fbl/string_buffer.h>

#include <unittest/unittest.h>

#define EXPECT_DATA_AND_LENGTH(expected, actual)      \
    do {                                              \
        EXPECT_STR_EQ(expected, actual.data());      \
        EXPECT_EQ(strlen(expected), actual.length()); \
    } while (false)

namespace {

// Note: |runnable| can't be a reference since that'd make the behavior of
// |va_start()| undefined.
template <typename Runnable>
void VAListHelper(Runnable runnable, ...) {
    va_list ap;
    va_start(ap, runnable);
    runnable(ap);
    va_end(ap);
}

bool capacity_test() {
    BEGIN_TEST;

    {
        fbl::StringBuffer<0u> buf;
        EXPECT_EQ(0u, buf.capacity());
    }

    {
        fbl::StringBuffer<100u> buf;
        EXPECT_EQ(100u, buf.capacity());
    }

    END_TEST;
}

bool empty_string_test() {
    BEGIN_TEST;

    {
        fbl::StringBuffer<0u> empty;

        EXPECT_STR_EQ("", empty.data());
        EXPECT_STR_EQ("", empty.c_str());

        EXPECT_EQ(0u, empty.length());
        EXPECT_EQ(0u, empty.size());
        EXPECT_TRUE(empty.empty());
        EXPECT_EQ(0u, empty.capacity());

        EXPECT_STR_EQ("", empty.begin());
        EXPECT_EQ(0u, empty.end() - empty.begin());
        EXPECT_STR_EQ("", empty.cbegin());
        EXPECT_EQ(0u, empty.cend() - empty.cbegin());

        EXPECT_EQ(0, empty[0u]);
    }

    {
        fbl::StringBuffer<16u> empty;

        EXPECT_STR_EQ("", empty.data());
        EXPECT_STR_EQ("", empty.c_str());

        EXPECT_EQ(0u, empty.length());
        EXPECT_EQ(0u, empty.size());
        EXPECT_TRUE(empty.empty());
        EXPECT_EQ(16u, empty.capacity());

        EXPECT_STR_EQ("", empty.begin());
        EXPECT_EQ(0u, empty.end() - empty.begin());
        EXPECT_STR_EQ("", empty.cbegin());
        EXPECT_EQ(0u, empty.cend() - empty.cbegin());

        EXPECT_EQ(0, empty[0u]);
    }

    END_TEST;
}

bool append_test() {
    BEGIN_TEST;

    {
        fbl::StringBuffer<16u> str;
        str.Append('a')
            .Append('b')
            .Append("cd")
            .Append("efghi", 3u)
            .Append(fbl::StringPiece("hijkl", 3u))
            .Append(fbl::String("klmnopqrstuvwxyz"))
            .Append('z') // these will be truncated away
            .Append("zz")
            .Append("zzzzzz", 3u)
            .Append(fbl::StringPiece("zzzzz", 3u))
            .Append(fbl::String("zzzzz"));

        EXPECT_STR_EQ("abcdefghijklmnop", str.data());
        EXPECT_STR_EQ("abcdefghijklmnop", str.c_str());

        EXPECT_EQ(16u, str.length());
        EXPECT_EQ(16u, str.size());
        EXPECT_FALSE(str.empty());
        EXPECT_EQ(16u, str.capacity());

        EXPECT_STR_EQ("abcdefghijklmnop", str.begin());
        EXPECT_EQ(16u, str.end() - str.begin());
        EXPECT_STR_EQ("abcdefghijklmnop", str.cbegin());
        EXPECT_EQ(16u, str.cend() - str.cbegin());

        EXPECT_EQ('b', str[1u]);
    }

    {
        fbl::StringBuffer<3u> str;
        str.Append('a');
        EXPECT_DATA_AND_LENGTH("a", str);
        str.Append('b');
        EXPECT_DATA_AND_LENGTH("ab", str);
        str.Append('c');
        EXPECT_DATA_AND_LENGTH("abc", str);
        str.Append('d');
        EXPECT_DATA_AND_LENGTH("abc", str);
    }

    {
        fbl::StringBuffer<3u> str;
        str.Append("ab");
        EXPECT_DATA_AND_LENGTH("ab", str);
        str.Append("");
        EXPECT_DATA_AND_LENGTH("ab", str);
        str.Append("cdefg");
        EXPECT_DATA_AND_LENGTH("abc", str);
    }

    {
        fbl::StringBuffer<3u> str;
        str.Append("abcdef", 2u);
        EXPECT_DATA_AND_LENGTH("ab", str);
        str.Append("zzzz", 0u);
        EXPECT_DATA_AND_LENGTH("ab", str);
        str.Append("cdefghijk", 5u);
        EXPECT_DATA_AND_LENGTH("abc", str);
    }

    {
        fbl::StringBuffer<3u> str;
        str.Append(fbl::StringPiece("abcdef", 2u));
        EXPECT_DATA_AND_LENGTH("ab", str);
        str.Append(fbl::StringPiece("zzzz", 0u));
        EXPECT_DATA_AND_LENGTH("ab", str);
        str.Append(fbl::StringPiece("cdefghijk", 5u));
        EXPECT_DATA_AND_LENGTH("abc", str);
    }

    {
        fbl::StringBuffer<3u> str;
        str.Append(fbl::String("ab"));
        EXPECT_DATA_AND_LENGTH("ab", str);
        str.Append(fbl::String());
        EXPECT_DATA_AND_LENGTH("ab", str);
        str.Append(fbl::String("cdefg"));
        EXPECT_DATA_AND_LENGTH("abc", str);
    }

    END_TEST;
}

bool append_printf_test() {
    BEGIN_TEST;

    {
        fbl::StringBuffer<12u> str;
        str.AppendPrintf("abc");
        EXPECT_DATA_AND_LENGTH("abc", str);
        str.AppendPrintf("%d,%s", 20, "de").Append('f');
        EXPECT_DATA_AND_LENGTH("abc20,def", str);
        str.AppendPrintf("%d", 123456789);
        EXPECT_DATA_AND_LENGTH("abc20,def123", str);
    }

    {
        fbl::StringBuffer<12u> str;
        VAListHelper([&str](va_list ap) { str.AppendVPrintf("abc", ap); });
        EXPECT_DATA_AND_LENGTH("abc", str);
        VAListHelper([&str](va_list ap) { str.AppendVPrintf("%d,%s", ap).Append('f'); },
                     20, "de");
        EXPECT_DATA_AND_LENGTH("abc20,def", str);
        VAListHelper([&str](va_list ap) { str.AppendVPrintf("%d", ap); }, 123456789);
        EXPECT_DATA_AND_LENGTH("abc20,def123", str);
    }

    END_TEST;
}

bool modify_test() {
    BEGIN_TEST;

    {
        fbl::StringBuffer<16u> str;
        str.Append("abcdef");

        EXPECT_EQ('c', str[2u]);
        str[2u] = 'x';
        EXPECT_EQ('x', str[2u]);
        EXPECT_DATA_AND_LENGTH("abxdef", str);

        memcpy(str.data(), "yyyy", 4u);
        EXPECT_DATA_AND_LENGTH("yyyyef", str);
    }

    END_TEST;
}

bool resize_test() {
    BEGIN_TEST;

    {
        fbl::StringBuffer<16u> str;

        str.Resize(4u, 'x');
        EXPECT_STR_EQ("xxxx", str.data());
        EXPECT_EQ(4u, str.length());

        str.Resize(8u, 'y');
        EXPECT_STR_EQ("xxxxyyyy", str.data());
        EXPECT_EQ(8u, str.length());

        str.Resize(16u);
        EXPECT_STR_EQ("xxxxyyyy", str.data());
        EXPECT_EQ(0, memcmp("xxxxyyyy\0\0\0\0\0\0\0\0\0", str.data(), str.length() + 1));
        EXPECT_EQ(16u, str.length());

        str.Resize(0u);
        EXPECT_STR_EQ("", str.data());
        EXPECT_EQ(0u, str.length());
    }

    END_TEST;
}

bool clear_test() {
    BEGIN_TEST;

    {
        fbl::StringBuffer<16u> str;
        str.Append("abcdef");

        str.Clear();
        EXPECT_STR_EQ("", str.data());
        EXPECT_EQ(0u, str.length());
    }

    END_TEST;
}

bool to_string_test() {
    BEGIN_TEST;

    {
        fbl::StringBuffer<16u> buf;
        buf.Append("abcdef");

        fbl::String str = buf.ToString();
        EXPECT_TRUE(str == "abcdef");
    }

    END_TEST;
}

bool to_string_piece_test() {
    BEGIN_TEST;

    {
        fbl::StringBuffer<16u> buf;
        buf.Append("abcdef");

        fbl::StringPiece piece = buf.ToStringPiece();
        EXPECT_EQ(buf.data(), piece.data());
        EXPECT_EQ(buf.length(), piece.length());
    }

    END_TEST;
}

} // namespace

BEGIN_TEST_CASE(string_buffer_tests)
RUN_TEST(capacity_test)
RUN_TEST(empty_string_test)
RUN_TEST(append_test)
RUN_TEST(append_printf_test)
RUN_TEST(modify_test)
RUN_TEST(resize_test)
RUN_TEST(clear_test)
RUN_TEST(to_string_test)
RUN_TEST(to_string_piece_test)
END_TEST_CASE(string_buffer_tests)
