/*
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/reg.h>
#include <sys/types.h>
#include <kernel/thread.h>
#include <platform.h>
#include <platform/interrupts.h>
#include <platform/debug.h>
#include <platform/timer.h>
#include <platform/sifive.h>
#include <dev/interrupt/riscv_plic.h>

#include "platform_p.h"

void platform_early_init(void) {
    gpio_early_init();

    sifive_uart_early_init();

    plic_early_init(PLIC_BASE, SIFIVE_NUM_IRQS, true);
}

void platform_init(void) {
    plic_init();
    gpio_init();
    sifive_uart_init();
}

void platform_dputc(char c) {
    if (c == '\n')
        sifive_uart_write('\r');
    sifive_uart_write(c);
}

int platform_dgetc(char *c, bool wait) {
    return sifive_uart_read(c, wait);
}


