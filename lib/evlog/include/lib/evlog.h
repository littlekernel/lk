/*
 * Copyright (c) 2012 Travis Geiselbrecht
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
#ifndef __LIB_EVLOG_H
#define __LIB_EVLOG_H

#include <inttypes.h>
#include <sys/types.h>

typedef struct evlog {
	uint head;
	uint unitsize;
	uint len_pow2;
	uintptr_t *items;
} evlog_t;

status_t evlog_init_etc(evlog_t *e, uint len, uint unitsize, uintptr_t *items);
status_t evlog_init(evlog_t *e, uint len, uint unitsize);

/* callback to evlog_dump. */
typedef void (*evlog_dump_cb)(const uintptr_t *);

void evlog_dump(evlog_t *e, evlog_dump_cb cb);

/* bump the head pointer and return the old one.
 */
uint evlog_bump_head(evlog_t *e);

/*
 * It's assumed you're following a pattern similar to the following:
 *
void evlog_add2(evlog_t *e, uintptr_t a, uintptr_t b)
{
	uint index = evlog_bump_head(e);

	e->items[index] = a;
	e->items[index + 1] = b;
}
*/

#endif

