/*
 * Copyright (c) 2015 Travis Geiselbrecht
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

#if WITH_LIB_MINIP
#include <app.h>

#include <platform.h>
#include <stdio.h>
#include <debug.h>
#include <string.h>
#include <pow2.h>
#include <err.h>
#include <assert.h>
#include <trace.h>

#include <app/lkboot.h>

#include "lkboot.h"

#include <lib/minip.h>

#define LOCAL_TRACE 0

static ssize_t tcp_readx(void *s, void *_data, size_t len) {
    char *data = _data;
    while (len > 0) {
        int r = tcp_read(s, data, len);
        if (r <= 0) return -1;
        data += r;
        len -= r;
    }
    return 0;
}

lkb_t *lkboot_tcp_opened(void *s)
{
    lkb_t *lkb;

    lkb = lkboot_create_lkb(s, tcp_readx, (void *)tcp_write);
    if (!lkb)
        return NULL;

    return lkb;
}

#endif
