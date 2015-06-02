/*
 * Copyright (c) 2013 Google, Inc.
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
#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <sys/types.h>
#include <iovec.h>
#include <compiler.h>
#include <stdarg.h>

#if WITH_LIB_KLOG

#define KLOG_CURRENT_BUFFER -1

void klog_init(void);

ssize_t klog_recover(void *ptr);
status_t klog_create(void *ptr, size_t len, uint count);

uint klog_buffer_count(void);
uint klog_current_buffer(void);
status_t klog_set_current_buffer(uint buffer);

/* dump the klog to the console, -1 is current buffer */
void klog_dump(int buffer);

/*
 * Fill in an iovec that points to the requested buffer, -1 is current buffer.
 * The buffer may be in 2 pieces, due to the internal circular buffer.
 * Return is number of iovec runs.
 */
int klog_get_buffer(int buffer, iovec_t *vec);

/*
 * Read functions actively remove data from the klog on read
 */
ssize_t klog_read(char *buf, size_t len, int buf_id);
char klog_getc(int buf_id);
char klog_getchar(void);
bool klog_has_data(void);

void klog_putchar(char c);
void klog_puts(const char *str);
void klog_printf(const char *fmt, ...) __PRINTFLIKE(1, 2);
void klog_vprintf(const char *fmt, va_list ap);

#else

/* if klog is not present, stub out the input routines */
static inline void klog_putc(char c) {}
static inline void klog_puts(const char *str) {}
static inline void klog_printf(const char *fmt, ...) {}
static inline void klog_vprintf(const char *fmt, va_list ap) {}

#endif
