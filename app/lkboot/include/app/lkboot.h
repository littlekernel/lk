/*
 * Copyright (c) 2014 Brian Swetland
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

typedef struct LKB lkb_t;

// lkb_read/write may *only* be called from within a lkb_handler()
// len of 0 is invalid for both

// returns 0 on success, -1 on failure (io error, etc)
int lkb_read(lkb_t *lkb, void *data, size_t len);
int lkb_write(lkb_t *lkb, const void *data, size_t len);

// len is the number of bytes the host has declared that it will send
// use lkb_read() to read some or all of this data
// return NULL on success, or an asciiz string (message) for error
typedef const char* (*lkb_handler_t)(lkb_t *lkb,
    const char *arg, unsigned len, void *cookie);

// cmd must be a string constant
void lkb_register(const char *cmd, lkb_handler_t handler, void *cookie);

