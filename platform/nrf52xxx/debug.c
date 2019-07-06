/*
 * Copyright (c) 2016 Eric Holland
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <stdarg.h>
#include <lk/reg.h>
#include <lk/debug.h>
#include <stdio.h>
#include <kernel/thread.h>
#include <dev/uart.h>
#include <arch/ops.h>
#include <arch/arm/cm.h>
#include <platform/debug.h>
#include <target/debugconfig.h>


void nrf52_debug_early_init(void) {
    uart_init_early();
}

/* later in the init process */
void nrf52_debug_init(void) {
    uart_init();
}



void platform_dputc(char c) {
    if (c == '\n')
        uart_putc(DEBUG_UART, '\r');
    uart_putc(DEBUG_UART, c);
}

int platform_dgetc(char *c, bool wait) {
    int ret = uart_getc(DEBUG_UART, wait);
    if (ret == -1)
        return -1;
    *c = ret;
    return 0;
}

