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
#include <debug.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <platform/debug.h>

int fputc(int c, FILE *fp)
{
	return fp->fputc(fp->ctx, c);
}

int putchar(int c)
{
	return fputc(c, stdout);
}

int puts(const char *str)
{
	int err = fputs(str, stdout);
	if (err >= 0)
		err = fputc('\n', stdout);
	return err;
}

int fputs(const char *s, FILE *fp)
{
	return fp->fputs(fp->ctx, s);
}

int getc(FILE *fp)
{
	return fp->fgetc(fp->ctx);
}

int getchar(void)
{
	return getc(stdin);
}

int vfprintf(FILE *fp, const char *fmt, va_list ap)
{
	return fp->vfprintf(fp->ctx, fmt, ap);
}

int fprintf(FILE *fp, const char *fmt, ...)
{
	va_list ap;
	int err;

	va_start(ap, fmt);
	err = vfprintf(fp, fmt, ap);
	va_end(ap);
	return err;
}

int _printf(const char *fmt, ...)
{
	va_list ap;
	int err;

	va_start(ap, fmt);
	err = vfprintf(stdout, fmt, ap);
	va_end(ap);

	return err;
}
