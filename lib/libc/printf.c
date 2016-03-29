/*
 * Copyright (c) 2008-2014 Travis Geiselbrecht
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
#include <debug.h>
#include <assert.h>
#include <limits.h>
#include <printf.h>
#include <stdarg.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <platform/debug.h>

#if WITH_NO_FP
#define FLOAT_PRINTF 0
#else
#define FLOAT_PRINTF 1
#endif

int sprintf(char *str, const char *fmt, ...)
{
    int err;

    va_list ap;
    va_start(ap, fmt);
    err = vsprintf(str, fmt, ap);
    va_end(ap);

    return err;
}

int snprintf(char *str, size_t len, const char *fmt, ...)
{
    int err;

    va_list ap;
    va_start(ap, fmt);
    err = vsnprintf(str, len, fmt, ap);
    va_end(ap);

    return err;
}

int vsprintf(char *str, const char *fmt, va_list ap)
{
    return vsnprintf(str, INT_MAX, fmt, ap);
}

struct _output_args {
    char *outstr;
    size_t len;
    size_t pos;
};

static int _vsnprintf_output(const char *str, size_t len, void *state)
{
    struct _output_args *args = state;

    size_t count = 0;
    while (count < len) {
        if (args->pos < args->len) {
            args->outstr[args->pos++] = *str;
        }

        str++;
        count++;
    }

    return count;
}

int vsnprintf(char *str, size_t len, const char *fmt, va_list ap)
{
    struct _output_args args;
    int wlen;

    args.outstr = str;
    args.len = len;
    args.pos = 0;

    wlen = _printf_engine(&_vsnprintf_output, (void *)&args, fmt, ap);
    if (args.pos >= len)
        str[len-1] = '\0';
    else
        str[wlen] = '\0';
    return wlen;
}

#define LONGFLAG       0x00000001
#define LONGLONGFLAG   0x00000002
#define HALFFLAG       0x00000004
#define HALFHALFFLAG   0x00000008
#define SIZETFLAG      0x00000010
#define INTMAXFLAG     0x00000020
#define PTRDIFFFLAG    0x00000040
#define ALTFLAG        0x00000080
#define CAPSFLAG       0x00000100
#define SHOWSIGNFLAG   0x00000200
#define SIGNEDFLAG     0x00000400
#define LEFTFORMATFLAG 0x00000800
#define LEADZEROFLAG   0x00001000
#define BLANKPOSFLAG   0x00002000

__NO_INLINE static char *longlong_to_string(char *buf, unsigned long long n, size_t len, uint flag, char *signchar)
{
    size_t pos = len;
    int negative = 0;

    if ((flag & SIGNEDFLAG) && (long long)n < 0) {
        negative = 1;
        n = -n;
    }

    buf[--pos] = 0;

    /* only do the math if the number is >= 10 */
    while (n >= 10) {
        int digit = n % 10;

        n /= 10;

        buf[--pos] = digit + '0';
    }
    buf[--pos] = n + '0';

    if (negative)
        *signchar = '-';
    else if ((flag & SHOWSIGNFLAG))
        *signchar = '+';
    else if ((flag & BLANKPOSFLAG))
        *signchar = ' ';
    else
        *signchar = '\0';

    return &buf[pos];
}

