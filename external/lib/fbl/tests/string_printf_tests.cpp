// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fbl/string_printf.h>

#include <stdarg.h>

#include <fbl/array.h>
#include <unittest/unittest.h>

namespace {

// Note: |runnable| can't be a reference since that'd make the behavior of
// |va_start()| undefined.
template <typename Runnable>
fbl::String VAListHelper(Runnable runnable, ...) {
    va_list ap;
    va_start(ap, runnable);
    fbl::String rv = runnable(ap);
    va_end(ap);
    return rv;
}

bool string_printf_basic_test() {
    BEGIN_TEST;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-zero-length"
    EXPECT_STR_EQ("", fbl::StringPrintf("").c_str());
#pragma GCC diagnostic pop
    EXPECT_STR_EQ("hello", fbl::StringPrintf("hello").c_str());
    EXPECT_STR_EQ("hello-123", fbl::StringPrintf("hello%d", -123).c_str());
    EXPECT_STR_EQ("hello0123FACE", fbl::StringPrintf("%s%04d%X", "hello", 123, 0xfaceU).c_str());

    END_TEST;
}

bool string_vprintf_basic_test() {
    BEGIN_TEST;

    EXPECT_STR_EQ("",
                  VAListHelper([](va_list ap) -> fbl::String {
                      return fbl::StringVPrintf("", ap);
                  })
                       .c_str());
    EXPECT_STR_EQ("hello",
                  VAListHelper([](va_list ap) -> fbl::String {
                      return fbl::StringVPrintf("hello", ap);
                  })
                       .c_str());
    EXPECT_STR_EQ("hello-123",
                  VAListHelper(
                      [](va_list ap) -> fbl::String {
                          return fbl::StringVPrintf("hello%d", ap);
                      },
                      -123)
                      .c_str());
    EXPECT_STR_EQ("hello0123FACE",
                  VAListHelper(
                      [](va_list ap) -> fbl::String {
                          return fbl::StringVPrintf("%s%04d%X", ap);
                      },
                      "hello", 123, 0xfaceU)
                      .c_str());

    END_TEST;
}

// Generally, we assume that everything forwards to |fbl::StringVPrintf()|, so
// testing |fbl::StringPrintf()| more carefully suffices.

bool string_printf_boundary_test() {
    BEGIN_TEST;

    // Note: The size of strings generated should cover the boundary cases in the
    // constant |kStackBufferSize| in |StringVPrintf()|.
    for (size_t i = 800; i < 1200; i++) {
        fbl::String stuff(i, 'x');
        fbl::String format = fbl::String::Concat({stuff, "%d", "%s", " world"});
        EXPECT_STR_EQ(fbl::String::Concat({stuff, "123", "hello world"}).c_str(),
                      fbl::StringPrintf(format.c_str(), 123, "hello").c_str());
    }

    END_TEST;
}

bool string_printf_very_big_string_test() {
    BEGIN_TEST;

    // 4 megabytes of exes (we'll generate 5 times this).
    fbl::String stuff(4u << 20u, 'x');
    fbl::String format = fbl::String::Concat({"%s", stuff, "%s", stuff, "%s"});
    EXPECT_STR_EQ(fbl::String::Concat({stuff, stuff, stuff, stuff, stuff}).c_str(),
                  fbl::StringPrintf(format.c_str(), stuff.c_str(), stuff.c_str(),
                                    stuff.c_str())
                      .c_str());

    END_TEST;
}

} // namespace

BEGIN_TEST_CASE(string_printf_tests)
RUN_TEST(string_printf_basic_test)
RUN_TEST(string_vprintf_basic_test)
RUN_TEST(string_printf_boundary_test)
RUN_TEST(string_printf_very_big_string_test)
END_TEST_CASE(string_printf_tests)
