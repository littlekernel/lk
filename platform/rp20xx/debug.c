// Copyright (c) 2020 Brian Swetland
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#include <platform/debug.h>

#include <target/debugconfig.h>

#include <hardware/uart.h>

#include <stdio.h>

void platform_dputc(char c) {
    if (c == '\n')
        uart_putc(DEBUG_UART, '\r');
    uart_putc(DEBUG_UART, c);
}

int platform_dgetc(char *c, bool wait) {
    if (!wait && !uart_is_readable(DEBUG_UART))
        return -1;
    *c = uart_getc(DEBUG_UART);
    return 0;
}