static const char hextable[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
static const char hextable_caps[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

__NO_INLINE static char *longlong_to_hexstring(char *buf, unsigned long long u, size_t len, uint flag)
{
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

#if FLOAT_PRINTF
union double_int {
    double d;
    uint64_t i;
};

#define OUT(c) buf[pos++] = (c)
#define OUTSTR(str) do { for (size_t i = 0; (str)[i] != 0; i++) OUT((str)[i]); } while (0)

/* print up to a 4 digit exponent as string, with sign */
__NO_INLINE static size_t exponent_to_string(char *buf, int32_t exponent)
{
    size_t pos = 0;

    /* handle sign */
    if (exponent < 0) {
        OUT('-');
        exponent = -exponent;
    } else {
        OUT('+');
    }

    /* see how far we need to bump into the string to print from the right */
    if (exponent >= 1000) pos += 4;
    else if (exponent >= 100) pos += 3;
    else if (exponent >= 10) pos += 2;
    else pos++;

    /* print decimal string, from the right */
    uint i = pos;
    do {
        uint digit = (uint32_t)exponent % 10;

        buf[--i] = digit + '0';

        exponent /= 10;
    } while (exponent != 0);

    /* return number of characters printed */
    return pos;
}

__NO_INLINE static char *double_to_string(char *buf, size_t len, double d, uint flag)
{
    size_t pos = 0;
    union double_int u = { d };

    uint32_t exponent = (u.i >> 52) & 0x7ff;
    uint64_t fraction = (u.i & ((1ULL << 52) - 1));
    bool neg = !!(u.i & (1ULL << 63));

    /* start constructing the string */
    if (neg) {
        OUT('-');
        d = -d;
    }

    /* longest:
     * 179769313486231570814527423731704356798070567525844996598917476803157260780028538760589558632766878171540458953514382464234321326889464182768467546703537516986049910576551282076245490090389328944075868508455133942304583236903222948165808559332123348274797826204144723168738177180919299881250404026184124858368.000000o
     */

    /* look for special cases */
    if (exponent == 0x7ff) {
        if (fraction == 0) {
            /* infinity */
            if (flag & CAPSFLAG) OUTSTR("INF");
            else OUTSTR("inf");
        } else {
            /* NaN */
            if (flag & CAPSFLAG) OUTSTR("NAN");
            else OUTSTR("nan");
        }
    } else if (exponent == 0) {
        if (fraction == 0) {
            /* zero */
            OUTSTR("0.000000");
        } else {
            /* denormalized */
            /* XXX does not handle */
            if (flag & CAPSFLAG) OUTSTR("DEN");
            else OUTSTR("den");
        }
    } else {
        /* see if it's in the range of floats we can easily print */
        int exponent_signed = exponent - 1023;
        if (exponent_signed < -52 || exponent_signed > 52) {
            OUTSTR("<range>");
        } else {
            /* start by walking backwards through the string */
#define OUTREV(c) do { if (&buf[pos] == buf) goto done; else buf[--pos] = (c); } while (0)
            pos = len;
            OUTREV(0);

            /* reserve space for the fractional component first */
            for (int i = 0; i <= 6; i++)
                OUTREV('0');
            size_t decimal_spot = pos;

            /* print the integer portion */
            uint64_t u;
            if (exponent_signed >= 0) {
                u = fraction;
                u |= (1ULL<<52);
                u >>= (52 - exponent_signed);

                char *s = longlong_to_string(buf, u, pos + 1, flag, &(char) {0});

                pos = s - buf;
            } else {
                /* exponent is negative */
                u = 0;
                OUTREV('0');
            }

            buf[decimal_spot] = '.';

            /* handle the fractional part */
            uint32_t frac = ((d - u) * 1000000) + .5;

            uint i = decimal_spot + 6 + 1;
            while (frac != 0) {
                uint digit = frac % 10;

                buf[--i] = digit + '0';

                frac /= 10;
            }

            if (neg)
                OUTREV('-');

done:
            /* separate return path, since we've been walking backwards through the string */
            return &buf[pos];
        }
#undef OUTREV
    }

    buf[pos] = 0;
    return buf;
}

__NO_INLINE static char *double_to_hexstring(char *buf, size_t len, double d, uint flag)
{
    size_t pos = 0;
    union double_int u = { d };

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
            if (flag & CAPSFLAG) OUTSTR("INF");
            else OUTSTR("inf");
        } else {
            /* NaN */
            if (flag & CAPSFLAG) OUTSTR("NAN");
            else OUTSTR("nan");
        }
    } else if (exponent == 0) {
        if (fraction == 0) {
            /* zero */
            if (flag & CAPSFLAG) OUTSTR("0X0P+0");
            else OUTSTR("0x0p+0");
        } else {
            /* denormalized */
            /* XXX does not handle */
            if (flag & CAPSFLAG) OUTSTR("DEN");
            else OUTSTR("den");
        }
    } else {
        /* regular normalized numbers:
         * 0x1p+1
         * 0x1.0000000000001p+1
         * 0X1.FFFFFFFFFFFFFP+1023
         * 0x1.FFFFFFFFFFFFFP+1023
         */
        int exponent_signed = exponent - 1023;

        /* implicit 1. */
        if (flag & CAPSFLAG) OUTSTR("0X1");
        else OUTSTR("0x1");

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
        buf[pos++] = (flag & CAPSFLAG) ? 'P' : 'p';
        pos += exponent_to_string(&buf[pos], exponent_signed);
    }

    buf[pos] = 0;
    return buf;
}

#undef OUT
#undef OUTSTR

#endif // FLOAT_PRINTF

int _printf_engine(_printf_engine_output_func out, void *state, const char *fmt, va_list ap)
{
    int err = 0;
    char c;
    unsigned char uc;
    const char *s;
    size_t string_len;
    unsigned long long n;
    void *ptr;
    int flags;
    unsigned int format_num;
    char signchar;
    size_t chars_written = 0;
    char num_buffer[32];

#define OUTPUT_STRING(str, len) do { err = out(str, len, state); if (err < 0) { goto exit; } else { chars_written += err; } } while(0)
#define OUTPUT_CHAR(c) do { char __temp[1] = { c }; OUTPUT_STRING(__temp, 1); } while (0)

    for (;;) {
        /* reset the format state */
        flags = 0;
        format_num = 0;
        signchar = '\0';

        /* handle regular chars that aren't format related */
        s = fmt;
        string_len = 0;
        while ((c = *fmt++) != 0) {
            if (c == '%')
                break; /* we saw a '%', break and start parsing format */
            string_len++;
        }

        /* output the string we've accumulated */
        OUTPUT_STRING(s, string_len);

        /* make sure we haven't just hit the end of the string */
        if (c == 0)
            break;

next_format:
        /* grab the next format character */
        c = *fmt++;
        if (c == 0)
            break;

        switch (c) {
            case '0'...'9':
                if (c == '0' && format_num == 0)
                    flags |= LEADZEROFLAG;
                format_num *= 10;
                format_num += c - '0';
                goto next_format;
            case '.':
                /* XXX for now eat numeric formatting */
                goto next_format;
            case '%':
                OUTPUT_CHAR('%');
                break;
            case 'c':
                uc = va_arg(ap, unsigned int);
                OUTPUT_CHAR(uc);
                break;
            case 's':
                s = va_arg(ap, const char *);
                if (s == 0)
                    s = "<null>";
                flags &= ~LEADZEROFLAG; /* doesn't make sense for strings */
                goto _output_string;
            case '-':
                flags |= LEFTFORMATFLAG;
                goto next_format;
            case '+':
                flags |= SHOWSIGNFLAG;
                goto next_format;
            case ' ':
                flags |= BLANKPOSFLAG;
                goto next_format;
            case '#':
                flags |= ALTFLAG;
                goto next_format;
            case 'l':
                if (flags & LONGFLAG)
                    flags |= LONGLONGFLAG;
                flags |= LONGFLAG;
                goto next_format;
            case 'h':
                if (flags & HALFFLAG)
                    flags |= HALFHALFFLAG;
                flags |= HALFFLAG;
                goto next_format;
            case 'z':
                flags |= SIZETFLAG;
                goto next_format;
            case 'j':
                flags |= INTMAXFLAG;
                goto next_format;
            case 't':
                flags |= PTRDIFFFLAG;
                goto next_format;
            case 'i':
            case 'd':
                n = (flags & LONGLONGFLAG) ? va_arg(ap, long long) :
                    (flags & LONGFLAG) ? va_arg(ap, long) :
                    (flags & HALFHALFFLAG) ? (signed char)va_arg(ap, int) :
                    (flags & HALFFLAG) ? (short)va_arg(ap, int) :
                    (flags & SIZETFLAG) ? va_arg(ap, ssize_t) :
                    (flags & INTMAXFLAG) ? va_arg(ap, intmax_t) :
                    (flags & PTRDIFFFLAG) ? va_arg(ap, ptrdiff_t) :
                    va_arg(ap, int);
                flags |= SIGNEDFLAG;
                s = longlong_to_string(num_buffer, n, sizeof(num_buffer), flags, &signchar);
                goto _output_string;
            case 'u':
                n = (flags & LONGLONGFLAG) ? va_arg(ap, unsigned long long) :
                    (flags & LONGFLAG) ? va_arg(ap, unsigned long) :
                    (flags & HALFHALFFLAG) ? (unsigned char)va_arg(ap, unsigned int) :
                    (flags & HALFFLAG) ? (unsigned short)va_arg(ap, unsigned int) :
                    (flags & SIZETFLAG) ? va_arg(ap, size_t) :
                    (flags & INTMAXFLAG) ? va_arg(ap, uintmax_t) :
                    (flags & PTRDIFFFLAG) ? (uintptr_t)va_arg(ap, ptrdiff_t) :
                    va_arg(ap, unsigned int);
                s = longlong_to_string(num_buffer, n, sizeof(num_buffer), flags, &signchar);
                goto _output_string;
            case 'p':
                flags |= LONGFLAG | ALTFLAG;
                goto hex;
            case 'X':
                flags |= CAPSFLAG;
                /* fallthrough */
hex:
            case 'x':
                n = (flags & LONGLONGFLAG) ? va_arg(ap, unsigned long long) :
                    (flags & LONGFLAG) ? va_arg(ap, unsigned long) :
                    (flags & HALFHALFFLAG) ? (unsigned char)va_arg(ap, unsigned int) :
                    (flags & HALFFLAG) ? (unsigned short)va_arg(ap, unsigned int) :
                    (flags & SIZETFLAG) ? va_arg(ap, size_t) :
                    (flags & INTMAXFLAG) ? va_arg(ap, uintmax_t) :
                    (flags & PTRDIFFFLAG) ? (uintptr_t)va_arg(ap, ptrdiff_t) :
                    va_arg(ap, unsigned int);
                s = longlong_to_hexstring(num_buffer, n, sizeof(num_buffer), flags);
                if (flags & ALTFLAG) {
                    OUTPUT_CHAR('0');
                    OUTPUT_CHAR((flags & CAPSFLAG) ? 'X': 'x');
                }
                goto _output_string;
            case 'n':
                ptr = va_arg(ap, void *);
                if (flags & LONGLONGFLAG)
                    *(long long *)ptr = chars_written;
                else if (flags & LONGFLAG)
                    *(long *)ptr = chars_written;
                else if (flags & HALFHALFFLAG)
                    *(signed char *)ptr = chars_written;
                else if (flags & HALFFLAG)
                    *(short *)ptr = chars_written;
                else if (flags & SIZETFLAG)
                    *(size_t *)ptr = chars_written;
                else
                    *(int *)ptr = chars_written;
                break;
#if FLOAT_PRINTF
            case 'F':
                flags |= CAPSFLAG;
                /* fallthrough */
            case 'f': {
                double d = va_arg(ap, double);
                s = double_to_string(num_buffer, sizeof(num_buffer), d, flags);
                goto _output_string;
            }
            case 'A':
                flags |= CAPSFLAG;
                /* fallthrough */
            case 'a': {
                double d = va_arg(ap, double);
                s = double_to_hexstring(num_buffer, sizeof(num_buffer), d, flags);
                goto _output_string;
            }
#endif
            default:
                OUTPUT_CHAR('%');
                OUTPUT_CHAR(c);
                break;
        }

        /* move on to the next field */
        continue;

        /* shared output code */
_output_string:
        string_len = strlen(s);

        if (flags & LEFTFORMATFLAG) {
            /* left justify the text */
            OUTPUT_STRING(s, string_len);
            uint written = err;

            /* pad to the right (if necessary) */
            for (; format_num > written; format_num--)
                OUTPUT_CHAR(' ');
        } else {
            /* right justify the text (digits) */

            /* if we're going to print a sign digit,
               it'll chew up one byte of the format size */
            if (signchar != '\0' && format_num > 0)
                format_num--;

            /* output the sign char before the leading zeros */
            if (flags & LEADZEROFLAG && signchar != '\0')
                OUTPUT_CHAR(signchar);

            /* pad according to the format string */
            for (; format_num > string_len; format_num--)
                OUTPUT_CHAR(flags & LEADZEROFLAG ? '0' : ' ');

            /* if not leading zeros, output the sign char just before the number */
            if (!(flags & LEADZEROFLAG) && signchar != '\0')
                OUTPUT_CHAR(signchar);

            /* output the string */
            OUTPUT_STRING(s, string_len);
        }
        continue;
    }

#undef OUTPUT_STRING
#undef OUTPUT_CHAR

exit:
    return (err < 0) ? err : (int)chars_written;
}
