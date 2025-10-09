// Copyright (c) 2008-2014 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#if !WITH_NO_FP

#include <lib/unittest.h>
#include <stdio.h>
#include <string.h>

#include "printf_tests_float_vec.h"

namespace {

bool test_printf(const char* expected, const char* format, ...) {
  char buf[100];
  va_list args;
  va_start(args, format);
  int length = vsnprintf_float(buf, sizeof(buf), format, args);
  va_end(args);

  if (length < 0 || length >= static_cast<int>(sizeof(buf))) {
    printf("vsnprintf() returned %d\n", length);
    return false;
  }

  bool success = true;
  if (buf[length] != '\0') {
    printf("missing string terminator\n");
    success = false;
  }
  if (length != static_cast<int>(strlen(expected)) || memcmp(buf, expected, length + 1) != 0) {
    printf("expected: \"%s\" (length %zu)\n", expected, strlen(expected));
    printf("but got:  \"%s\" (length %zu) with return value %d)\n", buf, strlen(buf), length);
    success = false;
  }
  return success;
}

bool basic() {
  BEGIN_TEST;

  for (const auto &vec : float_test_vec) {
    EXPECT_TRUE(test_printf(vec.expected_f, "%f", vec.d));
    EXPECT_TRUE(test_printf(vec.expected_F, "%F", vec.d));
  }
  END_TEST;
}

bool hex() {
  BEGIN_TEST;

  for (const auto &vec : float_test_vec) {
    EXPECT_TRUE(test_printf(vec.expected_a, "%a", vec.d));
    EXPECT_TRUE(test_printf(vec.expected_A, "%A", vec.d));
  }
  END_TEST;
}

BEGIN_TEST_CASE(printf_tests_float)
RUN_TEST(basic);
RUN_TEST(hex);
END_TEST_CASE(printf_tests_float)

}  // namespace

#endif // !WITH_NO_FP
