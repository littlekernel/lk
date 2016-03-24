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
#include <assert.h>
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

        while ((UART_REG(base, UART_SR) & (1<<1)) == 0) { // ~rempty
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

        uintptr_t base = uart_to_ptr(i);

        // clear all irqs
        UART_REG(base, UART_IDR) = 0xffffffff;

        // set rx fifo trigger to 1
        UART_REG(base, UART_RXWM) = 1;

        // enable the receiver
        // NOTE: must clear rxdis and set rxen in the same write
        UART_REG(base, UART_CR) = (UART_REG(base, UART_CR) & ~(1<<3)) | (1 << 2);

        // enable rx interrupt
        UART_REG(base, UART_IER) = (1<<0); // rxtrig

        // set up interrupt handler
        register_int_handler(uart_to_irq(i), &uart_irq, (void *)i);
        unmask_interrupt(uart_to_irq(i));
    }
}

void uart_init_early(void)
{
    for (uint i = 0; i < NUM_UARTS; i++) {
        uintptr_t base = uart_to_ptr(i);

        UART_REG(base, UART_BAUD_DIV) = UART_BRD_DIV(6);
        UART_REG(base, UART_BAUDGEN) = UART_BRG_DIV(0x3E);

        // reset the tx/rx path
        UART_REG(base, UART_CR) |= UART_CR_TXRES | UART_CR_RXRES;
        while ((UART_REG(base, UART_CR) & (UART_CR_TXRES | UART_CR_RXRES)) != 0)
            ;

        // n81, clock select ref_clk
        UART_REG(base, UART_MR) = UART_MR_PAR(0x4); // no parity

        // no flow
        UART_REG(base, UART_MODEMCR) = 0;

        UART_REG(base, UART_CR) = UART_CR_TXEN;
    }

    /* Configuration for the serial console */
    /*UART_REG(UART1_BASE, UART_CR) = 0x00000017;*/
    /*UART_REG(UART1_BASE, UART_MR) = 0x00000020;*/
}

int uart_putc(int port, char c)
{
    DEBUG_ASSERT(port >= 0 && port < NUM_UARTS);

    uintptr_t base = uart_to_ptr(port);

    /* spin while fifo is full */
    while (UART_REG(base, UART_SR) & (1<<4))
        ;
    UART_REG(base, UART_FIFO) = c;

    return 1;
}

int uart_getc(int port, bool wait)
{
    DEBUG_ASSERT(port >= 0 && port < NUM_UARTS);

    char c;
    if (cbuf_read_char(&uart_rx_buf[port], &c, wait) == 1)
        return c;

    return -1;
}

void uart_flush_tx(int port)
{
    DEBUG_ASSERT(port >= 0 && port < NUM_UARTS);
}

void uart_flush_rx(int port)
{
    DEBUG_ASSERT(port >= 0 && port < NUM_UARTS);
}

void uart_init_port(int port, uint baud)
{
    DEBUG_ASSERT(port >= 0 && port < NUM_UARTS);

    PANIC_UNIMPLEMENTED;
}

