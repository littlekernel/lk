/*
 * Copyright (c) 2014 Brian Swetland
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#pragma once

typedef struct LKB lkb_t;

// lkb_read/write may *only* be called from within a lkb_handler()
// len of 0 is invalid for both

// returns 0 on success, -1 on failure (io error, etc)
int lkb_read(lkb_t *lkb, void *data, size_t len);
int lkb_write(lkb_t *lkb, const void *data, size_t len);

// len is the number of bytes the host has declared that it will send
// use lkb_read() to read some or all of this data
// return NULL on success, or an asciiz string (message) for error
typedef const char *(*lkb_handler_t)(lkb_t *lkb,
                                     const char *arg, unsigned len, void *cookie);

// cmd must be a string constant
void lkb_register(const char *cmd, lkb_handler_t handler, void *cookie);

