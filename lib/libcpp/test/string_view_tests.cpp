//
// Copyright (c) 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

// Tests for lib/libcpp's <string_view>.

#include <lib/unittest.h>
#include <string.h>
#include <string_view>

namespace {

using namespace std::string_view_literals;
using std::string_view;

// construction and basic observers
constexpr string_view kHello = "hello";
static_assert(kHello.size() == 5);
static_assert(kHello.length() == 5);
static_assert(!kHello.empty());
static_assert(string_view().empty());
static_assert(kHello[1] == 'e');
static_assert(kHello.at(4) == 'o');
static_assert(kHello.front() == 'h' && kHello.back() == 'o');
static_assert(*kHello.begin() == 'h');
static_assert(kHello.end() - kHello.begin() == 5);

// literal
static_assert("hi\0there"sv.size() == 8, "embedded NUL preserved by the literal");
static_assert(string_view("hi\0there").size() == 2, "C-string ctor stops at NUL");

// comparisons, including against const char* via implicit conversion
static_assert(kHello == "hello");
static_assert("hello" == kHello);
static_assert(kHello != "world");
static_assert(kHello < "world");
static_assert(kHello > "abc");
static_assert(kHello >= "hello" && kHello <= "hello");
static_assert(kHello.compare("hello") == 0);
static_assert(kHello.compare("hellp") < 0);
static_assert(kHello.compare("hell") > 0);

// substr / remove_prefix / remove_suffix
static_assert(kHello.substr(1, 3) == "ell");
static_assert(kHello.substr(3) == "lo");
static_assert(kHello.substr(5).empty());

// remaining compare/observer forms
static_assert(kHello.compare(1, 3, "ell") == 0);
static_assert(kHello.compare(1, 3, "xyz") != 0);
static_assert(kHello.max_size() > 0);
static_assert(kHello.data() != nullptr);
static_assert(*kHello.cbegin() == 'h');
static_assert(kHello.cend() - kHello.cbegin() == 5);

// starts_with / ends_with
static_assert(kHello.starts_with("he"));
static_assert(kHello.starts_with('h'));
static_assert(!kHello.starts_with("eh"));
static_assert(kHello.ends_with("lo"));
static_assert(kHello.ends_with('o'));
static_assert(!kHello.ends_with("ol"));

// find family
constexpr string_view kAba = "abacabad";
static_assert(kAba.find('b') == 1);
static_assert(kAba.find('z') == string_view::npos);
static_assert(kAba.find("aba") == 0);
static_assert(kAba.find("aba", 1) == 4);
static_assert(kAba.find("") == 0);
static_assert(kAba.rfind('a') == 6);
static_assert(kAba.rfind("aba") == 4);
static_assert(kAba.rfind("aba", 3) == 0);
static_assert(kAba.find_first_of("bc") == 1);
static_assert(kAba.find_first_of("xyz") == string_view::npos);
static_assert(kAba.find_first_of('c', 2) == 3);
static_assert(kAba.find_last_of("bc") == 5);
static_assert(kAba.find_last_of('b') == 5);
static_assert(kAba.find_first_not_of("ab") == 3);
static_assert(kAba.find_first_not_of('a') == 1);
static_assert(kAba.find_last_not_of("ad") == 5);
static_assert(kAba.find_last_not_of('d') == 6);
static_assert(kAba.rfind("aba", 3) == 0);
static_assert(string_view("aaa").find_last_not_of('a') == string_view::npos);

bool sv_runtime() {
    BEGIN_TEST;

    string_view sv("kernel");
    EXPECT_EQ(6U, sv.size(), "");
    EXPECT_TRUE(sv == "kernel", "");

    sv.remove_prefix(3);
    EXPECT_TRUE(sv == "nel", "");
    sv.remove_suffix(1);
    EXPECT_TRUE(sv == "ne", "");

    string_view other("other");
    sv.swap(other);
    EXPECT_TRUE(sv == "other", "");
    EXPECT_TRUE(other == "ne", "");

    // copy into a buffer
    char buf[8] = {};
    string_view src("copyme");
    size_t n = src.copy(buf, sizeof(buf) - 1, 4);
    EXPECT_EQ(2U, n, "");
    EXPECT_EQ(0, strcmp(buf, "me"), "");

    // iteration
    int count = 0;
    for (char c : string_view("abc")) {
        (void)c;
        count++;
    }
    EXPECT_EQ(3, count, "");

    // views over non-terminated ranges
    const char raw[] = {'x', 'y', 'z'};
    string_view rv(raw, sizeof(raw));
    EXPECT_EQ(3U, rv.size(), "");
    EXPECT_TRUE(rv == "xyz", "");

    END_TEST;
}

BEGIN_TEST_CASE(libcpp_string_view_tests)
RUN_TEST(sv_runtime)
END_TEST_CASE(libcpp_string_view_tests)

} // namespace
