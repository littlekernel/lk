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
    { .i = 0xc000000000000000, .expected_f="-2.000000", .expected_F="-2.000000", .expected_a="-0x1p+1", .expected_A="-0X1P+1" },
    { .i = 0xbff0000000000000, .expected_f="-1.000000", .expected_F="-1.000000", .expected_a="-0x1p+0", .expected_A="-0X1P+0" },
    { .i = 0xbfe0000000000000, .expected_f="-0.500000", .expected_F="-0.500000", .expected_a="-0x1p-1", .expected_A="-0X1P-1" },
    { .i = 0x8000000000000000, .expected_f="-0.000000", .expected_F="-0.000000", .expected_a="-0x0p+0", .expected_A="-0X0P+0" },
    { .i = 0x0000000000000000, .expected_f="0.000000", .expected_F="0.000000", .expected_a="0x0p+0", .expected_A="0X0P+0" },
    { .i = 0x3f847ae147ae147b, .expected_f="0.010000", .expected_F="0.010000", .expected_a="0x1.47ae147ae147bp-7", .expected_A="0X1.47AE147AE147BP-7" },
    { .i = 0x3fb999999999999a, .expected_f="0.100000", .expected_F="0.100000", .expected_a="0x1.999999999999ap-4", .expected_A="0X1.999999999999AP-4" },
    { .i = 0x3fc999999999999a, .expected_f="0.200000", .expected_F="0.200000", .expected_a="0x1.999999999999ap-3", .expected_A="0X1.999999999999AP-3" },
    { .i = 0x3fd0000000000000, .expected_f="0.250000", .expected_F="0.250000", .expected_a="0x1p-2", .expected_A="0X1P-2" },
    { .i = 0x3fe0000000000000, .expected_f="0.500000", .expected_F="0.500000", .expected_a="0x1p-1", .expected_A="0X1P-1" },
    { .i = 0x3fe8000000000000, .expected_f="0.750000", .expected_F="0.750000", .expected_a="0x1.8p-1", .expected_A="0X1.8P-1" },
    { .i = 0x3ff0000000000000, .expected_f="1.000000", .expected_F="1.000000", .expected_a="0x1p+0", .expected_A="0X1P+0" },
    { .i = 0x4000000000000000, .expected_f="2.000000", .expected_F="2.000000", .expected_a="0x1p+1", .expected_A="0X1P+1" },
    { .i = 0x4008000000000000, .expected_f="3.000000", .expected_F="3.000000", .expected_a="0x1.8p+1", .expected_A="0X1.8P+1" },
    { .i = 0x4024000000000000, .expected_f="10.000000", .expected_F="10.000000", .expected_a="0x1.4p+3", .expected_A="0X1.4P+3" },
    { .i = 0x4059000000000000, .expected_f="100.000000", .expected_F="100.000000", .expected_a="0x1.9p+6", .expected_A="0X1.9P+6" },
    { .i = 0x40fe240000000000, .expected_f="123456.000000", .expected_F="123456.000000", .expected_a="0x1.e24p+16", .expected_A="0X1.E24P+16" },
    { .i = 0xc0fe240000000000, .expected_f="-123456.000000", .expected_F="-123456.000000", .expected_a="-0x1.e24p+16", .expected_A="-0X1.E24P+16" },
    { .i = 0x408114843a5e3464, .expected_f="546.564564", .expected_F="546.564564", .expected_a="0x1.114843a5e3464p+9", .expected_A="0X1.114843A5E3464P+9" },
    { .i = 0xc08114843a5e3464, .expected_f="-546.564564", .expected_F="-546.564564", .expected_a="-0x1.114843a5e3464p+9", .expected_A="-0X1.114843A5E3464P+9" },
    { .i = 0x3fbf9a6b50b0f27c, .expected_f="0.123450", .expected_F="0.123450", .expected_a="0x1.f9a6b50b0f27cp-4", .expected_A="0X1.F9A6B50B0F27CP-4" },
    { .i = 0x3eb4b6231abfd271, .expected_f="0.000001", .expected_F="0.000001", .expected_a="0x1.4b6231abfd271p-20", .expected_A="0X1.4B6231ABFD271P-20" },
    { .i = 0x3ec0c6c0a6f639de, .expected_f="0.000002", .expected_F="0.000002", .expected_a="0x1.0c6c0a6f639dep-19", .expected_A="0X1.0C6C0A6F639DEP-19" },
    { .i = 0x3eb92a737110e454, .expected_f="0.000002", .expected_F="0.000002", .expected_a="0x1.92a737110e454p-20", .expected_A="0X1.92A737110E454P-20" },
    { .i = 0x4005bf0a8b145649, .expected_f="2.718282", .expected_F="2.718282", .expected_a="0x1.5bf0a8b145649p+1", .expected_A="0X1.5BF0A8B145649P+1" },
    { .i = 0x400921fb54442d18, .expected_f="3.141593", .expected_F="3.141593", .expected_a="0x1.921fb54442d18p+1", .expected_A="0X1.921FB54442D18P+1" },
    { .i = 0x43f0000000000000, .expected_f="<range>", .expected_F="<range>", .expected_a="0x1p+64", .expected_A="0X1P+64" },
    { .i = 0x7fefffffffffffff, .expected_f="<range>", .expected_F="<range>", .expected_a="0x1.fffffffffffffp+1023", .expected_A="0X1.FFFFFFFFFFFFFP+1023" },
    { .i = 0x0010000000000000, .expected_f="<range>", .expected_F="<range>", .expected_a="0x1p-1022", .expected_A="0X1P-1022" },
    { .i = 0x0000000000000001, .expected_f="den", .expected_F="DEN", .expected_a="den", .expected_A="DEN" },
    { .i = 0x000fffffffffffff, .expected_f="den", .expected_F="DEN", .expected_a="den", .expected_A="DEN" },
    { .i = 0x7ff0000000000001, .expected_f="nan", .expected_F="NAN", .expected_a="nan", .expected_A="NAN" },
    { .i = 0x7ff7ffffffffffff, .expected_f="nan", .expected_F="NAN", .expected_a="nan", .expected_A="NAN" },
    { .i = 0x7ff8000000000000, .expected_f="nan", .expected_F="NAN", .expected_a="nan", .expected_A="NAN" },
    { .i = 0x7fffffffffffffff, .expected_f="nan", .expected_F="NAN", .expected_a="nan", .expected_A="NAN" },
    { .i = 0xfff0000000000000, .expected_f="-inf", .expected_F="-INF", .expected_a="-inf", .expected_A="-INF" },
    { .i = 0x7ff0000000000000, .expected_f="inf", .expected_F="INF", .expected_a="inf", .expected_A="INF" },
};
