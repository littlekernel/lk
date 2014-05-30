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

#define RXBUF_SIZE 16

static cbuf_t uart_rx_buf[NUM_UARTS];

static inline uintptr_t uart_to_ptr(unsigned int n) { return (n == 0) ? UART0_BASE : UART1_BASE; }
static inline uint uart_to_irq(unsigned int n) { return (n == 0) ? UART0_INT : UART1_INT; }

#define UART_REG(base, reg)  (*REG32((base)  + (reg)))

static enum handler_return uart_irq(void *arg)
{
    bool resched = false;
    uint port = (uint)arg;
    uintptr_t base = uart_to_ptr(port);

    /* read interrupt status and mask */
    uint32_t isr = UART_REG(base, UART_ISR);
    isr &= UART_REG(base, UART_IMR);

    if (isr & (1<<0)) { // rxtrig
        UART_REG(base, UART_ISR) = (1<< 0);

        while ((UART_REG(base, UART_SR) & (1<<1)) == 0) {
            char c = UART_REG(base, UART_FIFO);
            cbuf_write_char(&uart_rx_buf[port], c, false);

            resched = true;
        }
    }

    return resched ? INT_RESCHEDULE : INT_NO_RESCHEDULE;
}

void uart_init(void)
{
    for (uint i = 0; i < NUM_UARTS; i++) {
        cbuf_initialize(&uart_rx_buf[i], RXBUF_SIZE);

        register_int_handler(uart_to_irq(i), &uart_irq, (void *)i);

        // clear all irqs
        UART_REG(uart_to_ptr(i), UART_IDR) = 0xffffffff;

        // set rx fifo trigger to 1
        UART_REG(uart_to_ptr(i), UART_RXWM) = 1;

        // enable the receiver
        UART_REG(uart_to_ptr(i), UART_CR) |= (1<<2); // rxen

        // enable rx interrupt
        UART_REG(uart_to_ptr(i), UART_IER) = (1<<0); // rxtrig

        unmask_interrupt(uart_to_irq(i));
    }
}

void uart_init_early(void)
{
    for (uint i = 0; i < NUM_UARTS; i++) {
        UART_REG(uart_to_ptr(i), UART_CR) = (1<<4); // txen
    }
}

int uart_putc(int port, char c)
{
    uintptr_t base = uart_to_ptr(port);

    /* spin while fifo is full */
    while (UART_REG(base, UART_SR) & (1<<4))
        ;
    UART_REG(base, UART_FIFO) = c;

    return 1;
}

int uart_getc(int port, bool wait)
{
    char c;
    if (cbuf_read_char(&uart_rx_buf[port], &c, wait) == 1)
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

