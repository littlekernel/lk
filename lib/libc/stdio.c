/*
 * Copyright (c) 2013 Travis Geiselbrecht
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
#include <stdio.h>
#include <debug.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <err.h>
#include <sys/types.h>
#include <platform/debug.h>
#include <kernel/thread.h>

int fputc(int c, FILE *fp)
{
	DEBUG_ASSERT_PRINTCALLER(!in_critical_section());

	if (!fp->write)
		return 0;

	return fp->write(fp->ctx, &c, 1);
}

int putchar(int c)
{
	DEBUG_ASSERT_PRINTCALLER(!in_critical_section());

	return fputc(c, stdout);
}

int puts(const char *str)
{
	DEBUG_ASSERT_PRINTCALLER(!in_critical_section());

	int err = fputs(str, stdout);
	if (err >= 0)
		err = fputc('\n', stdout);
	return err;
}

int fputs(const char *s, FILE *fp)
{
	DEBUG_ASSERT_PRINTCALLER(!in_critical_section());

	if (!fp->write)
		return 0;

	size_t len = strlen(s);

	return fp->write(fp->ctx, s, len);
}

int getc(FILE *fp)
{
	DEBUG_ASSERT_PRINTCALLER(!in_critical_section());

	unsigned char c;

	if (!fp->read)
		return 0;

	ssize_t len = fp->read(fp->ctx, &c, 1, 0);
	if (len == 1)
		return c;
	else if (len == 0)
		return ERR_IO;
	else
		return len;
}

int getchar(void)
{
	DEBUG_ASSERT_PRINTCALLER(!in_critical_section());

	return getc(stdin);
}

static int _FILE_printf_output_routine(char c, void *state)
{
	FILE *fp = (FILE *)state;

	fputc(c, fp);

	return INT_MAX;
}

int vfprintf(FILE *fp, const char *fmt, va_list ap)
{
	DEBUG_ASSERT_PRINTCALLER(!in_critical_section());

	return _printf_engine(&_FILE_printf_output_routine, (void *)fp, fmt, ap);
}

int fprintf(FILE *fp, const char *fmt, ...)
{
	va_list ap;
	int err;

	DEBUG_ASSERT_PRINTCALLER(!in_critical_section());

	va_start(ap, fmt);
	err = vfprintf(fp, fmt, ap);
	va_end(ap);
	return err;
}

int _printf(const char *fmt, ...)
{
	va_list ap;
	int err;

	DEBUG_ASSERT_PRINTCALLER(!in_critical_section());

	va_start(ap, fmt);
	err = vfprintf(stdout, fmt, ap);
	va_end(ap);

	return err;
}

/* default stdio output target */
ssize_t __debug_stdio_write(void *ctx, const void *_ptr, size_t len)
{
	const char *ptr = _ptr;

	for (size_t i = 0; i < len; i++)
		platform_dputc(ptr[i]);

	return len;
}

ssize_t __debug_stdio_read(void *ctx, void *ptr, size_t len, unsigned int flags)
{
	int err;

	err = platform_dgetc(ptr, (flags & __FILE_READ_NONBLOCK) ? false : true);
	if (err < 0)
		return err;

	return 1;
}

#define DEFINE_STDIO_DESC(id)						\
	[(id)]	= {							\
		.ctx		= &__stdio_FILEs[(id)],			\
		.write		= __debug_stdio_write,			\
		.read		= __debug_stdio_read,			\
	}

FILE __stdio_FILEs[3] = {
	DEFINE_STDIO_DESC(0), /* stdin */
	DEFINE_STDIO_DESC(1), /* stdout */
	DEFINE_STDIO_DESC(2), /* stderr */
};
#undef DEFINE_STDIO_DESC

/* vim: set ts=4 sw=4 noexpandtab: */
