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
#include <app/tests.h>
#include <debug.h>
#include <string.h>

void printf_tests(void)
{
	printf("printf tests\n");

	printf("numbers:\n");
	printf("int8:  %hhd %hhd %hhd\n", -12, 0, 254);
	printf("uint8: %hhu %hhu %hhu\n", -12, 0, 254);
	printf("int16: %hd %hd %hd\n", -1234, 0, 1234);
	printf("uint16:%hu %hu %hu\n", -1234, 0, 1234);
	printf("int:   %d %d %d\n", -12345678, 0, 12345678);
	printf("uint:  %u %u %u\n", -12345678, 0, 12345678);
	printf("long:  %ld %ld %ld\n", -12345678L, 0L, 12345678L);
	printf("ulong: %lu %lu %lu\n", -12345678UL, 0UL, 12345678UL);

	printf("longlong: %lli %lli %lli\n", -12345678LL, 0LL, 12345678LL);
	printf("ulonglong: %llu %llu %llu\n", -12345678LL, 0LL, 12345678LL);
	printf("ssize_t: %zd %zd %zd\n", (ssize_t)-12345678, (ssize_t)0, (ssize_t)12345678);
	printf("usize_t: %zu %zu %zu\n", (size_t)-12345678, (size_t)0, (size_t)12345678);
	printf("intmax_t: %jd %jd %jd\n", (intmax_t)-12345678, (intmax_t)0, (intmax_t)12345678);
	printf("uintmax_t: %ju %ju %ju\n", (uintmax_t)-12345678, (uintmax_t)0, (uintmax_t)12345678);
	printf("ptrdiff_t: %td %td %td\n", (ptrdiff_t)-12345678, (ptrdiff_t)0, (ptrdiff_t)12345678);
	printf("ptrdiff_t (u): %tu %tu %tu\n", (ptrdiff_t)-12345678, (ptrdiff_t)0, (ptrdiff_t)12345678);

	printf("hex:\n");
	printf("uint8: %hhx %hhx %hhx\n", -12, 0, 254);
	printf("uint16:%hx %hx %hx\n", -1234, 0, 1234);
	printf("uint:  %x %x %x\n", -12345678, 0, 12345678);
	printf("ulong: %lx %lx %lx\n", -12345678UL, 0UL, 12345678UL);
	printf("ulong: %X %X %X\n", -12345678, 0, 12345678);
	printf("ulonglong: %llx %llx %llx\n", -12345678LL, 0LL, 12345678LL);
	printf("usize_t: %zx %zx %zx\n", (size_t)-12345678, (size_t)0, (size_t)12345678);

	printf("alt/sign:\n");
	printf("uint: %#x %#X\n", 0xabcdef, 0xabcdef);
	printf("int: %+d %+d\n", 12345678, -12345678);
	printf("int: % d %+d\n", 12345678, 12345678);

	printf("formatting\n");
	printf("int: a%8da\n", 12345678);
	printf("int: a%9da\n", 12345678);
	printf("int: a%-9da\n", 12345678);
	printf("int: a%10da\n", 12345678);
	printf("int: a%-10da\n", 12345678);
	printf("int: a%09da\n", 12345678);
	printf("int: a%010da\n", 12345678);
	printf("int: a%6da\n", 12345678);

	printf("a%1sa\n", "b");
	printf("a%9sa\n", "b");
	printf("a%-9sa\n", "b");
	printf("a%5sa\n", "thisisatest");

	int err;

	err = printf("a");
	printf(" returned %d\n", err);
	err = printf("ab");
	printf(" returned %d\n", err);
	err = printf("abc");
	printf(" returned %d\n", err);
	err = printf("abcd");
	printf(" returned %d\n", err);
	err = printf("abcde");
	printf(" returned %d\n", err);
	err = printf("abcdef");
	printf(" returned %d\n", err);

	/* make sure snprintf terminates at the right spot */
	char buf[32];

	memset(buf, 0, sizeof(buf));
	err = sprintf(buf, "0123456789abcdef012345678");
	printf("sprintf returns %d\n", err);
	hexdump8(buf, sizeof(buf));

	memset(buf, 0, sizeof(buf));
	err = snprintf(buf, 15, "0123456789abcdef012345678");
	printf("snprintf returns %d\n", err);
	hexdump8(buf, sizeof(buf));

}


