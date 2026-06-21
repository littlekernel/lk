/*
 * Copyright (c) 2008-2025 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <limits.h>
#include <lk/debug.h>
#include <printf.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include "printf_local.h"

// Various helper functions to generate numerical strings of various types.
// In a separate file to allow sharing of these routines between the floating and integer printf
// implementations.

char *_printf_longlong_to_string(char *buf, unsigned long long n, size_t len, uint flag,
                                 char *signchar) {
    size_t pos = len;
    bool negative = false;

    if ((flag & SIGNEDFLAG) && (long long)n < 0) {
        negative = true;
        n = -n;
    }

    buf[--pos] = 0;

    /* only do the math if the number is >= 10 */
    while (n >= 10) {
        unsigned int digit = n % 10;

        n /= 10;

        buf[--pos] = (char)digit + '0';
    }
    buf[--pos] = (char)n + '0';

    if (negative) {
        *signchar = '-';
    } else if ((flag & SHOWSIGNFLAG)) {
        *signchar = '+';
    } else if ((flag & BLANKPOSFLAG)) {
        *signchar = ' ';
    } else {
        *signchar = '\0';
    }

    return &buf[pos];
}

static const char hextable[] = { '0', '1', '2', '3', '4', '5', '6', '7',
                                 '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
static const char hextable_caps[] = { '0', '1', '2', '3', '4', '5', '6', '7',
                                      '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

char *_printf_longlong_to_hexstring(char *buf, unsigned long long u, size_t len, uint flag) {
    size_t pos = len;
    const char *table = (flag & CAPSFLAG) ? hextable_caps : hextable;

    buf[--pos] = 0;
    do {
        unsigned int digit = u % 16;
        u /= 16;

        buf[--pos] = table[digit];
    } while (u != 0);

    return &buf[pos];
}

union double_int {
    double d;
    uint64_t i;
};

#define OUT(c) buf[pos++] = (c)
#define OUTSTR(str)                                                                                \
    do {                                                                                           \
        for (size_t i = 0; (str)[i] != 0; i++) OUT((str)[i]);                                      \
    } while (0)

/* print up to a 4 digit exponent as string, with sign */
static size_t exponent_to_string(char *buf, int32_t exponent) {
    size_t pos = 0;

    /* handle sign */
    if (exponent < 0) {
        OUT('-');
        exponent = -exponent;
    } else {
        OUT('+');
    }

    /* see how far we need to bump into the string to print from the right */
    if (exponent >= 1000) {
        pos += 4;
    } else if (exponent >= 100) {
        pos += 3;
    } else if (exponent >= 10) {
        pos += 2;
    } else {
        pos++;
    }

    /* print decimal string, from the right */
    uint i = pos;
    do {
        uint digit = (uint32_t)exponent % 10;

        buf[--i] = (char)digit + '0';

        exponent /= 10;
    } while (exponent != 0);

    /* return number of characters printed */
    return pos;
}

char *_printf_double_to_string(char *buf, size_t len, uint64_t float_bits, uint flag) {
    size_t pos = 0;
    union double_int du = { .i = float_bits };

    uint32_t exponent = (du.i >> 52) & 0x7ff;
    uint64_t fraction = (du.i & ((1ULL << 52) - 1));
    bool neg = !!(du.i & (1ULL << 63));

    /* start constructing the string */
    if (neg) {
        OUT('-');
    }

    /* longest:
     * 179769313486231570814527423731704356798070567525844996598917476803157260780028538760589558632766878171540458953514382464234321326889464182768467546703537516986049910576551282076245490090389328944075868508455133942304583236903222948165808559332123348274797826204144723168738177180919299881250404026184124858368.000000o
     */

    /* look for special cases */
    if (exponent == 0x7ff) {
        if (fraction == 0) {
            /* infinity */
            if (flag & CAPSFLAG) {
                OUTSTR("INF");
            } else {
                OUTSTR("inf");
            }
        } else {
            /* NaN */
            if (flag & CAPSFLAG) {
                OUTSTR("NAN");
            } else {
                OUTSTR("nan");
            }
        }
    } else if (exponent == 0) {
        if (fraction == 0) {
            /* zero */
            OUTSTR("0.000000");
        } else {
            /* denormalized */
            /* XXX does not handle */
            if (flag & CAPSFLAG) {
                OUTSTR("DEN");
            } else {
                OUTSTR("den");
            }
        }
    } else {
        /* see if it's in the range of floats we can easily print */
        int exponent_signed = exponent - 1023;
        if (exponent_signed < -52 || exponent_signed > 52) {
            OUTSTR("<range>");
        } else {
            /* start by walking backwards through the string */
#define OUTREV(c)                                                                                  \
    do {                                                                                           \
        if (&buf[pos] == buf)                                                                      \
            goto done;                                                                             \
        else                                                                                       \
            buf[--pos] = (c);                                                                      \
    } while (0)
            pos = len;
            OUTREV(0);

            /* reserve space for the fractional component first */
            for (int i = 0; i <= 6; i++) {
                OUTREV('0');
            }
            size_t decimal_spot = pos;

            /* Extract integer and fractional parts */
            uint64_t u;
            uint64_t frac_bits_q64;

            if (exponent_signed >= 0) {
                /* Mantissa (53 bits) with implicit 1 */
                uint64_t mantissa = (1ULL << 52) | fraction;

                /* Integer part: shift right by (52 - exponent_signed) */
                /* Safe shift: ensure shift amount is valid */
                int shift_right = 52 - exponent_signed;
                if (shift_right >= 64) {
                    u = 0;
                } else if (shift_right <= 0) {
                    u = mantissa << (-shift_right);
                } else {
                    u = mantissa >> shift_right;
                }

                /* Fractional bits: lower bits that were shifted out */
                int frac_bit_count = 52 - exponent_signed;
                if (frac_bit_count <= 0) {
                    /* All bits are in the integer part; no fractional part */
                    frac_bits_q64 = 0;
                } else if (frac_bit_count >= 64) {
                    /* All 64 bits can be fractional */
                    frac_bits_q64 = mantissa << (64 - 52);
                } else {
                    /* Extract fractional bits and left-align to 64 bits */
                    uint64_t frac_mask = (1ULL << frac_bit_count) - 1;
                    frac_bits_q64 = mantissa & frac_mask;
                    frac_bits_q64 <<= (64 - frac_bit_count);
                }
            } else {
                /* exponent is negative: all bits are fractional, integer part is 0 */
                u = 0;

                /* Fractional bits: implicit 1 plus all 52 bits of fraction */
                int frac_bit_count = 52 - exponent_signed;
                if (frac_bit_count > 64) {
                    /* More than 64 bits of precision needed; right-shift to fit */
                    int shift_right = frac_bit_count - 64;
                    if (shift_right >= 64) {
                        /* Shift is too large; result is essentially 0 */
                        frac_bits_q64 = 0;
                    } else {
                        uint64_t mantissa_with_implicit = (1ULL << 52) | fraction;
                        frac_bits_q64 = mantissa_with_implicit >> shift_right;
                        /* Sticky bit: If any non-zero bits were truncated during the right shift,
                         * logically OR 1 into the lowest bit of frac_bits_q64 to record the
                         * precision loss. This ensures exact decimal rounding up for halfway (.5)
                         * boundary conditions. */
                        if ((mantissa_with_implicit & ((1ULL << shift_right) - 1)) != 0) {
                            frac_bits_q64 |= 1;
                        }
                    }
                } else {
                    /* Fewer than 64 bits; left-align to fill the 64-bit Q64 value */
                    uint64_t mantissa_with_implicit = (1ULL << 52) | fraction;
                    frac_bits_q64 = mantissa_with_implicit << (64 - frac_bit_count);
                }
            }

            /* Convert Q64 fractional bits to decimal digits using pure integer math */
            uint32_t frac = 0;
            for (int i = 0; i < 7; i++) {
                /* Multiply Q64 by 10 to extract next decimal digit */
                /* Q64 * 10 is a 128-bit result; digit = result >> 64; remainder = result &
                 * 0xFFFFFFFFFFFFFFFF */
                uint64_t xlo = frac_bits_q64 & 0xFFFFFFFFULL;
                uint64_t xhi = frac_bits_q64 >> 32;

                /* Compute (xhi << 32 + xlo) * 10 = xhi*10*2^32 + xlo*10 */
                uint64_t lo_product = xlo * 10;
                uint64_t hi_product = xhi * 10;

                /* Extract carry from lo_product into hi_product */
                uint64_t carry = lo_product >> 32;
                hi_product += carry;

                /* The digit is the new high 32 bits (overflow from hi_product) */
                uint digit = (uint)(hi_product >> 32);

                /* New Q64: keep lower 32 of hi_product and all of lo_product */
                frac_bits_q64 = ((hi_product & 0xFFFFFFFFULL) << 32) | (lo_product & 0xFFFFFFFFULL);

                frac = frac * 10 + digit;
            }

            /* Round to 6 decimal places: if 7th digit >= 5, round up */
            uint seventh_digit = frac % 10;
            frac /= 10;
            if (seventh_digit >= 5) {
                frac++;
                /* Check for rounding overflow (frac overflowed to 1000000) */
                if (frac >= 1000000) {
                    /* Rounding caused carry into integer part */
                    frac = 0;
                    u++;
                }
            }

            /* print the integer portion */
            char *s = _printf_longlong_to_string(buf, u, decimal_spot + 1, flag, &(char){ 0 });
            pos = s - buf;

            buf[decimal_spot] = '.';

            /* Print the fractional part (6 digits, zero-padded) */
            uint i = decimal_spot + 6 + 1;
            for (int j = 0; j < 6; j++) {
                uint digit = frac % 10;
                buf[--i] = digit + '0';
                frac /= 10;
            }

            if (neg) {
                OUTREV('-');
            }

done:
            /* separate return path, since we've been walking backwards through the string */
            return &buf[pos];
        }
#undef OUTREV
    }

    buf[pos] = 0;
    return buf;
}

char *_printf_double_to_hexstring(char *buf, size_t len, uint64_t float_bits, uint flag) {
    size_t pos = 0;
    union double_int u = { .i = float_bits };

    uint32_t exponent = (u.i >> 52) & 0x7ff;
    uint64_t fraction = (u.i & ((1ULL << 52) - 1));
    bool neg = !!(u.i & (1ULL << 63));

    /* start constructing the string */
    if (neg) {
        OUT('-');
    }

    /* look for special cases */
    if (exponent == 0x7ff) {
        if (fraction == 0) {
            /* infinity */
            if (flag & CAPSFLAG) {
                OUTSTR("INF");
            } else {
                OUTSTR("inf");
            }
        } else {
            /* NaN */
            if (flag & CAPSFLAG) {
                OUTSTR("NAN");
            } else {
                OUTSTR("nan");
            }
        }
    } else if (exponent == 0) {
        if (fraction == 0) {
            /* zero */
            if (flag & CAPSFLAG) {
                OUTSTR("0X0P+0");
            } else {
                OUTSTR("0x0p+0");
            }
        } else {
            /* denormalized */
            /* XXX does not handle */
            if (flag & CAPSFLAG) {
                OUTSTR("DEN");
            } else {
                OUTSTR("den");
            }
        }
    } else {
        /* regular normalized numbers:
         * 0x1p+1
         * 0x1.0000000000001p+1
         * 0X1.FFFFFFFFFFFFFP+1023
         * 0x1.FFFFFFFFFFFFFP+1023
         */
        int32_t exponent_signed = exponent - 1023;

        /* implicit 1. */
        if (flag & CAPSFLAG) {
            OUTSTR("0X1");
        } else {
            OUTSTR("0x1");
        }

        /* select the appropriate hex case table */
        const char *table = (flag & CAPSFLAG) ? hextable_caps : hextable;

        int zero_count = 0;
        bool output_dot = false;
        for (int i = 52 - 4; i >= 0; i -= 4) {
            uint digit = (fraction >> i) & 0xf;

            if (digit == 0) {
                zero_count++;
            } else {
                /* output a . the first time we output a char */
                if (!output_dot) {
                    OUT('.');
                    output_dot = true;
                }
                /* if we have a non zero digit, see if we need to output a string of zeros */
                while (zero_count > 0) {
                    OUT('0');
                    zero_count--;
                }
                buf[pos++] = table[digit];
            }
        }

        /* handle the exponent */
        OUT((flag & CAPSFLAG) ? 'P' : 'p');
        pos += exponent_to_string(&buf[pos], exponent_signed);
    }

    buf[pos] = 0;
    return buf;
}

#undef OUT
#undef OUTSTR
