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
#ifndef __LIB_PRINTF_H
#define __LIB_PRINTF_H

#include <stdarg.h>
#include <compiler.h>
#include <debug.h>
#include <stddef.h>

__BEGIN_CDECLS

#if !DISABLE_DEBUG_OUTPUT
#define printf(x...) _printf(x)
#else
static inline int __PRINTFLIKE(1, 2) printf(const char *fmt, ...) { return 0; }
#endif

int _printf(const char *fmt, ...) __PRINTFLIKE(1, 2);
int sprintf(char *str, const char *fmt, ...) __PRINTFLIKE(2, 3);
int snprintf(char *str, size_t len, const char *fmt, ...) __PRINTFLIKE(3, 4);
int vsprintf(char *str, const char *fmt, va_list ap);
int vsnprintf(char *str, size_t len, const char *fmt, va_list ap);

/* printf engine that parses the format string and generates output */

/* function pointer to pass the engine,
 * return code is remaining characters in destination (or INT_MAX for infinity)
 */
typedef int (*_printf_engine_output_func)(char c, void *state);

int _printf_engine(_printf_engine_output_func out, void *state, const char *fmt, va_list ap);

__END_CDECLS

#endif
