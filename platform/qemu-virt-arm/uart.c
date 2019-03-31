/*
 * Copyright (c) 2014-2015 Travis Geiselbrecht
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
#include <platform/qemu-virt.h>
#include <target/debugconfig.h>

/* PL011 implementation */
#define UART_DR    (0x00)
#define UART_RSR   (0x04)
#define UART_TFR   (0x18)
#define UART_ILPR  (0x20)
#define UART_IBRD  (0x24)
#define UART_FBRD  (0x28)
#define UART_LCRH  (0x2c)
#define UART_CR    (0x30)
#define UART_IFLS  (0x34)
#define UART_IMSC  (0x38)
#define UART_TRIS  (0x3c)
#define UART_TMIS  (0x40)
#define UART_ICR   (0x44)
#define UART_DMACR (0x48)

#define UARTREG(base, reg)  (*REG32((base)  + (reg)))

#define RXBUF_SIZE 16
#define NUM_UART 1

static cbuf_t uart_rx_buf[NUM_UART];

static inline uintptr_t uart_to_ptr(unsigned int n)
{
    switch (n) {
        default:
        case 0:
            return UART_BASE;
    }
}

static enum handler_return uart_irq(void *arg)
{
    bool resched = false;
    uint port = (uintptr_t)arg;
    uintptr_t base = uart_to_ptr(port);

    /* read interrupt status and mask */
    uint32_t isr = UARTREG(base, UART_TMIS);

    if (isr & (1<<4)) { // rxmis
        cbuf_t *rxbuf = &uart_rx_buf[port];

        /* while fifo is not empty, read chars out of it */
        while ((UARTREG(base, UART_TFR) & (1<<4)) == 0) {
#if CONSOLE_HAS_INPUT_BUFFER
            if (port == DEBUG_UART) {
                char c = UARTREG(base, UART_DR);
                cbuf_write_char(&console_input_cbuf, c, false);
            } else
#endif
            {
                /* if we're out of rx buffer, mask the irq instead of handling it */
                if (cbuf_space_avail(rxbuf) == 0) {
                    UARTREG(base, UART_IMSC) &= ~(1<<4); // !rxim
                    break;
                }

                char c = UARTREG(base, UART_DR);
                cbuf_write_char(rxbuf, c, false);
            }

            resched = true;
        }
    }

    return resched ? INT_RESCHEDULE : INT_NO_RESCHEDULE;
}

void uart_init(void)
{
    for (size_t i = 0; i < NUM_UART; i++) {
        uintptr_t base = uart_to_ptr(i);

        // create circular buffer to hold received data
        cbuf_initialize(&uart_rx_buf[i], RXBUF_SIZE);

        // assumes interrupts are contiguous
        register_int_handler(UART0_INT + i, &uart_irq, (void *)i);

        // clear all irqs
        UARTREG(base, UART_ICR) = 0x3ff;

        // set fifo trigger level
        UARTREG(base, UART_IFLS) = 0; // 1/8 rxfifo, 1/8 txfifo

        // enable rx interrupt
        UARTREG(base, UART_IMSC) = (1<<4); // rxim

        // enable receive
        UARTREG(base, UART_CR) |= (1<<9); // rxen

        // enable interrupt
        unmask_interrupt(UART0_INT + i);
    }
}

void uart_init_early(void)
{
    for (size_t i = 0; i < NUM_UART; i++) {
        UARTREG(uart_to_ptr(i), UART_CR) = (1<<8)|(1<<0); // tx_enable, uarten
    }
}

int uart_putc(int port, char c)
{
    uintptr_t base = uart_to_ptr(port);

    /* spin while fifo is full */
    while (UARTREG(base, UART_TFR) & (1<<5))
        ;
    UARTREG(base, UART_DR) = c;

    return 1;
}

int uart_getc(int port, bool wait)
{
    cbuf_t *rxbuf = &uart_rx_buf[port];

    char c;
    if (cbuf_read_char(rxbuf, &c, wait) == 1) {
        UARTREG(uart_to_ptr(port), UART_IMSC) = (1<<4); // rxim
        return c;
    }

    return -1;
}

/* panic-time getc/putc */
int uart_pputc(int port, char c)
{
    uintptr_t base = uart_to_ptr(port);

    /* spin while fifo is full */
    while (UARTREG(base, UART_TFR) & (1<<5))
        ;
    UARTREG(base, UART_DR) = c;

    return 1;
}

int uart_pgetc(int port, bool wait)
{
    uintptr_t base = uart_to_ptr(port);

    if ((UARTREG(base, UART_TFR) & (1<<4)) == 0) {
        return UARTREG(base, UART_DR);
    } else {
        return -1;
    }
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

