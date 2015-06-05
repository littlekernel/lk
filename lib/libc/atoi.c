/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
/*
 * Copyright (c) 2008 Travis Geiselbrecht
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

#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#define LONG_IS_INT 1

static int hexval(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	else if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	else if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;

	return 0;
}

int atoi(const char *num)
{
#if !LONG_IS_INT
	// XXX fail
#else
	return atol(num);
#endif
}

unsigned int atoui(const char *num)
{
#if !LONG_IS_INT
	// XXX fail
#else
	return atoul(num);
#endif
}

long atol(const char *num)
{
	long value = 0;
	int neg = 0;

	if (num[0] == '0' && num[1] == 'x') {
		// hex
		num += 2;
		while (*num && isxdigit(*num))
			value = value * 16 + hexval(*num++);
	} else {
		// decimal
		if (num[0] == '-') {
			neg = 1;
			num++;
		}
		while (*num && isdigit(*num))
			value = value * 10 + *num++  - '0';
	}

	if (neg)
		value = -value;

	return value;
}

unsigned long atoul(const char *num)
{
	unsigned long value = 0;
	if (num[0] == '0' && num[1] == 'x') {
		// hex
		num += 2;
		while (*num && isxdigit(*num))
			value = value * 16 + hexval(*num++);
	} else {
		// decimal
		while (*num && isdigit(*num))
			value = value * 10 + *num++  - '0';
	}

	return value;
}

unsigned long long atoull(const char *num)
{
	unsigned long long value = 0;
	if (num[0] == '0' && num[1] == 'x') {
		// hex
		num += 2;
		while (*num && isxdigit(*num))
			value = value * 16 + hexval(*num++);
	} else {
		// decimal
		while (*num && isdigit(*num))
			value = value * 10 + *num++  - '0';
	}

	return value;
}

unsigned long strtoul(const char *nptr, char **endptr, int base) {
	int neg = 0;
	unsigned long ret = 0;

	if (base < 0 || base == 1 || base > 36) {
		errno = EINVAL;
		return 0;
	}

	while (isspace(*nptr)) {
		nptr++;
	}

	if (*nptr == '+') {
		nptr++;
	} else if (*nptr == '-') {
		neg = 1;
		nptr++;
	}

	if ((base == 0 || base == 16) && nptr[0] == '0' && nptr[1] == 'x') {
		base = 16;
		nptr += 2;
	} else if (base == 0 && nptr[0] == '0') {
		base = 8;
		nptr++;
	} else if (base == 0) {
		base = 10;
	}

	for (;;) {
		char c = *nptr;
		int v = -1;
		unsigned long new_ret;

		if (c >= 'A' && c <= 'Z') {
			v = c - 'A' + 10;
		} else if (c >= 'a' && c <= 'z') {
			v = c - 'a' + 10;
		} else if (c >= '0' && c <= '9') {
			v = c - '0';
		}

		if (v < 0 || v >= base) {
			*endptr = (char *) nptr;
			break;
		}

		new_ret = ret * base;
		if (new_ret / base != ret ||
		    new_ret + v < new_ret ||
		    ret == ULONG_MAX) {
			ret = ULONG_MAX;
			errno = ERANGE;
		} else {
			ret = new_ret + v;
		}

		nptr++;
	}

	if (neg && ret != ULONG_MAX) {
		ret = -ret;
	}

	return ret;
}
