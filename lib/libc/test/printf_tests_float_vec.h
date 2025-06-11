/*
 * Copyright (c) 2014 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <stdint.h>

struct float_test_vec_t {
    constexpr float_test_vec_t(uint64_t _i, const char *_f, const char *_F, const char *_a, const char *_A)
        : i(_i), expected_f(_f), expected_F(_F), expected_a(_a), expected_A(_A) {}
    union {
        double d;
        uint64_t i;
    };
    const char *expected_f;
    const char *expected_F;
    const char *expected_a;
    const char *expected_A;
};

const struct float_test_vec_t float_test_vec[] = {
    { 0xc000000000000000, "-2.000000", "-2.000000", "-0x1p+1", "-0X1P+1" },
    { 0xbff0000000000000, "-1.000000", "-1.000000", "-0x1p+0", "-0X1P+0" },
    { 0xbfe0000000000000, "-0.500000", "-0.500000", "-0x1p-1", "-0X1P-1" },
    { 0x8000000000000000, "-0.000000", "-0.000000", "-0x0p+0", "-0X0P+0" },
    { 0x0000000000000000, "0.000000", "0.000000", "0x0p+0", "0X0P+0" },
    { 0x3f847ae147ae147b, "0.010000", "0.010000", "0x1.47ae147ae147bp-7", "0X1.47AE147AE147BP-7" },
    { 0x3fb999999999999a, "0.100000", "0.100000", "0x1.999999999999ap-4", "0X1.999999999999AP-4" },
    { 0x3fc999999999999a, "0.200000", "0.200000", "0x1.999999999999ap-3", "0X1.999999999999AP-3" },
    { 0x3fd0000000000000, "0.250000", "0.250000", "0x1p-2", "0X1P-2" },
    { 0x3fe0000000000000, "0.500000", "0.500000", "0x1p-1", "0X1P-1" },
    { 0x3fe8000000000000, "0.750000", "0.750000", "0x1.8p-1", "0X1.8P-1" },
    { 0x3ff0000000000000, "1.000000", "1.000000", "0x1p+0", "0X1P+0" },
    { 0x4000000000000000, "2.000000", "2.000000", "0x1p+1", "0X1P+1" },
    { 0x4008000000000000, "3.000000", "3.000000", "0x1.8p+1", "0X1.8P+1" },
    { 0x4024000000000000, "10.000000", "10.000000", "0x1.4p+3", "0X1.4P+3" },
    { 0x4059000000000000, "100.000000", "100.000000", "0x1.9p+6", "0X1.9P+6" },
    { 0x40fe240000000000, "123456.000000", "123456.000000", "0x1.e24p+16", "0X1.E24P+16" },
    { 0xc0fe240000000000, "-123456.000000", "-123456.000000", "-0x1.e24p+16", "-0X1.E24P+16" },
    { 0x408114843a5e3464, "546.564564", "546.564564", "0x1.114843a5e3464p+9", "0X1.114843A5E3464P+9" },
    { 0xc08114843a5e3464, "-546.564564", "-546.564564", "-0x1.114843a5e3464p+9", "-0X1.114843A5E3464P+9" },
    { 0x3fbf9a6b50b0f27c, "0.123450", "0.123450", "0x1.f9a6b50b0f27cp-4", "0X1.F9A6B50B0F27CP-4" },
    { 0x3eb4b6231abfd271, "0.000001", "0.000001", "0x1.4b6231abfd271p-20", "0X1.4B6231ABFD271P-20" },
    { 0x3ec0c6c0a6f639de, "0.000002", "0.000002", "0x1.0c6c0a6f639dep-19", "0X1.0C6C0A6F639DEP-19" },
    { 0x3eb92a737110e454, "0.000002", "0.000002", "0x1.92a737110e454p-20", "0X1.92A737110E454P-20" },
    { 0x4005bf0a8b145649, "2.718282", "2.718282", "0x1.5bf0a8b145649p+1", "0X1.5BF0A8B145649P+1" },
    { 0x400921fb54442d18, "3.141593", "3.141593", "0x1.921fb54442d18p+1", "0X1.921FB54442D18P+1" },
    { 0x43f0000000000000, "<range>", "<range>", "0x1p+64", "0X1P+64" },
    { 0x7fefffffffffffff, "<range>", "<range>", "0x1.fffffffffffffp+1023", "0X1.FFFFFFFFFFFFFP+1023" },
    { 0x0010000000000000, "<range>", "<range>", "0x1p-1022", "0X1P-1022" },
    { 0x0000000000000001, "den", "DEN", "den", "DEN" },
    { 0x000fffffffffffff, "den", "DEN", "den", "DEN" },
    { 0x7ff0000000000001, "nan", "NAN", "nan", "NAN" },
    { 0x7ff7ffffffffffff, "nan", "NAN", "nan", "NAN" },
    { 0x7ff8000000000000, "nan", "NAN", "nan", "NAN" },
    { 0x7fffffffffffffff, "nan", "NAN", "nan", "NAN" },
    { 0xfff0000000000000, "-inf", "-INF", "-inf", "-INF" },
    { 0x7ff0000000000000, "inf", "INF", "inf", "INF" },
};
