/*
 * Copyright (c) 2008-2014 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/reg.h>
#include <stdio.h>
#include <kernel/thread.h>
#include <dev/uart.h>
#include <platform.h>
#include <platform/debug.h>
#include <platform/zynq.h>
#include <target/debugconfig.h>
#include <lk/reg.h>

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
    if (ret == -1)
        return -1;
    *c = ret;
    return 0;

}

/* zynq specific halt */
void platform_halt(platform_halt_action suggested_action,
                   platform_halt_reason reason) {
    switch (suggested_action) {
        default:
        case HALT_ACTION_SHUTDOWN:
        case HALT_ACTION_HALT:
            printf("HALT: spinning forever... (reason = %d)\n", reason);
            arch_disable_ints();
            for (;;)
                arch_idle();
            break;
        case HALT_ACTION_REBOOT:
            printf("REBOOT\n");
            arch_disable_ints();
            for (;;) {
                zynq_slcr_unlock();
                SLCR->PSS_RST_CTRL = 1;
            }
            break;
    }
}
