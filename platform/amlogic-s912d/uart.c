/*
 * Copyright (c) 2018 The Fuchsia Authors
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

#include <reg.h>
#include <stdio.h>
#include <trace.h>
#include <lib/cbuf.h>
#include <kernel/thread.h>
#include <platform/interrupts.h>
#include <platform/s912d.h>
#include <dev/uart.h>

#define S912D_UART_WFIFO            (0x00)
#define S912D_UART_RFIFO            (0x04)
#define S912D_UART_CONTROL          (0x08)
#define S912D_UART_STATUS           (0x0C)
#define S912D_UART_IRQ_CONTROL      (0x10)
#define S912D_UART_REG5             (0x14)

#define S912D_UART_CONTROL_INVRTS    (1 << 31)
#define S912D_UART_CONTROL_MASKERR   (1 << 30)
#define S912D_UART_CONTROL_INVCTS    (1 << 29)
#define S912D_UART_CONTROL_TXINTEN   (1 << 28)
#define S912D_UART_CONTROL_RXINTEN   (1 << 27)
#define S912D_UART_CONTROL_INVTX     (1 << 26)
#define S912D_UART_CONTROL_INVRX     (1 << 25)
#define S912D_UART_CONTROL_CLRERR    (1 << 24)
#define S912D_UART_CONTROL_RSTRX     (1 << 23)
#define S912D_UART_CONTROL_RSTTX     (1 << 22)
#define S912D_UART_CONTROL_XMITLEN   (1 << 20)
#define S912D_UART_CONTROL_XMITLEN_MASK   (0x3 << 20)
#define S912D_UART_CONTROL_PAREN     (1 << 19)
#define S912D_UART_CONTROL_PARTYPE   (1 << 18)
#define S912D_UART_CONTROL_STOPLEN   (1 << 16)
#define S912D_UART_CONTROL_STOPLEN_MASK   (0x3 << 16)
#define S912D_UART_CONTROL_TWOWIRE   (1 << 15)
#define S912D_UART_CONTROL_RXEN      (1 << 13)
#define S912D_UART_CONTROL_TXEN      (1 << 12)
#define S912D_UART_CONTROL_BAUD0     (1 << 0)
#define S912D_UART_CONTROL_BAUD0_MASK     (0xfff << 0)

#define S912D_UART_STATUS_RXBUSY         (1 << 26)
#define S912D_UART_STATUS_TXBUSY         (1 << 25)
#define S912D_UART_STATUS_RXOVRFLW       (1 << 24)
#define S912D_UART_STATUS_CTSLEVEL       (1 << 23)
#define S912D_UART_STATUS_TXEMPTY        (1 << 22)
#define S912D_UART_STATUS_TXFULL         (1 << 21)
#define S912D_UART_STATUS_RXEMPTY        (1 << 20)
#define S912D_UART_STATUS_RXFULL         (1 << 19)
#define S912D_UART_STATUS_TXOVRFLW       (1 << 18)
#define S912D_UART_STATUS_FRAMEERR       (1 << 17)
#define S912D_UART_STATUS_PARERR         (1 << 16)
#define S912D_UART_STATUS_TXCOUNT_POS    (8)
#define S912D_UART_STATUS_TXCOUNT_MASK   (0x7f << S912D_UART_STATUS_TXCOUNT_POS)
#define S912D_UART_STATUS_RXCOUNT_POS    (0)
#define S912D_UART_STATUS_RXCOUNT_MASK   (0x7f << S912D_UART_STATUS_RXCOUNT_POS)


#define UARTREG(base, reg)          (*(volatile uint32_t*)((base)  + (reg)))

#define RXBUF_SIZE 16
#define NUM_UART 1

static cbuf_t uart_rx_buf;
static uintptr_t uart_base = 0;
static uint32_t uart_irq = 0;


int uart_putc(int port, char c)
{
    while (UARTREG(uart_base, S912D_UART_STATUS) & S912D_UART_STATUS_TXFULL)
        ;
    UARTREG(uart_base, S912D_UART_WFIFO) = c;

    return 0;
}

int uart_getc(int port, bool wait)
{
    char c;
    if (cbuf_read_char(&uart_rx_buf, &c, wait) == 1) {
        return c;
    }
    else {
        return -1;
    }
}

void uart_irq_handler(void)
{
    while ( (UARTREG(uart_base, S912D_UART_STATUS) & S912D_UART_STATUS_RXCOUNT_MASK) > 0 ) {
        if (cbuf_space_avail(&uart_rx_buf) == 0) {
            break;
        }
        char c = UARTREG(uart_base, S912D_UART_RFIFO);
        cbuf_write_char(&uart_rx_buf, c,false);
    }
}

void uart_init_early(void)
{
    uart_base = UART0_AO_BASE;
    uart_irq = UART0_IRQ;

    // reset port
    UARTREG(uart_base,S912D_UART_CONTROL) |=  S912D_UART_CONTROL_RSTRX |
            S912D_UART_CONTROL_RSTTX |
            S912D_UART_CONTROL_CLRERR;
    UARTREG(uart_base,S912D_UART_CONTROL) &= ~(S912D_UART_CONTROL_RSTRX |
            S912D_UART_CONTROL_RSTTX |
            S912D_UART_CONTROL_CLRERR);

    // enable rx and tx
    UARTREG(uart_base,S912D_UART_CONTROL) |= S912D_UART_CONTROL_TXEN |
            S912D_UART_CONTROL_RXEN;

    UARTREG(uart_base,S912D_UART_CONTROL) |= S912D_UART_CONTROL_INVRTS |
            S912D_UART_CONTROL_RXINTEN |
            S912D_UART_CONTROL_TWOWIRE;

}
void uart_init(void)
{
    uart_base = UART0_AO_BASE;
    uart_irq = UART0_IRQ;

    // create a circular buufer to hold rx data
    cbuf_initialize(&uart_rx_buf, RXBUF_SIZE);

    // register uart irq
    register_int_handler(uart_irq, &uart_irq_handler, NULL);

    // reset port
    UARTREG(uart_base,S912D_UART_CONTROL) |=  S912D_UART_CONTROL_RSTRX |
            S912D_UART_CONTROL_RSTTX |
            S912D_UART_CONTROL_CLRERR;
    UARTREG(uart_base,S912D_UART_CONTROL) &= ~(S912D_UART_CONTROL_RSTRX |
            S912D_UART_CONTROL_RSTTX |
            S912D_UART_CONTROL_CLRERR);

    // enable rx and tx
    UARTREG(uart_base,S912D_UART_CONTROL) |= S912D_UART_CONTROL_TXEN |
            S912D_UART_CONTROL_RXEN;

    UARTREG(uart_base,S912D_UART_CONTROL) |= S912D_UART_CONTROL_INVRTS |
            S912D_UART_CONTROL_RXINTEN |
            S912D_UART_CONTROL_TWOWIRE;

    // Set to interrupt every 1 rx byte
    uint32_t temp2 = UARTREG(uart_base,S912D_UART_IRQ_CONTROL);
    temp2 &= 0xffff0000;
    temp2 |= (1 << 8) | ( 1 );
    UARTREG(uart_base,S912D_UART_IRQ_CONTROL) = temp2;

    // TODO: Look into adding baud rate support

    // enable interrupts
    unmask_interrupt(uart_irq);

}