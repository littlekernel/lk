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
#include <target/debugconfig.h>
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

/* DEBUG_UART must be defined to 0 or 1 */
#if defined(DEBUG_UART) && DEBUG_UART == 0
#define DEBUG_UART_BASE UART0_BASE
#elif defined(DEBUG_UART) && DEBUG_UART == 1
#define DEBUG_UART_BASE UART1_BASE
#else
#error define DEBUG_UART to something valid
#endif

#define UARTREG(reg)  (*REG32(DEBUG_UART_BASE + (reg)))

#if 0
uboot setup for zynq board
E0001000:   00000114
E0001004:   00000020
E0001008:   00000000
E000100C:   00000000
E0001010:   00000000
E0001014:   00000200
E0001018:   0000003E
E000101C:   00000000
E0001020:   00000020
E0001024:   00000000
E0001028:   000000FB
E000102C:   0000000A
E0001030:   00000000
E0001034:   00000006
E0001038:   00000000
E000103C:   00000000
E0001040:   00000000
E0001044:   00000020
#endif

void platform_dputc(char c)
{
    if (c == '\n')
        platform_dputc('\r');

    /* spin while fifo is full */
    while (UARTREG(UART_SR) & (1<<4))
        ;
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
