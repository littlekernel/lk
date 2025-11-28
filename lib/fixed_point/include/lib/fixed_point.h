/*
 * Copyright (c) 2013, Google Inc. All rights reserved.
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

struct fp_32_64 {
    uint32_t l0;  /* unshifted value */
    uint32_t l32; /* value shifted left 32 bits (or bit -1 to -32) */
    uint32_t l64; /* value shifted left 64 bits (or bit -33 to -64) */
};

static inline void
fp_32_64_div_32_32(struct fp_32_64 *result, uint32_t dividend, uint32_t divisor) {
    uint64_t tmp;
    uint32_t rem;

    tmp = ((uint64_t)dividend << 32) / divisor;
    rem = ((uint64_t)dividend << 32) % divisor;
    result->l0 = tmp >> 32;
    result->l32 = tmp;
    tmp = ((uint64_t)rem << 32) / divisor;
    result->l64 = tmp;
}

static inline void
fp_32_64_div_64_32(struct fp_32_64 *result, uint64_t dividend, uint32_t divisor) {
    // Compute dividend / divisor in fixed point format
    // The result is dividend / divisor with fractional bits stored in l32 and l64

    // First, compute the integer part and get the remainder
    result->l0 = dividend / divisor;
    uint64_t rem = dividend % divisor;

    // Now compute the fractional part by shifting the remainder left by 32 bits
    // This gives us bits -1 to -32 (stored in l32)
    uint64_t tmp = (rem << 32) / divisor;
    result->l32 = tmp;
    rem = (rem << 32) % divisor;

    // Finally compute bits -33 to -64 (stored in l64)
    tmp = (rem << 32) / divisor;
    result->l64 = tmp;
}

static inline void
fp_32_64_div_32_64(struct fp_32_64 *result, uint32_t dividend, uint64_t divisor) {
    // Compute dividend / divisor in fixed point format where divisor is 64-bit
    // When dividend < divisor, result->l0 will be 0

    result->l0 = dividend / divisor;
    uint64_t rem = dividend % divisor;

    // Compute fractional bits by shifting remainder left
    uint64_t tmp = (rem << 32) / divisor;
    result->l32 = tmp;
    rem = (rem << 32) % divisor;

    tmp = (rem << 32) / divisor;
    result->l64 = tmp;
}

static inline uint64_t
mul_u32_u32(uint32_t a, uint32_t b, int a_shift, int b_shift) {
    uint64_t ret = (uint64_t)a * b;
    return ret;
}

static inline uint64_t
u64_mul_u32_fp32_64(uint32_t a, struct fp_32_64 b) {
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

    return ret;
}

static inline uint32_t
u32_mul_u64_fp32_64(uint64_t a, struct fp_32_64 b) {
    uint32_t a_r32 = a >> 32;
    uint32_t a_0 = a;
    uint64_t res_l32;
    uint32_t ret;

    /* mul_u32_u32(a_r32, b.l0, 32, 0) does not affect result */
    res_l32 = mul_u32_u32(a_0, b.l0, 0, 0) << 32;
    res_l32 += mul_u32_u32(a_r32, b.l32, 32, -32) << 32;
    res_l32 += mul_u32_u32(a_0, b.l32, 0, -32);
    res_l32 += mul_u32_u32(a_r32, b.l64, 32, -64);
    res_l32 += mul_u32_u32(a_0, b.l64, 0, -64) >> 32;  /* Improve rounding accuracy */
    ret = (res_l32 >> 32) + ((uint32_t)res_l32 >> 31); /* Round to nearest integer */

    return ret;
}

static inline uint64_t
u64_mul_u64_fp32_64(uint64_t a, struct fp_32_64 b) {
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
    ret = res_0 + (res_l32_32 >> 31); /* Round to nearest integer */

    return ret;
}

/*
 * Format a fixed-point number as a decimal string without using floating point.
 * The fp_32_64 structure represents a number in base-2 fixed-point:
 *   value = l0 + l32/2^32 + l64/2^64
 * This function converts it to decimal notation: "l0.ddddd..."
 *
 * @param buf: Output buffer for the formatted string
 * @param buf_size: Size of the output buffer
 * @param fp: Fixed-point number to format
 * @param decimals: Number of decimal places to output (maximum 20)
 * @return: Pointer to the provided buffer containing the formatted string
 *          (undefined behavior if buf_size == 0)
 */
char *fp_32_64_snprintf(char *buf, size_t buf_size, const struct fp_32_64 *fp, int decimals);
