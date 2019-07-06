/*
 * Copyright (c) 2012 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/debug.h>
#include <assert.h>
#include <lk/err.h>
#include <lk/pow2.h>
#include <stdlib.h>
#include <lib/evlog.h>

#define INCPTR(e, ptr, inc) \
    modpow2((ptr) + (inc), (e)->len_pow2)

status_t evlog_init_etc(evlog_t *e, uint len, uint unitsize, uintptr_t *items) {
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

status_t evlog_init(evlog_t *e, uint len, uint unitsize) {
    uintptr_t *items = calloc(1, len * sizeof(uintptr_t));
    if (!items) {
        return ERR_NO_MEMORY;
    }

    status_t err = evlog_init_etc(e, len, unitsize, items);
    if (err < 0)
        free(items);
    return err;
}

uint evlog_bump_head(evlog_t *e) {
    uint index = e->head;
    e->head = INCPTR(e, e->head, e->unitsize);

    return index;
}

void evlog_dump(evlog_t *e, evlog_dump_cb cb) {
    for (uint index = INCPTR(e, e->head, e->unitsize); index != e->head; index = INCPTR(e, index, e->unitsize)) {
        cb(&e->items[index]);
    }
}


