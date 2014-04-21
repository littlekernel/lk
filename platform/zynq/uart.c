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
#include <reg.h>
#include <stdio.h>
#include <trace.h>
#include <lib/cbuf.h>
#include <kernel/thread.h>
#include <platform/interrupts.h>
#include <platform/debug.h>
#include <platform/zynq.h>

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

#define UARTREG(base, reg)  (*REG32((base)  + (reg)))

#define RXBUF_SIZE 16

static cbuf_t uart0_rx_buf;
static cbuf_t uart1_rx_buf;

static inline uintptr_t uart_to_ptr(unsigned int n) { return (n == 0) ? UART0_BASE : UART1_BASE; }
static inline cbuf_t *uart_to_rxbuf(unsigned int n) { return (n == 0) ? &uart0_rx_buf : &uart1_rx_buf; }

static enum handler_return uart_irq(void *arg)
{
    bool resched = false;
    uint port = (uint)arg;
    uintptr_t base = uart_to_ptr(port);

    /* read interrupt status and mask */
    uint32_t isr = UARTREG(base, UART_ISR);
    isr &= UARTREG(base, UART_IMR);

    if (isr & (1<<0)) { // rxtrig
        UARTREG(base, UART_ISR) = (1<< 0);
        cbuf_t *rxbuf = uart_to_rxbuf(port);

        while ((UARTREG(base, UART_SR) & (1<<1)) == 0) {
            char c = UARTREG(base, UART_FIFO);
            cbuf_write_char(rxbuf, c, false);

            resched = true;
        }
    }

    return resched ? INT_RESCHEDULE : INT_NO_RESCHEDULE;
}

void uart_init(void)
{
    cbuf_initialize(&uart0_rx_buf, RXBUF_SIZE);
    cbuf_initialize(&uart1_rx_buf, RXBUF_SIZE);

    register_int_handler(UART0_INT, &uart_irq, (void *)0);
    register_int_handler(UART1_INT, &uart_irq, (void *)1);

    // clear all irqs
    UARTREG(uart_to_ptr(0), UART_IDR) = 0xffffffff;
    UARTREG(uart_to_ptr(1), UART_IDR) = 0xffffffff;

    UARTREG(uart_to_ptr(0), UART_CR) |= (1<<2); // rxen
    UARTREG(uart_to_ptr(1), UART_CR) |= (1<<2); // rxen

    // set rx fifo trigger to 1
    UARTREG(uart_to_ptr(0), UART_RXWM) = 1;
    UARTREG(uart_to_ptr(1), UART_RXWM) = 1;

    // enable rx interrupt
    UARTREG(uart_to_ptr(0), UART_IER) = (1<<0); // rxtrig
    UARTREG(uart_to_ptr(1), UART_IER) = (1<<0); // rxtrig

    unmask_interrupt(UART0_INT);
    unmask_interrupt(UART1_INT);
}

void uart_init_early(void)
{
    UARTREG(uart_to_ptr(0), UART_CR) = (1<<4); // txen
    UARTREG(uart_to_ptr(1), UART_CR) = (1<<4); // txen
}

int uart_putc(int port, char c)
{
    uintptr_t base = uart_to_ptr(port);

    /* spin while fifo is full */
    while (UARTREG(base, UART_SR) & (1<<4))
        ;
    UARTREG(base, UART_FIFO) = c;

    return 1;
}

int uart_getc(int port, bool wait)
{
    cbuf_t *rxbuf = uart_to_rxbuf(port);

    char c;
    if (cbuf_read_char(rxbuf, &c, wait) == 1)
        return c;

    return -1;
}

void uart_flush_tx(int port)
{
}

void uart_flush_rx(int port)
{
}

void uart_init_port(int port, uint baud)
{
}

