/*
 * Copyright (c) 2009 Corey Tabaka
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lib/cbuf.h>

void platform_init_keyboard(cbuf_t *buffer);

int platform_read_key(char *c);

// Panic time input: reads a key straight from the controller, without the irq or the
// input buffer. ERR_NOT_READY if no key is waiting, or no keyboard was initialized.
int platform_poll_key(char *c);
void platform_i8042_keyboard_reset(void);
