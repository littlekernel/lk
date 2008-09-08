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
	printf("long:  %ld %ld %ld\n", -12345678, 0, 12345678);
	printf("ulong: %lu %lu %lu\n", -12345678, 0, 12345678);
	printf("long:  %D %D %D\n", -12345678, 0, 12345678);
	printf("ulong: %U %U %U\n", -12345678, 0, 12345678);
	printf("longlong: %lli %lli %lli\n", -12345678LL, 0LL, 12345678LL);
	printf("ulonglong: %llu %llu %llu\n", -12345678LL, 0LL, 12345678LL);
	printf("size_t: %zd %zd %zd\n", -12345678, 0, 12345678);
	printf("usize_t: %zu %zu %zu\n", -12345678, 0, 12345678);

	printf("hex:\n");
	printf("uint8: %hhx %hhx %hhx\n", -12, 0, 254);
	printf("uint16:%hx %hx %hx\n", -1234, 0, 1234);
	printf("uint:  %x %x %x\n", -12345678, 0, 12345678);
	printf("ulong: %lx %lx %lx\n", -12345678, 0, 12345678);
	printf("ulong: %X %X %X\n", -12345678, 0, 12345678);
	printf("ulonglong: %llx %llx %llx\n", -12345678LL, 0LL, 12345678LL);
	printf("usize_t: %zx %zx %zx\n", -12345678, 0, 12345678);

	printf("alt/sign:\n");
	printf("uint: %#x %#X\n", 0xabcdef, 0xabcdef);
	printf("int: %+d %+d\n", 12345678, -12345678);

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
}


