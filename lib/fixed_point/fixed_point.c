/*
 * Copyright (c) 2025 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <lib/fixed_point.h>

/*
 * Format a fixed-point number as a decimal string without using floating point.
 * The fp_32_64 structure represents a number in base-2 fixed-point:
 *   value = l0 + l32/2^32 + l64/2^64
 * This function converts it to decimal notation: "l0.ddddd..."
 */
char *fp_32_64_snprintf(char *buf, size_t buf_size, const struct fp_32_64 *fp, int decimals) {
    // buf_size == 0 is undefined per request
    if (decimals > 20) decimals = 20;
    if (decimals < 0) decimals = 0;

    char *ptr = buf;
    char *end = buf + buf_size - 1; // Reserve space for null terminator

    // Print the integer part
    uint32_t integer = fp->l0;

    // Handle integer part by converting to string manually
    char int_buf[16]; // Enough for 32-bit integer
    int int_len = 0;
    uint32_t temp = integer;

    // Special case for zero
    if (temp == 0) {
        int_buf[int_len++] = '0';
    } else {
        // Build digits in reverse
        while (temp > 0 && int_len < (int)sizeof(int_buf)) {
            int_buf[int_len++] = (char)('0' + (temp % 10));
            temp /= 10;
        }
    }

    // Copy integer digits in correct order
    for (int i = int_len - 1; i >= 0 && ptr < end; i--) {
        *ptr++ = int_buf[i];
    }

    // If no decimals requested, we're done
    if (decimals == 0 || ptr >= end) {
        *ptr = '\0';
        return buf;
    }

    // Add decimal point
    *ptr++ = '.';

    // Convert fractional part to decimal
    // We have: frac = l32/2^32 + l64/2^64
    // To get decimal digits, we multiply by 10 repeatedly and extract the integer part

    // Work with a 64-bit fraction representing l32/2^32 + l64/2^64
    // We'll use 64-bit arithmetic: numerator is (l32 << 32) | l64, denominator is 2^64
    // But we can optimize by keeping the value in [0,1) form

    uint64_t frac = ((uint64_t)fp->l32 << 32) | fp->l64;

    for (int d = 0; d < decimals && ptr < end; d++) {
        // Multiply fraction by 10 in 128-bit space: frac * (8 + 2)
        uint64_t x = frac;
        uint64_t times_2 = x << 1;      // low 64 bits of x*2
        uint64_t times_8 = x << 3;      // low 64 bits of x*8
        uint64_t result_lo = times_2 + times_8; // low 64 bits of x*10
        uint64_t carry = (result_lo < times_2) ? 1 : 0; // carry from addition

        // High 64 bits of x*10 come from high bits shifted out of x*2 and x*8 plus carry.
        // x*8 high part = x >> 61, x*2 high part = x >> 63.
        // Sum is at most 7 + 1 + 1 = 9, fitting in a single decimal digit.
        int digit = (int)((x >> 61) + (x >> 63) + carry);
        *ptr++ = (char)('0' + digit);

        // Remaining fraction for next iteration is the low 64 bits.
        frac = result_lo;
    }

    *ptr = '\0';
    return buf;
}
