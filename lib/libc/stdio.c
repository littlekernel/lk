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
#include <string.h>
#include <sys/types.h>
#include <platform/debug.h>

#define DEFINE_STDIO_DESC(id)   \
    [(id)]  = {                 \
        .io = &console_io,      \
    }

FILE __stdio_FILEs[3] = {
    DEFINE_STDIO_DESC(0), /* stdin */
    DEFINE_STDIO_DESC(1), /* stdout */
    DEFINE_STDIO_DESC(2), /* stderr */
};
#undef DEFINE_STDIO_DESC

int fputc(int _c, FILE *fp)
{
    unsigned char c = _c;
    return io_write(fp->io, (char *)&c, 1);
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
    size_t len = strlen(s);

    return io_write(fp->io, s, len);
}

size_t fwrite(const void *ptr, size_t size, size_t count, FILE *fp)
{
    size_t bytes_written;

    if (size == 0 || count == 0)
        return 0;

    // fast path for size == 1
    if (likely(size == 1)) {
        return io_write(fp->io, ptr, count);
    }

    bytes_written = io_write(fp->io, ptr, size * count);
    return bytes_written / size;
}

int getc(FILE *fp)
{
    char c;
    ssize_t ret = io_read(fp->io, &c, sizeof(c));

    return (ret > 0) ? c : ret;
}

int getchar(void)
{
    return getc(stdin);
}

static int _fprintf_output_func(const char *str, size_t len, void *state)
{
    FILE *fp = (FILE *)state;

    return io_write(fp->io, str, len);
}

int vfprintf(FILE *fp, const char *fmt, va_list ap)
{
    return _printf_engine(&_fprintf_output_func, (void *)fp, fmt, ap);
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

int _vprintf(const char *fmt, va_list ap)
{
    return vfprintf(stdout, fmt, ap);
}
