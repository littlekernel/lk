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
#include <bits.h>
#include <stdio.h>
#include <trace.h>
#include <lib/cbuf.h>
#include <kernel/thread.h>
#include <kernel/spinlock.h>
#include <platform/interrupts.h>
#include <platform/debug.h>
#include <platform/alterasoc.h>

#define UART_THR (0x00)
#define UART_RBR (0x00)
#define UART_DLL (0x00)
#define UART_IER (0x04)
#define UART_DLH (0x04)
#define UART_IIR (0x08)
#define UART_FCR (0x08)
#define UART_LCR (0x0c)
#define UART_MCR (0x10)
#define UART_LSR (0x14)
#define UART_MSR (0x18)
#define UART_SCR (0x1c)
#define UART_USR (0x7c)
#define UART_TFL (0x80)
#define UART_RFL (0x84)
#define UART_SRR (0x88)

#define UARTREG(base, reg)  (*REG32((base)  + (reg)))

#define RXBUF_SIZE 16

static cbuf_t uart0_rx_buf;
static cbuf_t uart1_rx_buf;

static inline uintptr_t uart_to_ptr(unsigned int n) { return (n == 0) ? UART0_BASE : UART1_BASE; }
static inline cbuf_t *uart_to_rxbuf(unsigned int n) { return (n == 0) ? &uart0_rx_buf : &uart1_rx_buf; }

static spin_lock_t lock = SPIN_LOCK_INITIAL_VALUE;

static enum handler_return uart_irq(void *arg)
{
    bool resched = false;
    uint port = (uint)arg;
    uintptr_t base = uart_to_ptr(port);

    /* read interrupt identity */
    uint32_t iir = UARTREG(base, UART_IIR);

    /* receive data available */
    if (BITS(iir, 3, 0) == 0x4) {
        cbuf_t *rxbuf = uart_to_rxbuf(port);

        /* while receive fifo not empty, read a char */
        while ((UARTREG(base, UART_USR) & (1<<3))) {
            char c = UARTREG(base, UART_RBR);
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

    // enable the fifo
    UARTREG(uart_to_ptr(0), UART_FCR) = (1<<0); // enable rx fifo, set tx trigger to 0, set rx trigger to 0
    UARTREG(uart_to_ptr(1), UART_FCR) = (1<<0); // enable rx fifo, set tx trigger to 0, set rx trigger to 0

    // enable rx interrupt
    UARTREG(uart_to_ptr(0), UART_IER) = (1<<0); // receive data interrupt
    UARTREG(uart_to_ptr(1), UART_IER) = (1<<0); // receive data interrupt

    unmask_interrupt(UART0_INT);
    unmask_interrupt(UART1_INT);
}

void uart_init_early(void)
{
#if 0
    UARTREG(uart_to_ptr(0), UART_CR) = (1<<4); // txen
    UARTREG(uart_to_ptr(1), UART_CR) = (1<<4); // txen
#endif
}

int uart_putc(int port, char c)
{
    uintptr_t base = uart_to_ptr(port);

    spin_lock_saved_state_t state;
    spin_lock_irqsave(&lock, state);

    /* spin while fifo is full */
    while ((UARTREG(base, UART_USR) & (1<<1)) == 0) {
    }
    UARTREG(base, UART_THR) = c;

    spin_unlock_irqrestore(&lock, state);

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

