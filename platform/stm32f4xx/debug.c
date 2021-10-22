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
#include <platform/stm32.h>
#include <stm32f4xx_rcc.h>
#include <stm32f4xx_usart.h>
#include <arch/arm/cm.h>

void stm32_debug_early_init(void) {
    uart_init_early();
}

/* later in the init process */
void stm32_debug_init(void) {
    uart_init();
}

#define ITM_STIM0   0xE0000000
#define ITM_TCR     0xE0000E80

void platform_dputc(char c) {
    // if ITM is enabled, send character to STIM0
    if (readl(ITM_TCR) & 1) {
        while (!readl(ITM_STIM0)) ;
        writeb(c, ITM_STIM0);
    }

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

void __debugger_console_putc(char c);

#define DCRDR 0xE000EDF8

void _debugmonitor(void) {
    u32 n;
    arm_cm_irq_entry();
    n = readl(DCRDR);
    if (n & 0x80000000) {
        switch (n >> 24) {
            case 0x80: // write to console
                __debugger_console_putc(n & 0xFF);
                n = 0;
                break;
            default:
                n = 0x01000000;
        }
        writel(n, DCRDR);
    }
    arm_cm_irq_exit(1);
}
