/*
 * Copyright (c) 2025 Mykola Hohsadze
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <stdarg.h>
#include <lk/reg.h>
#include <kernel/thread.h>
#include <dev/uart.h>
#include <platform/debug.h>
#include <platform/fvp-base.h>
#include <target/debugconfig.h>

/* DEBUG_UART must be defined to 0 */
#if !defined(DEBUG_UART) || DEBUG_UART != 0
#error define DEBUG_UART to 0
#endif

void platform_dputc(char c) {
    if (c == '\n')
        uart_putc(DEBUG_UART, '\r');
    uart_putc(DEBUG_UART, c);
}

int platform_dgetc(char *c, bool wait) {
    int ret = uart_getc(DEBUG_UART, wait);
    if (ret < 0) {
        return ret;
    }
    *c = ret;
    return 0;
}
