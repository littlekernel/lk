/*
 * Copyright (c) 2008-2014 Travis Geiselbrecht
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
#include <platform/qemu-virt.h>
#include <target/debugconfig.h>

/* DEBUG_UART must be defined to 0 or 1 */
#if defined(DEBUG_UART) && DEBUG_UART == 0
#define DEBUG_UART_BASE UART0_BASE
#elif defined(DEBUG_UART) && DEBUG_UART == 1
#define DEBUG_UART_BASE UART1_BASE
#else
#error define DEBUG_UART to something valid
#endif

void platform_dputc(char c) {
    if (c == '\n')
        uart_putc(DEBUG_UART, '\r');
    uart_putc(DEBUG_UART, c);
}

int platform_dgetc(char *c, bool wait) {
    int ret = uart_getc(DEBUG_UART, wait);
    if (ret < 0)
        return ret;
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
        return ret;
    *c = ret;
    return 0;
}

