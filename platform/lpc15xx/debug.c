/*
 * Copyright (c) 2014 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <stdarg.h>
#include <reg.h>
#include <debug.h>
#include <stdio.h>
#include <lib/cbuf.h>
#include <kernel/thread.h>
#include <platform/debug.h>
#include <arch/ops.h>
#include <arch/arm/cm.h>
#include <target/debugconfig.h>

static cbuf_t debug_rx_buf;

void lpc_debug_early_init(void)
{
    /* Use main clock rate as base for UART baud rate divider */
    Chip_Clock_SetUARTBaseClockRate(Chip_Clock_GetMainClockRate(), false);

    /* Setup UART */
    Chip_UART_Init(DEBUG_UART);
    Chip_UART_ConfigData(DEBUG_UART, UART_CFG_DATALEN_8 | UART_CFG_PARITY_NONE | UART_CFG_STOPLEN_1);
    Chip_UART_SetBaud(DEBUG_UART, 115200);
    Chip_UART_Enable(DEBUG_UART);
    Chip_UART_TXEnable(DEBUG_UART);
}

void lpc_debug_init(void)
{
    cbuf_initialize(&debug_rx_buf, 16);
}

void platform_dputc(char c)
{
    if (c == '\n') {
        _dputc('\r');
    }

    Chip_UART_SendBlocking(DEBUG_UART, &c, 1);
}

int platform_dgetc(char *c, bool wait)
{
    //return cbuf_read_char(&debug_rx_buf, c, wait);

    uint8_t data;

    if (Chip_UART_Read(DEBUG_UART, &data, 1) == 1) {
        *c = data;
        return 1;
    }
    return -1;
}

void platform_halt(void)
{
    dprintf(ALWAYS, "HALT: spinning forever...\n");
    for (;;);
}


