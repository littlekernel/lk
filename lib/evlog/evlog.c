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
#include <debug.h>
#include <assert.h>
#include <err.h>
#include <pow2.h>
#include <stdlib.h>
#include <lib/evlog.h>

#define INCPTR(e, ptr, inc) \
	modpow2((ptr) + (inc), (e)->len_pow2)

status_t evlog_init_etc(evlog_t *e, uint len, uint unitsize, uintptr_t *items)
{
	if (len < 2 || !ispow2(len)) {
		return ERR_INVALID_ARGS;
	}
	if (unitsize < 1 || !ispow2(unitsize)) {
		return ERR_INVALID_ARGS;
	}
	if (unitsize > len) {
		return ERR_INVALID_ARGS;
	}

	e->head = 0;
	e->unitsize = unitsize;
	e->len_pow2 = log2_uint(len);
	e->items = items;	

	return NO_ERROR;
}

status_t evlog_init(evlog_t *e, uint len, uint unitsize)
{
	uintptr_t *items = calloc(1, len * sizeof(uintptr_t));
	if (!items) {
		return ERR_NO_MEMORY;
	}

	status_t err = evlog_init_etc(e, len, unitsize, items);
	if (err < 0)
		free(items);
	return err;
}

uint evlog_bump_head(evlog_t *e)
{
	uint index = e->head;
	e->head = INCPTR(e, e->head, e->unitsize);

	return index;
}

void evlog_dump(evlog_t *e, evlog_dump_cb cb)
{
	for (uint index = INCPTR(e, e->head, e->unitsize); index != e->head; index = INCPTR(e, index, e->unitsize)) {
		cb(&e->items[index]);
	}
}


