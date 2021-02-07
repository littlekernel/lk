// Copyright (c) 2020 Brian Swetland
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#include <platform/debug.h>

#include <hardware/uart.h>

#include <stdio.h>

void platform_dputc(char c) {
    if (c == '\n')
        uart_putc(uart0, '\r');
    uart_putc(uart0, c);
}

int platform_dgetc(char *c, bool wait) {
    if (!wait && !uart_is_readable(uart0))
        return -1;
    *c = uart_getc(uart0);
    return 0;
}

