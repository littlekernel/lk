// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fbl/string_traits.h>

#include <fbl/algorithm.h>
#include <fbl/string.h>
#include <fbl/string_piece.h>
#include <unittest/unittest.h>

namespace {

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

struct EmptyStructBadString {};

struct DataOnlyBadString {
    const char* data();
};

struct LengthOnlyBadString {
    size_t length();
};

struct WrongDataTypeBadString {
    char* data() const;
    size_t length() const;
};

struct WrongLengthTypeBadString {
    const char* data() const;
    int32_t length() const;
};

static_assert(fbl::is_string_like<fbl::String>::value, "ok - string");
static_assert(fbl::is_string_like<fbl::StringPiece>::value, "ok - string piece");
static_assert(fbl::is_string_like<SimpleFakeString>::value, "ok - simple");
static_assert(fbl::is_string_like<OverloadedFakeString>::value, "ok - overloaded");
static_assert(!fbl::is_string_like<decltype(nullptr)>::value, "bad - null");
static_assert(!fbl::is_string_like<int>::value, "bad - int");
static_assert(!fbl::is_string_like<EmptyStructBadString>::value, "bad - empty struct");
static_assert(!fbl::is_string_like<DataOnlyBadString>::value, "bad - data only");
static_assert(!fbl::is_string_like<LengthOnlyBadString>::value, "bad - length only");
static_assert(!fbl::is_string_like<WrongDataTypeBadString>::value, "bad - wrong data type");
static_assert(!fbl::is_string_like<WrongLengthTypeBadString>::value, "bad - wrong length type");

bool string_accessors_test() {
    BEGIN_TEST;

    {
        SimpleFakeString str;
        EXPECT_EQ(kFakeStringData, fbl::GetStringData(str));
        EXPECT_EQ(kFakeStringLength, fbl::GetStringLength(str));
    }

    {
        OverloadedFakeString str;
        EXPECT_EQ(kFakeStringData, fbl::GetStringData(str));
        EXPECT_EQ(kFakeStringLength, fbl::GetStringLength(str));
    }

    END_TEST;
}

} // namespace

BEGIN_TEST_CASE(string_traits_tests)
RUN_TEST(string_accessors_test)
END_TEST_CASE(string_traits_tests)
