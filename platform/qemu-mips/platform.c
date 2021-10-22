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
#include <platform/qemu-mips.h>
#include <arch/mips.h>
#include <dev/uart.h>

extern void platform_init_interrupts(void);
extern void platform_init_uart(void);
extern void uart_init(void);

void platform_early_init(void) {
    platform_init_interrupts();
    uart_init_early();

    mips_init_timer(100000000);
    mips_enable_irq(2);
}

void platform_init(void) {
    uart_init();
}

