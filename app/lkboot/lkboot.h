/*
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
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

