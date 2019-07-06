/*
 * Copyright (c) 2012 Travis Geiselbrecht
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
#include <platform/debug.h>
#include <arch/ops.h>
#include <dev/uart.h>
#include <target/debugconfig.h>
#include <arch/arm/cm.h>
#include <platform/stm32.h>

void stm32_debug_early_init(void) {
    uart_init_early();
}

/* later in the init process */
void stm32_debug_init(void) {
    uart_init();
}

void platform_dputc(char c) {
    if (c == '\n')
        uart_putc(DEBUG_UART, '\r');
    uart_putc(DEBUG_UART, c);
}

int platform_dgetc(char *c, bool wait) {
    int ret = uart_getc(DEBUG_UART, wait);
    if (ret < 0)
        return -1;
    *c = ret;
    return 0;
}

void platform_pputc(char c) {
    if (c == '\n')
        uart_pputc(DEBUG_UART, '\r');
    uart_pputc(DEBUG_UART, c);
}

int platform_pgetc(char *c, bool wait) {
    int ret = uart_pgetc(DEBUG_UART);
    if (ret < 0)
        return -1;
    *c = ret;
    return 0;
}
