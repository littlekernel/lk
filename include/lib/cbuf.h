/*
 * Copyright (c) 2009-2013 Travis Geiselbrecht
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
#ifndef __LIB_CBUF_H
#define __LIB_CBUF_H

#include <sys/types.h>
#include <kernel/event.h>

typedef struct cbuf {
	uint head;
	uint tail;
	uint len_pow2;
	char *buf;
	event_t event;
} cbuf_t;

void cbuf_initialize(cbuf_t *cbuf, size_t len);
size_t cbuf_read(cbuf_t *cbuf, void *_buf, size_t buflen, bool block);
size_t cbuf_write(cbuf_t *cbuf, const void *_buf, size_t len, bool canreschedule);
size_t cbuf_space_avail(cbuf_t *cbuf);

/* special cases for dealing with a single char of data */
size_t cbuf_read_char(cbuf_t *cbuf, char *c, bool block);
size_t cbuf_write_char(cbuf_t *cbuf, char c, bool canreschedule);

#endif

