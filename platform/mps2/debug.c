/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <dev/uart.h>
#include <platform/debug.h>
#include <platform/mps2.h>
#include <stdbool.h>

/* the board wires the first uart to the host console */
#define DEBUG_UART 0

void mps2_debug_early_init(void) {
    uart_init_early();
}

void mps2_debug_init(void) {
    uart_init();
}

void platform_dputc(char c) {
    if (c == '\n') {
        uart_putc(DEBUG_UART, '\r');
    }
    uart_putc(DEBUG_UART, c);
}

int platform_dgetc(char *c, bool wait) {
    int ret = uart_getc(DEBUG_UART, wait);
    if (ret == -1) {
        return -1;
    }
    *c = ret;
    return 0;
}
