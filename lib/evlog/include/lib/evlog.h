/*
 * Copyright (c) 2012 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

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

