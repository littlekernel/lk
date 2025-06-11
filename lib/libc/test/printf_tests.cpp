// Copyright 2016 The Fuchsia Authors
// Copyright (c) 2008-2014 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#include <lib/unittest.h>
#include <stdio.h>
#include <string.h>

namespace {

template <typename IntegralType>
constexpr bool kIs64Bit = sizeof(IntegralType) == sizeof(uint64_t);

template <typename IntegralType>
constexpr bool kIs32Bit = sizeof(IntegralType) == sizeof(uint32_t);

// Checks that vsnprintf() gives the expected string as output.
bool test_printf(const char* expected, const char* format, ...) {
  char buf[100];
  va_list args;
  va_start(args, format);
  int length = vsnprintf(buf, sizeof(buf), format, args);
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

bool numbers() {
  BEGIN_TEST;

  EXPECT_TRUE(test_printf("int8:  -12 0 -2", "int8:  %hhd %hhd %hhd", -12, 0, 254));
  EXPECT_TRUE(test_printf("uint8: 244 0 254", "uint8: %hhu %hhu %hhu", -12, 0, 254));
  EXPECT_TRUE(test_printf("int16: -1234 0 1234", "int16: %hd %hd %hd", -1234, 0, 1234));
  EXPECT_TRUE(test_printf("uint16:64302 0 1234", "uint16:%hu %hu %hu", -1234, 0, 1234));
  EXPECT_TRUE(
      test_printf("int:   -12345678 0 12345678", "int:   %d %d %d", -12345678, 0, 12345678));
  static_assert(-12345678U == 4282621618, "");
  EXPECT_TRUE(
      test_printf("uint:  4282621618 0 12345678", "uint:  %u %u %u", -12345678, 0, 12345678));
  EXPECT_TRUE(
      test_printf("long:  -12345678 0 12345678", "long:  %ld %ld %ld", -12345678L, 0L, 12345678L));

  if (kIs64Bit<unsigned long>) {
    EXPECT_TRUE(test_printf("ulong: 18446744073697205938 0 12345678", "ulong: %lu %lu %lu",
                            -12345678UL, 0UL, 12345678UL));
  } else {
    EXPECT_TRUE(test_printf("ulong: 4282621618 0 12345678", "ulong: %lu %lu %lu", -12345678UL, 0UL,
                            12345678UL));
  }

  EXPECT_TRUE(test_printf("longlong: -12345678 0 12345678", "longlong: %lli %lli %lli", -12345678LL,
                          0LL, 12345678LL));
  EXPECT_TRUE(test_printf("ulonglong: 18446744073697205938 0 12345678", "ulonglong: %llu %llu %llu",
                          -12345678LL, 0LL, 12345678LL));
  EXPECT_TRUE(test_printf("ssize_t: -12345678 0 12345678", "ssize_t: %zd %zd %zd",
                          (ssize_t)-12345678, (ssize_t)0, (ssize_t)12345678));

  if (kIs64Bit<size_t>) {
    EXPECT_TRUE(test_printf("usize_t: 18446744073697205938 0 12345678", "usize_t: %zu %zu %zu",
                            (size_t)-12345678, (size_t)0, (size_t)12345678));
  } else {
    EXPECT_TRUE(test_printf("usize_t: 4282621618 0 12345678", "usize_t: %zu %zu %zu",
                            (size_t)-12345678, (size_t)0, (size_t)12345678));
  }

  EXPECT_TRUE(test_printf("intmax_t: -12345678 0 12345678", "intmax_t: %jd %jd %jd",
                          (intmax_t)-12345678, (intmax_t)0, (intmax_t)12345678));
  EXPECT_TRUE(test_printf("uintmax_t: 18446744073697205938 0 12345678", "uintmax_t: %ju %ju %ju",
                          (uintmax_t)-12345678, (uintmax_t)0, (uintmax_t)12345678));
  EXPECT_TRUE(test_printf("ptrdiff_t: -12345678 0 12345678", "ptrdiff_t: %td %td %td",
                          (ptrdiff_t)-12345678, (ptrdiff_t)0, (ptrdiff_t)12345678));

  if (kIs64Bit<ptrdiff_t>) {
    EXPECT_TRUE(test_printf("ptrdiff_t (u): 18446744073697205938 0 12345678",
                            "ptrdiff_t (u): %tu %tu %tu", (ptrdiff_t)-12345678, (ptrdiff_t)0,
                            (ptrdiff_t)12345678));
  } else {
    EXPECT_TRUE(test_printf("ptrdiff_t (u): 4282621618 0 12345678", "ptrdiff_t (u): %tu %tu %tu",
                            (ptrdiff_t)-12345678, (ptrdiff_t)0, (ptrdiff_t)12345678));
  }

  END_TEST;
}

bool hex() {
  BEGIN_TEST;

  EXPECT_TRUE(test_printf("uint8: f4 0 fe", "uint8: %hhx %hhx %hhx", -12, 0, 254));
  EXPECT_TRUE(test_printf("uint16:fb2e 0 4d2", "uint16:%hx %hx %hx", -1234, 0, 1234));
  EXPECT_TRUE(test_printf("uint:  ff439eb2 0 bc614e", "uint:  %x %x %x", -12345678, 0, 12345678));
  if (kIs64Bit<void*>) {
    EXPECT_TRUE(test_printf("ptr: 0xdeadbeefcabba6e5", "ptr: %p", (void*)0xdeadbeefcabba6e5));
  } else if (kIs32Bit<void*>) {
    EXPECT_TRUE(test_printf("ptr: 0xdeadbeef", "ptr: %p", (void*)0xdeadbeef));
  } else {
    EXPECT_TRUE(false, "Unexpected pointer size");
  }

  if (kIs64Bit<unsigned long>) {
    EXPECT_TRUE(test_printf("ulong: ffffffffff439eb2 0 bc614e", "ulong: %lx %lx %lx", -12345678UL,
                            0UL, 12345678UL));
  } else {
    EXPECT_TRUE(test_printf("ulong: ff439eb2 0 bc614e", "ulong: %lx %lx %lx", -12345678UL, 0UL,
                            12345678UL));
  }

  EXPECT_TRUE(test_printf("ulong: FF439EB2 0 BC614E", "ulong: %X %X %X", -12345678, 0, 12345678));
  EXPECT_TRUE(test_printf("ulonglong: ffffffffff439eb2 0 bc614e", "ulonglong: %llx %llx %llx",
                          -12345678LL, 0LL, 12345678LL));

  if (kIs64Bit<size_t>) {
    EXPECT_TRUE(test_printf("usize_t: ffffffffff439eb2 0 bc614e", "usize_t: %zx %zx %zx",
                            (size_t)-12345678, (size_t)0, (size_t)12345678));
  } else {
    EXPECT_TRUE(test_printf("usize_t: ff439eb2 0 bc614e", "usize_t: %zx %zx %zx", (size_t)-12345678,
                            (size_t)0, (size_t)12345678));
  }

  END_TEST;
}

bool alt_and_sign() {
  BEGIN_TEST;

  EXPECT_TRUE(test_printf("uint: 0xabcdef 0XABCDEF", "uint: %#x %#X", 0xabcdef, 0xabcdef));
  EXPECT_TRUE(test_printf("int: +12345678 -12345678", "int: %+d %+d", 12345678, -12345678));
  EXPECT_TRUE(test_printf("int:  12345678 +12345678", "int: % d %+d", 12345678, 12345678));

  // Test if zero-padding works fine with hex's alt (%#x)
  EXPECT_TRUE(test_printf("uint: 0x02 0X02", "uint: %#04x %#04X", 2, 2));
  EXPECT_TRUE(test_printf("uint: 0x0000002 0X0000002", "uint: %#09x %#09X", 2, 2));
  EXPECT_TRUE(test_printf("uint: 0x2 0X2", "uint: %#02x %#02X", 2, 2));

  END_TEST;
}

bool formatting() {
  BEGIN_TEST;

  EXPECT_TRUE(test_printf("int: a12345678a", "int: a%8da", 12345678));
  EXPECT_TRUE(test_printf("int: a 12345678a", "int: a%9da", 12345678));
  EXPECT_TRUE(test_printf("int: a12345678 a", "int: a%-9da", 12345678));
  EXPECT_TRUE(test_printf("int: a  12345678a", "int: a%10da", 12345678));
  EXPECT_TRUE(test_printf("int: a12345678  a", "int: a%-10da", 12345678));
  EXPECT_TRUE(test_printf("int: a012345678a", "int: a%09da", 12345678));
  EXPECT_TRUE(test_printf("int: a0012345678a", "int: a%010da", 12345678));
  EXPECT_TRUE(test_printf("int: a12345678a", "int: a%6da", 12345678));

  EXPECT_TRUE(test_printf("aba", "a%1sa", "b"));
  EXPECT_TRUE(test_printf("a        ba", "a%9sa", "b"));
  EXPECT_TRUE(test_printf("ab        a", "a%-9sa", "b"));
  EXPECT_TRUE(test_printf("athisisatesta", "a%5sa", "thisisatest"));

  EXPECT_TRUE(test_printf("-02", "%03d", -2));
  EXPECT_TRUE(test_printf("-02", "%0+3d", -2));
  EXPECT_TRUE(test_printf("+02", "%0+3d", 2));
  EXPECT_TRUE(test_printf(" +2", "%+3d", 2));
  EXPECT_TRUE(test_printf("-2000", "% 3d", -2000));
  EXPECT_TRUE(test_printf(" 2000", "% 3d", 2000));
  EXPECT_TRUE(test_printf("+2000", "%+3d", 2000));
  EXPECT_TRUE(test_printf("      test", "%10s", "test"));
  EXPECT_TRUE(test_printf("      test", "%010s", "test"));
  EXPECT_TRUE(test_printf("test      ", "%-10s", "test"));
  EXPECT_TRUE(test_printf("test      ", "%-010s", "test"));

  END_TEST;
}

bool printf_field_width_and_precision_test() {
  BEGIN_TEST;

  const char input[] = "0123456789";

  // Hard-coded width; no precision.
  EXPECT_TRUE(test_printf("'0123456789'", "'%0s'", input));
  EXPECT_TRUE(test_printf("'0123456789'", "'%1s'", input));
  EXPECT_TRUE(test_printf("'0123456789'", "'%2s'", input));
  EXPECT_TRUE(test_printf("'0123456789'", "'%5s'", input));
  EXPECT_TRUE(test_printf("'0123456789'", "'%10s'", input));
  EXPECT_TRUE(test_printf("' 0123456789'", "'%11s'", input));
  EXPECT_TRUE(test_printf("'  0123456789'", "'%12s'", input));
  EXPECT_TRUE(test_printf("'     0123456789'", "'%15s'", input));
  EXPECT_TRUE(test_printf("'          0123456789'", "'%20s'", input));

  EXPECT_TRUE(test_printf("'0123456789'", "'%-0s'", input));
  EXPECT_TRUE(test_printf("'0123456789'", "'%-1s'", input));
  EXPECT_TRUE(test_printf("'0123456789'", "'%-2s'", input));
  EXPECT_TRUE(test_printf("'0123456789'", "'%-5s'", input));
  EXPECT_TRUE(test_printf("'0123456789'", "'%-10s'", input));
  EXPECT_TRUE(test_printf("'0123456789 '", "'%-11s'", input));
  EXPECT_TRUE(test_printf("'0123456789  '", "'%-12s'", input));
  EXPECT_TRUE(test_printf("'0123456789     '", "'%-15s'", input));
  EXPECT_TRUE(test_printf("'0123456789          '", "'%-20s'", input));

  // variable width; no precision.
  EXPECT_TRUE(test_printf("'0123456789'", "'%*s'", 0, input));
  EXPECT_TRUE(test_printf("'0123456789'", "'%*s'", 1, input));
  EXPECT_TRUE(test_printf("'0123456789'", "'%*s'", 2, input));
  EXPECT_TRUE(test_printf("'0123456789'", "'%*s'", 5, input));
  EXPECT_TRUE(test_printf("'0123456789'", "'%*s'", 10, input));
  EXPECT_TRUE(test_printf("' 0123456789'", "'%*s'", 11, input));
  EXPECT_TRUE(test_printf("'  0123456789'", "'%*s'", 12, input));
  EXPECT_TRUE(test_printf("'     0123456789'", "'%*s'", 15, input));
  EXPECT_TRUE(test_printf("'          0123456789'", "'%*s'", 20, input));

  EXPECT_TRUE(test_printf("'0123456789'", "'%*s'", -1, input));
  EXPECT_TRUE(test_printf("'0123456789'", "'%*s'", -2, input));
  EXPECT_TRUE(test_printf("'0123456789'", "'%*s'", -5, input));
  EXPECT_TRUE(test_printf("'0123456789'", "'%*s'", -10, input));
  EXPECT_TRUE(test_printf("'0123456789 '", "'%*s'", -11, input));
  EXPECT_TRUE(test_printf("'0123456789  '", "'%*s'", -12, input));
  EXPECT_TRUE(test_printf("'0123456789     '", "'%*s'", -15, input));
  EXPECT_TRUE(test_printf("'0123456789          '", "'%*s'", -20, input));

  // No width; hard-coded precision.
  EXPECT_TRUE(test_printf("", "%.", input));
  EXPECT_TRUE(test_printf("", "%.s", input));
  EXPECT_TRUE(test_printf("''", "'%.0s'", input));
  EXPECT_TRUE(test_printf("'0'", "'%.1s'", input));
  EXPECT_TRUE(test_printf("'01'", "'%.2s'", input));
  EXPECT_TRUE(test_printf("'012'", "'%.3s'", input));
  EXPECT_TRUE(test_printf("'0123'", "'%.4s'", input));
  EXPECT_TRUE(test_printf("'01234'", "'%.5s'", input));
  EXPECT_TRUE(test_printf("'012345'", "'%.6s'", input));
  EXPECT_TRUE(test_printf("'0123456'", "'%.7s'", input));
  EXPECT_TRUE(test_printf("'01234567'", "'%.8s'", input));
  EXPECT_TRUE(test_printf("'012345678'", "'%.9s'", input));
  EXPECT_TRUE(test_printf("'0123456789'", "'%.10s'", input));

  // No width; variable precision.
  EXPECT_TRUE(test_printf("'0'", "'%.*s'", 1, input));
  EXPECT_TRUE(test_printf("'01'", "'%.*s'", 2, input));
  EXPECT_TRUE(test_printf("'012'", "'%.*s'", 3, input));
  EXPECT_TRUE(test_printf("'0123'", "'%.*s'", 4, input));
  EXPECT_TRUE(test_printf("'01234'", "'%.*s'", 5, input));
  EXPECT_TRUE(test_printf("'012345'", "'%.*s'", 6, input));
  EXPECT_TRUE(test_printf("'0123456'", "'%.*s'", 7, input));
  EXPECT_TRUE(test_printf("'01234567'", "'%.*s'", 8, input));
  EXPECT_TRUE(test_printf("'012345678'", "'%.*s'", 9, input));
  EXPECT_TRUE(test_printf("'0123456789'", "'%.*s'", 10, input));

  // Hard-coded width; hard-coded precision.
  EXPECT_TRUE(test_printf("''", "'%0.0s'", input));
  EXPECT_TRUE(test_printf("'0'", "'%0.1s'", input));
  EXPECT_TRUE(test_printf("'01'", "'%0.2s'", input));
  EXPECT_TRUE(test_printf("'01234'", "'%0.5s'", input));
  EXPECT_TRUE(test_printf("'0123456789'", "'%0.10s'", input));

  EXPECT_TRUE(test_printf("'     '", "'%5.0s'", input));
  EXPECT_TRUE(test_printf("'    0'", "'%5.1s'", input));
  EXPECT_TRUE(test_printf("'   01'", "'%5.2s'", input));
  EXPECT_TRUE(test_printf("'01234'", "'%5.5s'", input));
  EXPECT_TRUE(test_printf("'0123456789'", "'%5.10s'", input));

  EXPECT_TRUE(test_printf("'          '", "'%10.0s'", input));
  EXPECT_TRUE(test_printf("'         0'", "'%10.1s'", input));
  EXPECT_TRUE(test_printf("'        01'", "'%10.2s'", input));
  EXPECT_TRUE(test_printf("'     01234'", "'%10.5s'", input));
  EXPECT_TRUE(test_printf("'0123456789'", "'%10.10s'", input));

  EXPECT_TRUE(test_printf("'               '", "'%15.0s'", input));
  EXPECT_TRUE(test_printf("'              0'", "'%15.1s'", input));
  EXPECT_TRUE(test_printf("'             01'", "'%15.2s'", input));
  EXPECT_TRUE(test_printf("'          01234'", "'%15.5s'", input));
  EXPECT_TRUE(test_printf("'     0123456789'", "'%15.10s'", input));

  EXPECT_TRUE(test_printf("'     '", "'%-5.0s'", input));
  EXPECT_TRUE(test_printf("'0    '", "'%-5.1s'", input));
  EXPECT_TRUE(test_printf("'01   '", "'%-5.2s'", input));
  EXPECT_TRUE(test_printf("'01234'", "'%-5.5s'", input));
  EXPECT_TRUE(test_printf("'0123456789'", "'%-5.10s'", input));

  EXPECT_TRUE(test_printf("'          '", "'%-10.0s'", input));
  EXPECT_TRUE(test_printf("'0         '", "'%-10.1s'", input));
  EXPECT_TRUE(test_printf("'01        '", "'%-10.2s'", input));
  EXPECT_TRUE(test_printf("'01234     '", "'%-10.5s'", input));
  EXPECT_TRUE(test_printf("'0123456789'", "'%-10.10s'", input));

  EXPECT_TRUE(test_printf("'               '", "'%-15.0s'", input));
  EXPECT_TRUE(test_printf("'0              '", "'%-15.1s'", input));
  EXPECT_TRUE(test_printf("'01             '", "'%-15.2s'", input));
  EXPECT_TRUE(test_printf("'01234          '", "'%-15.5s'", input));
  EXPECT_TRUE(test_printf("'0123456789     '", "'%-15.10s'", input));

  // Variable width; hard-coded precision.
  EXPECT_TRUE(test_printf("''", "'%*.0s'", 0, input));
  EXPECT_TRUE(test_printf("'0'", "'%*.1s'", 0, input));
  EXPECT_TRUE(test_printf("'01'", "'%*.2s'", 0, input));
  EXPECT_TRUE(test_printf("'01234'", "'%*.5s'", 0, input));
  EXPECT_TRUE(test_printf("'0123456789'", "'%*.10s'", 0, input));

  EXPECT_TRUE(test_printf("'     '", "'%*.0s'", 5, input));
  EXPECT_TRUE(test_printf("'    0'", "'%*.1s'", 5, input));
  EXPECT_TRUE(test_printf("'   01'", "'%*.2s'", 5, input));
  EXPECT_TRUE(test_printf("'01234'", "'%*.5s'", 5, input));
  EXPECT_TRUE(test_printf("'0123456789'", "'%*.10s'", 5, input));

  EXPECT_TRUE(test_printf("'          '", "'%*.0s'", 10, input));
  EXPECT_TRUE(test_printf("'         0'", "'%*.1s'", 10, input));
  EXPECT_TRUE(test_printf("'        01'", "'%*.2s'", 10, input));
  EXPECT_TRUE(test_printf("'     01234'", "'%*.5s'", 10, input));
  EXPECT_TRUE(test_printf("'0123456789'", "'%*.10s'", 10, input));

  EXPECT_TRUE(test_printf("'               '", "'%*.0s'", 15, input));
  EXPECT_TRUE(test_printf("'              0'", "'%*.1s'", 15, input));
  EXPECT_TRUE(test_printf("'             01'", "'%*.2s'", 15, input));
  EXPECT_TRUE(test_printf("'          01234'", "'%*.5s'", 15, input));
  EXPECT_TRUE(test_printf("'     0123456789'", "'%*.10s'", 15, input));

  EXPECT_TRUE(test_printf("'     '", "'%*.0s'", -5, input));
  EXPECT_TRUE(test_printf("'0    '", "'%*.1s'", -5, input));
  EXPECT_TRUE(test_printf("'01   '", "'%*.2s'", -5, input));
  EXPECT_TRUE(test_printf("'01234'", "'%*.5s'", -5, input));
  EXPECT_TRUE(test_printf("'0123456789'", "'%*.10s'", -5, input));

  EXPECT_TRUE(test_printf("'          '", "'%*.0s'", -10, input));
  EXPECT_TRUE(test_printf("'0         '", "'%*.1s'", -10, input));
  EXPECT_TRUE(test_printf("'01        '", "'%*.2s'", -10, input));
  EXPECT_TRUE(test_printf("'01234     '", "'%*.5s'", -10, input));
  EXPECT_TRUE(test_printf("'0123456789'", "'%*.10s'", -10, input));

  EXPECT_TRUE(test_printf("'               '", "'%*.0s'", -15, input));
  EXPECT_TRUE(test_printf("'0              '", "'%*.1s'", -15, input));
  EXPECT_TRUE(test_printf("'01             '", "'%*.2s'", -15, input));
  EXPECT_TRUE(test_printf("'01234          '", "'%*.5s'", -15, input));
  EXPECT_TRUE(test_printf("'0123456789     '", "'%*.10s'", -15, input));

  // Hard-coded width; variable precision.
  EXPECT_TRUE(test_printf("''", "'%0.*s'", 0, input));
  EXPECT_TRUE(test_printf("'0'", "'%0.*s'", 1, input));
  EXPECT_TRUE(test_printf("'01'", "'%0.*s'", 2, input));
  EXPECT_TRUE(test_printf("'01234'", "'%0.*s'", 5, input));
  EXPECT_TRUE(test_printf("'0123456789'", "'%0.*s'", 10, input));

  EXPECT_TRUE(test_printf("'     '", "'%5.*s'", 0, input));
  EXPECT_TRUE(test_printf("'    0'", "'%5.*s'", 1, input));
  EXPECT_TRUE(test_printf("'   01'", "'%5.*s'", 2, input));
  EXPECT_TRUE(test_printf("'01234'", "'%5.*s'", 5, input));
  EXPECT_TRUE(test_printf("'0123456789'", "'%5.*s'", 10, input));

  EXPECT_TRUE(test_printf("'          '", "'%10.*s'", 0, input));
  EXPECT_TRUE(test_printf("'         0'", "'%10.*s'", 1, input));
  EXPECT_TRUE(test_printf("'        01'", "'%10.*s'", 2, input));
  EXPECT_TRUE(test_printf("'     01234'", "'%10.*s'", 5, input));
  EXPECT_TRUE(test_printf("'0123456789'", "'%10.*s'", 10, input));

  EXPECT_TRUE(test_printf("'               '", "'%15.*s'", 0, input));
  EXPECT_TRUE(test_printf("'              0'", "'%15.*s'", 1, input));
  EXPECT_TRUE(test_printf("'             01'", "'%15.*s'", 2, input));
  EXPECT_TRUE(test_printf("'          01234'", "'%15.*s'", 5, input));
  EXPECT_TRUE(test_printf("'     0123456789'", "'%15.*s'", 10, input));

  EXPECT_TRUE(test_printf("'     '", "'%-5.*s'", 0, input));
  EXPECT_TRUE(test_printf("'0    '", "'%-5.*s'", 1, input));
  EXPECT_TRUE(test_printf("'01   '", "'%-5.*s'", 2, input));
  EXPECT_TRUE(test_printf("'01234'", "'%-5.*s'", 5, input));
  EXPECT_TRUE(test_printf("'0123456789'", "'%-5.*s'", 10, input));

  EXPECT_TRUE(test_printf("'          '", "'%-10.*s'", 0, input));
  EXPECT_TRUE(test_printf("'0         '", "'%-10.*s'", 1, input));
  EXPECT_TRUE(test_printf("'01        '", "'%-10.*s'", 2, input));
  EXPECT_TRUE(test_printf("'01234     '", "'%-10.*s'", 5, input));
  EXPECT_TRUE(test_printf("'0123456789'", "'%-10.*s'", 10, input));

  EXPECT_TRUE(test_printf("'               '", "'%-15.*s'", 0, input));
  EXPECT_TRUE(test_printf("'0              '", "'%-15.*s'", 1, input));
  EXPECT_TRUE(test_printf("'01             '", "'%-15.*s'", 2, input));
  EXPECT_TRUE(test_printf("'01234          '", "'%-15.*s'", 5, input));
  EXPECT_TRUE(test_printf("'0123456789     '", "'%-15.*s'", 10, input));

  // Variable width; variable precision.
  EXPECT_TRUE(test_printf("''", "'%*.*s'", 0, 0, input));
  EXPECT_TRUE(test_printf("'0'", "'%*.*s'", 0, 1, input));
  EXPECT_TRUE(test_printf("'01'", "'%*.*s'", 0, 2, input));
  EXPECT_TRUE(test_printf("'01234'", "'%*.*s'", 0, 5, input));
  EXPECT_TRUE(test_printf("'0123456789'", "'%*.*s'", 0, 10, input));

  EXPECT_TRUE(test_printf("'     '", "'%*.*s'", 5, 0, input));
  EXPECT_TRUE(test_printf("'    0'", "'%*.*s'", 5, 1, input));
  EXPECT_TRUE(test_printf("'   01'", "'%*.*s'", 5, 2, input));
  EXPECT_TRUE(test_printf("'01234'", "'%*.*s'", 5, 5, input));
  EXPECT_TRUE(test_printf("'0123456789'", "'%*.*s'", 5, 10, input));

  EXPECT_TRUE(test_printf("'          '", "'%*.*s'", 10, 0, input));
  EXPECT_TRUE(test_printf("'         0'", "'%*.*s'", 10, 1, input));
  EXPECT_TRUE(test_printf("'        01'", "'%*.*s'", 10, 2, input));
  EXPECT_TRUE(test_printf("'     01234'", "'%*.*s'", 10, 5, input));
  EXPECT_TRUE(test_printf("'0123456789'", "'%*.*s'", 10, 10, input));

  EXPECT_TRUE(test_printf("'               '", "'%*.*s'", 15, 0, input));
  EXPECT_TRUE(test_printf("'              0'", "'%*.*s'", 15, 1, input));
  EXPECT_TRUE(test_printf("'             01'", "'%*.*s'", 15, 2, input));
  EXPECT_TRUE(test_printf("'          01234'", "'%*.*s'", 15, 5, input));
  EXPECT_TRUE(test_printf("'     0123456789'", "'%*.*s'", 15, 10, input));

  EXPECT_TRUE(test_printf("'     '", "'%*.*s'", -5, 0, input));
  EXPECT_TRUE(test_printf("'0    '", "'%*.*s'", -5, 1, input));
  EXPECT_TRUE(test_printf("'01   '", "'%*.*s'", -5, 2, input));
  EXPECT_TRUE(test_printf("'01234'", "'%*.*s'", -5, 5, input));
  EXPECT_TRUE(test_printf("'0123456789'", "'%*.*s'", -5, 10, input));

  EXPECT_TRUE(test_printf("'          '", "'%*.*s'", -10, 0, input));
  EXPECT_TRUE(test_printf("'0         '", "'%*.*s'", -10, 1, input));
  EXPECT_TRUE(test_printf("'01        '", "'%*.*s'", -10, 2, input));
  EXPECT_TRUE(test_printf("'01234     '", "'%*.*s'", -10, 5, input));
  EXPECT_TRUE(test_printf("'0123456789'", "'%*.*s'", -10, 10, input));

  EXPECT_TRUE(test_printf("'               '", "'%*.*s'", -15, 0, input));
  EXPECT_TRUE(test_printf("'0              '", "'%*.*s'", -15, 1, input));
  EXPECT_TRUE(test_printf("'01             '", "'%*.*s'", -15, 2, input));
  EXPECT_TRUE(test_printf("'01234          '", "'%*.*s'", -15, 5, input));
  EXPECT_TRUE(test_printf("'0123456789     '", "'%*.*s'", -15, 10, input));

  END_TEST;
}

// Test snprintf() when the output is larger than the given buffer.
bool snprintf_truncation_test() {
  BEGIN_TEST;

  char buf[32];

  memset(buf, 'x', sizeof(buf));
  static const char str[26] = "0123456789abcdef012345678";
  int shorter_length = 15;
  int result = snprintf(buf, shorter_length, "%s", str);

  // Check that snprintf() returns the length of the string that it would
  // have written if the buffer was big enough.
  EXPECT_EQ(result, (int)strlen(str));

  // Check that snprintf() wrote a truncated, terminated string.
  EXPECT_EQ(memcmp(buf, str, shorter_length - 1), 0);
  EXPECT_EQ(buf[shorter_length - 1], '\0');

  // Check that snprintf() did not overrun the buffer it was given.
  for (uint32_t i = shorter_length; i < sizeof(buf); ++i)
    EXPECT_EQ(buf[i], 'x');

  END_TEST;
}

// Test snprintf() with zero length.
bool snprintf_truncation_test_zero_length() {
  BEGIN_TEST;

  char buf[32];

  memset(buf, 'x', sizeof(buf));
  static const char str[26] = "0123456789abcdef012345678";

  // Write with len = 0 a little ways into the buffer (to make sure it doesn't
  // write to len -1).
  int result = snprintf(buf + 4, 0, "%s", str);

  // Check that snprintf() returns the length of the string that it would
  // have written if the buffer was big enough.
  EXPECT_EQ(result, (int)strlen(str));

  // Check that snprintf() did not write anything.
  for (auto c : buf)
    EXPECT_EQ(c, 'x');

  END_TEST;
}

// Test snprintf() with null pointer and zero length.
bool snprintf_truncation_test_null_buffer() {
  BEGIN_TEST;

  static const char str[26] = "0123456789abcdef012345678";
  int result = snprintf(nullptr, 0, "%s", str);

  // Check that snprintf() returns the length of the string that it would
  // have written if the buffer was big enough.
  EXPECT_EQ(result, (int)strlen(str));

  END_TEST;
}

BEGIN_TEST_CASE(printf_tests)
RUN_TEST(numbers)
RUN_TEST(hex)
RUN_TEST(alt_and_sign)
RUN_TEST(formatting)
//RUN_TEST(printf_field_width_and_precision_test)
RUN_TEST(snprintf_truncation_test)
RUN_TEST(snprintf_truncation_test_zero_length)
RUN_TEST(snprintf_truncation_test_null_buffer)
END_TEST_CASE(printf_tests)

}  // namespace
