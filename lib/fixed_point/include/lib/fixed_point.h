/*
 * Copyright (c) 2013, Google Inc. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once

#include <stdint.h>

#ifndef DEBUG_FIXED_POINT
#define DEBUG_FIXED_POINT 0
#endif

struct fp_32_64 {
    uint32_t l0;    /* unshifted value */
    uint32_t l32;   /* value shifted left 32 bits (or bit -1 to -32) */
    uint32_t l64;   /* value shifted left 64 bits (or bit -33 to -64) */
};

#include "fixed_point_debug.h"

static void
fp_32_64_div_32_32(struct fp_32_64 *result, uint32_t dividend, uint32_t divisor)
{
    uint64_t tmp;
    uint32_t rem;

    tmp = ((uint64_t)dividend << 32) / divisor;
    rem = ((uint64_t)dividend << 32) % divisor;
    result->l0 = tmp >> 32;
    result->l32 = tmp;
    tmp = ((uint64_t)rem << 32) / divisor;
    result->l64 = tmp;
}

static uint64_t
mul_u32_u32(uint32_t a, uint32_t b, int a_shift, int b_shift)
{
    uint64_t ret = (uint64_t)a * b;
    debug_mul_u32_u32(a, b, a_shift, b_shift, ret);
    return ret;
}

static uint64_t
u64_mul_u32_fp32_64(uint32_t a, struct fp_32_64 b)
{
    uint64_t tmp;
    uint64_t res_0;
    uint64_t res_l32;
    uint32_t res_l32_32;
    uint64_t ret;

    res_0 = mul_u32_u32(a, b.l0, 0, 0);
    tmp = mul_u32_u32(a, b.l32, 0, -32);
    res_0 += tmp >> 32;
    res_l32 = (uint32_t)tmp;
    res_l32 += mul_u32_u32(a, b.l64, 0, -64) >> 32; /* Improve rounding accuracy */
    res_0 += res_l32 >> 32;
    res_l32_32 = res_l32;
    ret = res_0 + (res_l32_32 >> 31); /* Round to nearest integer */

    debug_u64_mul_u32_fp32_64(a, b, res_0, res_l32_32, ret);

    return ret;
}

static uint32_t
u32_mul_u64_fp32_64(uint64_t a, struct fp_32_64 b)
{
    uint32_t a_r32 = a >> 32;
    uint32_t a_0 = a;
    uint64_t res_l32;
    uint32_t ret;

    /* mul_u32_u32(a_r32, b.l0, 32, 0) does not affect result */
    res_l32 = mul_u32_u32(a_0, b.l0, 0, 0) << 32;
    res_l32 += mul_u32_u32(a_r32, b.l32, 32, -32) << 32;
    res_l32 += mul_u32_u32(a_0, b.l32, 0, -32);
    res_l32 += mul_u32_u32(a_r32, b.l64, 32, -64);
    res_l32 += mul_u32_u32(a_0, b.l64, 0, -64) >> 32; /* Improve rounding accuracy */
    ret = (res_l32 >> 32) + ((uint32_t)res_l32 >> 31); /* Round to nearest integer */

    debug_u32_mul_u64_fp32_64(a, b, res_l32, ret);

    return ret;
}

static uint64_t
u64_mul_u64_fp32_64(uint64_t a, struct fp_32_64 b)
{
    uint32_t a_r32 = a >> 32;
    uint32_t a_0 = a;
    uint64_t res_0;
    uint64_t res_l32;
    uint32_t res_l32_32;
    uint64_t tmp;
    uint64_t ret;

    tmp = mul_u32_u32(a_r32, b.l0, 32, 0);
    res_0 = tmp << 32;
    tmp = mul_u32_u32(a_0, b.l0, 0, 0);
    res_0 += tmp;
    tmp = mul_u32_u32(a_r32, b.l32, 32, -32);
    res_0 += tmp;
    tmp = mul_u32_u32(a_0, b.l32, 0, -32);
    res_0 += tmp >> 32;
    res_l32 = (uint32_t)tmp;
    tmp = mul_u32_u32(a_r32, b.l64, 32, -64);
    res_0 += tmp >> 32;
    res_l32 += (uint32_t)tmp;
    tmp = mul_u32_u32(a_0, b.l64, 0, -64); /* Improve rounding accuracy */
    res_l32 += tmp >> 32;
    res_0 += res_l32 >> 32;
    res_l32_32 = res_l32;
    ret = res_0 +  (res_l32_32 >> 31); /* Round to nearest integer */

    debug_u64_mul_u64_fp32_64(a, b, res_0, res_l32_32, ret);

    return ret;
}

