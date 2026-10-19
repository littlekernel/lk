/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <dev/uart.h>
#include <dev/uart/dwc8250.h>
#include <platform/debug.h>
#include <platform/virt.h>

#include "platform_p.h"

static const struct dwc8250_config uart_config = {
    .base = UART0_BASE_VIRT,
    .irq = IRQ_UART0,
    // input goes to the console buffer when the build has one (CONSOLE_HAS_INPUT_BUFFER)
    .flag = DWC8250_FLAG_DEBUG_UART,
    .reg_style = DWC8250_REG_8,
};

void platform_init_uart_early(void) {
    dwc8250_init_early(DEBUG_UART, &uart_config);
}

void platform_init_uart(void) {
    dwc8250_init(DEBUG_UART);
}

void platform_dputc(char c) {
    if (c == '\n') {
        uart_putc(DEBUG_UART, '\r');
    }
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

/* panic-time getc/putc */
void platform_pputc(char c) {
    if (c == '\n') {
        uart_pputc(DEBUG_UART, '\r');
    }
    uart_pputc(DEBUG_UART, c);
}

int platform_pgetc(char *c, bool wait) {
    int ret = uart_pgetc(DEBUG_UART);
    if (ret < 0) {
        return ret;
    }
    *c = ret;
    return 0;
}
