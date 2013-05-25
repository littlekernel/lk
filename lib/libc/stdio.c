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
	_dputc(c);
	return 0;
}

int putchar(int c)
{
	return fputc(c, stdout);
}

int puts(const char *str)
{
	int err = _dputs(str);
	if (err >= 0)
		_dputc('\n');

	return err;
}

int fputs(const char *s, FILE *fp)
{
	return _dputs(s);
}

int getc(FILE *fp)
{
	char c;

	int err = platform_dgetc(&c, true);
	if (err < 0)
		return err;

	return (unsigned char)c;
}

int getchar(void)
{
	return getc(stdin);
}

