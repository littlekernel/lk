// Copyright (c) 2020 Brian Swetland
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#include <platform/debug.h>

#include <dev/uart.h>

#define RP23XX_DEBUG_UART_PORT 0

void platform_dputc(char c) {
    if (c == '\n') {
        uart_putc(RP23XX_DEBUG_UART_PORT, '\r');
    }
    uart_putc(RP23XX_DEBUG_UART_PORT, c);
}

int platform_dgetc(char *c, bool wait) {
    int ret = uart_getc(RP23XX_DEBUG_UART_PORT, wait);
    if (ret < 0) {
        return -1;
    }
    *c = (char)ret;
    return 0;
}
