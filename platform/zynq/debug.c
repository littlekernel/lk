/*
 * Copyright (c) 2008-2014 Travis Geiselbrecht
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
#include <stdio.h>
#include <kernel/thread.h>
#include <platform/debug.h>
#include <platform/zynq.h>
#include <reg.h>

#define UART_CR (0x00)
#define UART_MR (0x04)
#define UART_IER (0x08)
#define UART_IDR (0x0c)
#define UART_IMR (0x10)
#define UART_ISR (0x14)
#define UART_BAUDGEN (0x18)
#define UART_RXTOUT (0x1c)
#define UART_RXWM (0x20)
#define UART_MODEMCR (0x24)
#define UART_MODEMSR (0x28)
#define UART_SR (0x2c)
#define UART_FIFO (0x30)
#define UART_BAUD_DIV (0x34)
#define UART_FLOW_DELAY (0x38)
#define UART_TX_FIFO_TRIGGER (0x44)

#define UARTREG(reg)  (*REG32(UART0_BASE + (reg)))

void platform_dputc(char c)
{
    UARTREG(UART_FIFO) = c;
}

int platform_dgetc(char *c, bool wait)
{
    if (!wait) {
        if (UARTREG(UART_SR) & (1<<1)) {
            /* fifo empty */
            return -1;
        }
        *c = UARTREG(UART_FIFO) & 0xff;
        return 0;
    } else {
        while ((UARTREG(UART_SR) & (1<<1))) {
            // XXX actually block on interrupt
            thread_yield();
        }

        *c = UARTREG(UART_FIFO) & 0xff;
        return 0;
    }
}

void platform_halt(void)
{
    arch_disable_ints();
    for (;;);
}


void platform_early_init_debug(void)
{
    UARTREG(UART_CR) = (1<<4)|(1<<2); // txen,rxen
}
