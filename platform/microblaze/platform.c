/*
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/reg.h>
#include <kernel/thread.h>
#include <platform.h>
#include <platform/interrupts.h>
#include <platform/debug.h>
#include <platform/timer.h>
#include <sys/types.h>
#include <target/microblaze-config.h>

#include "uartlite.h"

void platform_dputc(char c) {
    if (c == '\n')
        uartlite_putc('\r');
    uartlite_putc(c);
}

int platform_dgetc(char *c, bool wait) {
    for (;;) {
        int ret = uartlite_getc(wait);
        if (ret >= 0) {
            *c = ret;
            return 0;
        }

        if (!wait)
            return -1;

        thread_yield();
    }
}


