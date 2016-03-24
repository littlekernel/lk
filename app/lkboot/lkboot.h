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
#pragma once

#include <sys/types.h>
#include <app/lkboot.h>
#include "lkboot_protocol.h"

/* private to lkboot app */

int lkb_handle_command(lkb_t *lkb, const char *cmd, const char *arg, size_t len, const char **result);

status_t do_flash_boot(void);

typedef ssize_t lkb_read_hook(void *s, void *data, size_t len);
typedef ssize_t lkb_write_hook(void *s, const void *data, size_t len);

lkb_t *lkboot_create_lkb(void *cookie, lkb_read_hook *read, lkb_write_hook *write);
status_t lkboot_process_command(lkb_t *);

/* inet server */
lkb_t *lkboot_tcp_opened(void *s);

/* dcc based server */
void lkboot_dcc_init(void);
lkb_t *lkboot_check_dcc_open(void);

