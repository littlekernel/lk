/*
 * Copyright (c) 2008-2014 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <limits.h>
#include <lk/debug.h>
#include <platform/debug.h>
#include <printf.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include "printf_local.h"

// The main printf engine and all of the printf wrapper routines.
// It's important these are all in the same file, or at least all
// compiled with the same flags concerning floating point support.

// Included by two wrapper files (printf.c and printf_float.c) with
// different compile settings.

// If we're compiling with FLOAT_PRINTF set, emit all the functions
// with the _float suffix.
#if FLOAT_PRINTF
#define FUNC(x) x##_float
#else
#define FUNC(x) x
#endif

int FUNC(sprintf)(char *str, const char *fmt, ...) {
    int err;

    va_list ap;
    va_start(ap, fmt);
    err = FUNC(vsprintf)(str, fmt, ap);
    va_end(ap);

    return err;
}

int FUNC(snprintf)(char *str, size_t len, const char *fmt, ...) {
    int err;

    va_list ap;
    va_start(ap, fmt);
    err = FUNC(vsnprintf)(str, len, fmt, ap);
    va_end(ap);

    return err;
}

int FUNC(vsprintf)(char *str, const char *fmt, va_list ap) {
    return FUNC(vsnprintf)(str, INT_MAX, fmt, ap);
}

struct _output_args {
    char *outstr;
    size_t len;
    size_t pos;
};

static int _vsnprintf_output(const char *str, size_t len, void *state) {
    struct _output_args *args = state;

    size_t count = 0;
    while (count < len) {
        if (args->pos < args->len) {
            args->outstr[args->pos++] = *str;
        }

        str++;
        count++;
    }

    // Return the count of the number of bytes that would be written even if the buffer
    // wasn't large enough.
    return (int)count;
}

int FUNC(vsnprintf)(char *str, size_t len, const char *fmt, va_list ap) {
    struct _output_args args;
    int wlen;

    args.outstr = str;
    args.len = len;
    args.pos = 0;

    wlen = FUNC(_printf_engine)(&_vsnprintf_output, (void *)&args, fmt, ap);
    if (len == 0) {
        // do nothing, we can't null terminate the output
    } else if (args.pos >= len) {
        str[len - 1] = '\0';
    } else {
        str[wlen] = '\0';
    }
    return wlen;
}

int FUNC(vfprintf)(FILE *fp, const char *fmt, va_list ap) {
    return FUNC(_printf_engine)(&_fprintf_output_func, (void *)fp, fmt, ap);
}

int FUNC(fprintf)(FILE *fp, const char *fmt, ...) {
    va_list ap;
    int err;

    va_start(ap, fmt);
    err = FUNC(vfprintf)(fp, fmt, ap);
    va_end(ap);
    return err;
}

#if !DISABLE_DEBUG_OUTPUT
int FUNC(printf)(const char *fmt, ...) {
    va_list ap;
    int err;

    va_start(ap, fmt);
    err = FUNC(vfprintf)(stdout, fmt, ap);
    va_end(ap);

    return err;
}

int FUNC(vprintf)(const char *fmt, va_list ap) {
    return FUNC(vfprintf)(stdout, fmt, ap);
}
#endif // !DISABLE_DEBUG_OUTPUT

int FUNC(_printf_engine)(_printf_engine_output_func out, void *state, const char *fmt, va_list ap) {
    int err = 0;
    char c;
    unsigned char uc;
    const char *s;
    size_t string_len;
    unsigned long long n;
    void *ptr;
    int flags;
    unsigned int format_num;
    unsigned int precision_num;
    char signchar;
    size_t chars_written = 0;
    char num_buffer[32];

#define OUTPUT_STRING(str, len)                                                                    \
    do {                                                                                           \
        err = out(str, len, state);                                                                \
        if (err < 0) {                                                                             \
            goto exit;                                                                             \
        } else {                                                                                   \
            chars_written += err;                                                                  \
        }                                                                                          \
    } while (0)
#define OUTPUT_CHAR(c)                                                                             \
    do {                                                                                           \
        char __temp[1] = { c };                                                                    \
        OUTPUT_STRING(__temp, 1);                                                                  \
    } while (0)

    for (;;) {
        /* reset the format state */
        n = 0;
        flags = 0;
        format_num = 0;
        precision_num = 0;
        signchar = '\0';

        /* handle regular chars that aren't format related */
        s = fmt;
        string_len = 0;
        while ((c = *fmt++) != 0) {
            if (c == '%') {
                break; /* we saw a '%', break and start parsing format */
            }
            string_len++;
        }

        /* output the string we've accumulated */
        OUTPUT_STRING(s, string_len);

        /* make sure we haven't just hit the end of the string */
        if (c == 0) {
            break;
        }

