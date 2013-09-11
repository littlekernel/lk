/*
 * Copyright (c) 2008-2012 Travis Geiselbrecht
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

#include <ctype.h>
#include <debug.h>
#include <stdlib.h>
#include <printf.h>
#include <stdio.h>
#include <list.h>
#include <string.h>
#include <arch/ops.h>
#include <platform.h>
#include <platform/debug.h>
#include <kernel/thread.h>

void spin(uint32_t usecs)
{
	lk_bigtime_t start = current_time_hires();

	while ((current_time_hires() - start) < usecs)
		;
}

void halt(void)
{
	enter_critical_section(); // disable ints
	platform_halt();
}

void _panic(void *caller, const char *fmt, ...)
{
	dprintf(ALWAYS, "panic (caller %p): ", caller);

	va_list ap;
	va_start(ap, fmt);
	_dvprintf(fmt, ap);
	va_end(ap);

	halt();
}

static int __debug_stdio_fputc(void *ctx, int c)
{
	_dputc(c);
	return 0;
}

static int __debug_stdio_fputs(void *ctx, const char *s)
{
	return _dputs(s);
}

static int __debug_stdio_fgetc(void *ctx)
{
	char c;
	int err;

	err = platform_dgetc(&c, true);
	if (err < 0)
		return err;
	return (unsigned char)c;
}

static int __debug_stdio_vfprintf(void *ctx, const char *fmt, va_list ap)
{
	return _dvprintf(fmt, ap);
}

#define DEFINE_STDIO_DESC(id)						\
	[(id)]	= {							\
		.ctx		= &__stdio_FILEs[(id)],			\
		.fputc		= __debug_stdio_fputc,			\
		.fputs		= __debug_stdio_fputs,			\
		.fgetc		= __debug_stdio_fgetc,			\
		.vfprintf	= __debug_stdio_vfprintf,		\
	}

FILE __stdio_FILEs[3] = {
	DEFINE_STDIO_DESC(0), /* stdin */
	DEFINE_STDIO_DESC(1), /* stdout */
	DEFINE_STDIO_DESC(2), /* stderr */
};
#undef DEFINE_STDIO_DESC

#if !DISABLE_DEBUG_OUTPUT

int _dputs(const char *str)
{
	while (*str != 0) {
		_dputc(*str++);
	}

	return 0;
}

static int _dprintf_output_func(const char *str, size_t len, void *state)
{
	size_t count = 0;
	while (count < len && *str) {
		_dputc(*str);
		str++;
		count++;
	}

	return count;
}

int _dprintf(const char *fmt, ...)
{
	int err;

	va_list ap;
	va_start(ap, fmt);
	err = _printf_engine(&_dprintf_output_func, NULL, fmt, ap);
	va_end(ap);

	return err;
}

int _dvprintf(const char *fmt, va_list ap)
{
	int err;

	err = _printf_engine(&_dprintf_output_func, NULL, fmt, ap);

	return err;
}

void hexdump(const void *ptr, size_t len)
{
	addr_t address = (addr_t)ptr;
	size_t count;
	int i;

	for (count = 0 ; count < len; count += 16) {
		printf("0x%08lx: ", address);
		printf("%08x %08x %08x %08x |", *(const uint32_t *)address, *(const uint32_t *)(address + 4), *(const uint32_t *)(address + 8), *(const uint32_t *)(address + 12));
		for (i=0; i < 16; i++) {
			char c = *(const char *)(address + i);
			if (isalpha(c)) {
				printf("%c", c);
			} else {
				printf(".");
			}
		}
		printf("|\n");
		address += 16;
	}
}

void hexdump8(const void *ptr, size_t len)
{
	addr_t address = (addr_t)ptr;
	size_t count;
	int i;

	for (count = 0 ; count < len; count += 16) {
		printf("0x%08lx: ", address);
		for (i=0; i < 16; i++) {
			printf("0x%02hhx ", *(const uint8_t *)(address + i));
		}
		printf("\n");
		address += 16;
	}
}

#endif // !DISABLE_DEBUG_OUTPUT

