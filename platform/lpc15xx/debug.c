/*
 * Copyright (c) 2014 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <stdarg.h>
#include <lk/reg.h>
#include <lk/debug.h>
#include <stdio.h>
#include <lk/compiler.h>
#include <lib/cbuf.h>
#include <kernel/thread.h>
#include <platform/debug.h>
#include <arch/ops.h>
#include <arch/arm/cm.h>
#include <target/debugconfig.h>

#include <platform/lpc.h>

static cbuf_t debug_rx_buf;

/* this code is only set up to handle UART0 as the debug uart */
STATIC_ASSERT(DEBUG_UART == LPC_USART0);

void lpc_debug_early_init(void) {
    /* Use main clock rate as base for UART baud rate divider */
    Chip_Clock_SetUARTBaseClockRate(Chip_Clock_GetMainClockRate(), false);

    /* Setup UART */
    Chip_UART_Init(DEBUG_UART);
    Chip_UART_ConfigData(DEBUG_UART, UART_CFG_DATALEN_8 | UART_CFG_PARITY_NONE | UART_CFG_STOPLEN_1);
    Chip_UART_SetBaud(DEBUG_UART, 115200);
    Chip_UART_Enable(DEBUG_UART);
    Chip_UART_TXEnable(DEBUG_UART);
}

void lpc_debug_init(void) {
    cbuf_initialize(&debug_rx_buf, 16);

    /* enable uart interrupts */
    Chip_UART_IntEnable(DEBUG_UART, UART_INTEN_RXRDY);

    NVIC_EnableIRQ(UART0_IRQn);
}

void lpc_UART0_irq(void) {
    arm_cm_irq_entry();

    /* read the rx buffer until it's empty */
    while ((Chip_UART_GetStatus(DEBUG_UART) & UART_STAT_RXRDY) != 0) {
        uint8_t c = Chip_UART_ReadByte(DEBUG_UART);
        cbuf_write_char(&debug_rx_buf, c, false);
    }

    arm_cm_irq_exit(true);
}

void platform_dputc(char c) {
    if (c == '\n') {
        platform_dputc('\r');
    }

    Chip_UART_SendBlocking(DEBUG_UART, &c, 1);
}

int platform_dgetc(char *c, bool wait) {
#if 1
    return cbuf_read_char(&debug_rx_buf, c, wait);
#else
    uint8_t data;

    if (Chip_UART_Read(DEBUG_UART, &data, 1) == 1) {
        *c = data;
        return 1;
    }
    return -1;
#endif
}