next_format:
        /* grab the next format character */
        c = *fmt++;
        if (c == 0) {
            break;
        }

        switch (c) {
            case '0' ... '9':
                if (flags & HAS_PRECISION) {
                    precision_num *= 10;
                    precision_num += c - '0';
                } else {
                    if (c == '0' && format_num == 0) {
                        flags |= LEADZEROFLAG;
                    }
                    format_num *= 10;
                    format_num += c - '0';
                }
                goto next_format;
            case '.':
                flags |= HAS_PRECISION;
                precision_num = 0;
                goto next_format;
            case '*':
                if (flags & HAS_PRECISION) {
                    int val = va_arg(ap, int);
                    if (val < 0) {
                        flags &= ~HAS_PRECISION;
                    } else {
                        precision_num = val;
                    }
                } else {
                    int val = va_arg(ap, int);
                    if (val < 0) {
                        flags |= LEFTFORMATFLAG;
                        format_num = -val;
                    } else {
                        format_num = val;
                    }
                }
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
                if (s == 0) {
                    s = "<null>";
                }
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
                if (flags & LONGFLAG) {
                    flags |= LONGLONGFLAG;
                }
                flags |= LONGFLAG;
                goto next_format;
            case 'h':
                if (flags & HALFFLAG) {
                    flags |= HALFHALFFLAG;
                }
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
                n = (flags & LONGLONGFLAG)   ? va_arg(ap, long long)
                    : (flags & LONGFLAG)     ? va_arg(ap, long)
                    : (flags & HALFHALFFLAG) ? (signed char)va_arg(ap, int)
                    : (flags & HALFFLAG)     ? (short)va_arg(ap, int)
                    : (flags & SIZETFLAG)    ? va_arg(ap, ssize_t)
                    : (flags & INTMAXFLAG)   ? va_arg(ap, intmax_t)
                    : (flags & PTRDIFFFLAG)  ? va_arg(ap, ptrdiff_t)
                                             : va_arg(ap, int);
                flags |= SIGNEDFLAG;
                s = _printf_longlong_to_string(num_buffer, n, sizeof(num_buffer), flags, &signchar);
                goto _output_string;
            case 'u':
                n = (flags & LONGLONGFLAG)   ? va_arg(ap, unsigned long long)
                    : (flags & LONGFLAG)     ? va_arg(ap, unsigned long)
                    : (flags & HALFHALFFLAG) ? (unsigned char)va_arg(ap, unsigned int)
                    : (flags & HALFFLAG)     ? (unsigned short)va_arg(ap, unsigned int)
                    : (flags & SIZETFLAG)    ? va_arg(ap, size_t)
                    : (flags & INTMAXFLAG)   ? va_arg(ap, uintmax_t)
                    : (flags & PTRDIFFFLAG)  ? (uintptr_t)va_arg(ap, ptrdiff_t)
                                             : va_arg(ap, unsigned int);
                s = _printf_longlong_to_string(num_buffer, n, sizeof(num_buffer), flags, &signchar);
                goto _output_string;
            case 'p':
                flags |= SIZETFLAG | ALTFLAG;
                goto hex;
            case 'X':
                flags |= CAPSFLAG;
                /* fallthrough */
hex:
            case 'x':
                n = (flags & LONGLONGFLAG)   ? va_arg(ap, unsigned long long)
                    : (flags & LONGFLAG)     ? va_arg(ap, unsigned long)
                    : (flags & HALFHALFFLAG) ? (unsigned char)va_arg(ap, unsigned int)
                    : (flags & HALFFLAG)     ? (unsigned short)va_arg(ap, unsigned int)
                    : (flags & SIZETFLAG)    ? va_arg(ap, size_t)
                    : (flags & INTMAXFLAG)   ? va_arg(ap, uintmax_t)
                    : (flags & PTRDIFFFLAG)  ? (uintptr_t)va_arg(ap, ptrdiff_t)
                                             : va_arg(ap, unsigned int);
                s = _printf_longlong_to_hexstring(num_buffer, n, sizeof(num_buffer), flags);

                /* Normalize c, since code in _output_string needs to know that this is printing hex
                 */
                c = 'x';

                /* Altflag processing should be bypassed when n == 0 so that 0x is not prepended to
                 * it */
                if (n == 0) {
                    flags &= ~ALTFLAG;
                }

                goto _output_string;
            case 'n':
                ptr = va_arg(ap, void *);
                if (flags & LONGLONGFLAG) {
                    *(long long *)ptr = chars_written;
                } else if (flags & LONGFLAG) {
                    *(long *)ptr = chars_written;
                } else if (flags & HALFHALFFLAG) {
                    *(signed char *)ptr = chars_written;
                } else if (flags & HALFFLAG) {
                    *(short *)ptr = chars_written;
                } else if (flags & SIZETFLAG) {
                    *(size_t *)ptr = chars_written;
                } else {
                    *(int *)ptr = chars_written;
                }
                break;
#if FLOAT_PRINTF
            case 'F':
                flags |= CAPSFLAG;
            /* fallthrough */
            case 'f': {
                union {
                    double d;
                    uint64_t i;
                } u;
                u.d = va_arg(ap, double);
                s = _printf_double_to_string(num_buffer, sizeof(num_buffer), u.i, flags);
                goto _output_string;
            }
            case 'A':
                flags |= CAPSFLAG;
            /* fallthrough */
            case 'a': {
                union {
                    double d;
                    uint64_t i;
                } u;
                u.d = va_arg(ap, double);
                s = _printf_double_to_hexstring(num_buffer, sizeof(num_buffer), u.i, flags);
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
        uint precision_fill = 0;
        if (c == 's') {
            if (flags & HAS_PRECISION) {
                string_len = strnlen(s, precision_num);
            }
        } else if (c == 'd' || c == 'i' || c == 'u' || c == 'x') {
            if (flags & HAS_PRECISION) {
                flags &= ~LEADZEROFLAG;
                if (precision_num == 0 && n == 0) {
                    string_len = 0;
                } else if (string_len < precision_num) {
                    precision_fill = precision_num - string_len;
                    string_len = precision_num;
                }
            }
        }

        if (flags & LEFTFORMATFLAG) {
            /* left justify the text */
            if (signchar != '\0') {
                OUTPUT_CHAR(signchar);
                if (format_num > 0) {
                    format_num--;
                }
            }

            if (c == 'x' && (flags & ALTFLAG)) {
                OUTPUT_CHAR('0');
                OUTPUT_CHAR(flags & CAPSFLAG ? 'X' : 'x');
                if (format_num >= 2) {
                    format_num -= 2;
                }
            }

            for (uint i = 0; i < precision_fill; i++) {
                OUTPUT_CHAR('0');
            }
            OUTPUT_STRING(s, string_len - precision_fill);
            uint written = string_len;

            /* pad to the right (if necessary) */
            for (; format_num > written; format_num--) {
                OUTPUT_CHAR(' ');
            }
        } else {
            /* right justify the text (digits) */

            /* if we're going to print a sign digit,
               it'll chew up one byte of the format size */
            if (signchar != '\0' && format_num > 0) {
                format_num--;
            }

            /* output the sign char before the leading zeros */
            if (flags & LEADZEROFLAG && signchar != '\0') {
                OUTPUT_CHAR(signchar);
            }

            /* Handle (altflag) printing 0x before the number */
            /* Note that this needs to be done before padding the number */
            if (c == 'x' && (flags & ALTFLAG)) {
                OUTPUT_CHAR('0');
                OUTPUT_CHAR(flags & CAPSFLAG ? 'X' : 'x');
                /* Width is adjusted so i.e printf("%#04x", 0x02) -> 0x02 instead of 0x0002 */
                if (format_num >= 2) {
                    format_num -= 2;
                }
            }

            /* pad according to the format string */
            for (; format_num > string_len; format_num--) {
                OUTPUT_CHAR(flags & LEADZEROFLAG ? '0' : ' ');
            }

            /* if not leading zeros, output the sign char just before the number */
            if (!(flags & LEADZEROFLAG) && signchar != '\0') {
                OUTPUT_CHAR(signchar);
            }

            /* output the string */
            for (uint i = 0; i < precision_fill; i++) {
                OUTPUT_CHAR('0');
            }
            OUTPUT_STRING(s, string_len - precision_fill);
        }
    }

#undef OUTPUT_STRING
#undef OUTPUT_CHAR

exit:
    return (err < 0) ? err : (int)chars_written;
}
