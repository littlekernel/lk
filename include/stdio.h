/*
 * Copyright (c) 2008-2015 Travis Geiselbrecht
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
#ifndef __STDIO_H
#define __STDIO_H

#include <compiler.h>
#include <printf.h>
#include <sys/types.h>
#include <lib/io.h>

__BEGIN_CDECLS

typedef struct FILE {
    io_handle_t *io;
} FILE;

extern FILE __stdio_FILEs[];

#define stdin  (&__stdio_FILEs[0])
#define stdout (&__stdio_FILEs[1])
#define stderr (&__stdio_FILEs[2])

FILE *fopen(const char *filename, const char *mode);
int fclose(FILE *stream);
size_t fread(void *ptr, size_t size, size_t count, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t count, FILE *stream);
int fflush(FILE *stream);
int feof(FILE *stream);

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

int fseek(FILE *stream, long offset, int whence);
long ftell(FILE *stream);

int fputc(int c, FILE *fp);
#define putc(c, fp) fputc(c, fp)
int putchar(int c);

int fputs(const char *s, FILE *fp);
int puts(const char *str);

int getc(FILE *fp);
int getchar(void);

#if !DISABLE_DEBUG_OUTPUT
#define printf(x...) _printf(x)
#define vprintf(x...) _vprintf(x)
#else
static inline int __PRINTFLIKE(1, 2) printf(const char *fmt, ...) { return 0; }
static inline int vprintf(const char *fmt, va_list ap) { return 0; }
#endif

int _printf(const char *fmt, ...) __PRINTFLIKE(1, 2);
int _vprintf(const char *fmt, va_list ap);

int fprintf(FILE *fp, const char *fmt, ...) __PRINTFLIKE(2, 3);
int vfprintf(FILE *fp, const char *fmt, va_list ap);

int sprintf(char *str, const char *fmt, ...) __PRINTFLIKE(2, 3);
int snprintf(char *str, size_t len, const char *fmt, ...) __PRINTFLIKE(3, 4);
int vsprintf(char *str, const char *fmt, va_list ap);
int vsnprintf(char *str, size_t len, const char *fmt, va_list ap);


__END_CDECLS

#endif

