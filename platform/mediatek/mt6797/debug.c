/*
 * Copyright (c) 2015 MediaTek Inc.
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/debug.h>
#include <arch/ops.h>
#include <stdarg.h>
#include <dev/uart.h>
#include <platform/mt_uart.h>
#include <platform.h>

void _dputc(char c) {
    int port = mtk_get_current_uart();

    if (c == '\n') {
        uart_putc(port, '\r');
    }

    uart_putc(port, c);
}

int dgetc(char *c, bool wait) {
    int _c;
    int port = mtk_get_current_uart();

    if ((_c = uart_getc(port, wait)) < 0) {
        return -1;
    }

    *c = _c;
    return 0;
}

void platform_halt(platform_halt_action suggested_action, platform_halt_reason reason) {
    arch_disable_ints();
    for (;;);
}

uint32_t debug_cycle_count(void) {
    PANIC_UNIMPLEMENTED;
}

void platform_dputc(char c) {
    if (c == '\n') {
        _dputc('\r');
    }

    _dputc(c);
}

int platform_dgetc(char *c, bool wait) {
    return dgetc(c, wait);
}

