/*
 * Copyright (c) 2008-2013 Travis Geiselbrecht
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
#include <limits.h>
#include <printf.h>
#include <stdarg.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <platform/debug.h>

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

static char *longlong_to_string(char *buf, unsigned long long n, int len, uint flag, char *signchar)
{
	int pos = len;
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

static char *longlong_to_hexstring(char *buf, unsigned long long u, int len, uint flag)
{
	int pos = len;
	static const char hextable[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
	static const char hextable_caps[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
	const char *table;

	if ((flag & CAPSFLAG))
		table = hextable_caps;
	else
		table = hextable;

	buf[--pos] = 0;
	do {
		unsigned int digit = u % 16;
		u /= 16;

		buf[--pos] = table[digit];
	} while (u != 0);

	return &buf[pos];
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
	while (count < len && *str) {
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
			default:
				OUTPUT_CHAR('%');
				OUTPUT_CHAR(c);
				break;
		}

		/* move on to the next field */
		continue;

		/* shared output code */
_output_string:
		if (flags & LEFTFORMATFLAG) {
			/* left justify the text */
			OUTPUT_STRING(s, SIZE_MAX);
			uint written = err;

			/* pad to the right (if necessary) */
			for (; format_num > written; format_num--)
				OUTPUT_CHAR(' ');
		} else {
			/* right justify the text (digits) */
			string_len = strlen(s);

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
			OUTPUT_STRING(s, SIZE_MAX);
		}
		continue;
	}

#undef OUTPUT_STRING
#undef OUTPUT_CHAR

exit:
	return (err < 0) ? err : (int)chars_written;
}


