/*
 * Copyright (c) 2018 Travis Geiselbrecht
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
#include <trace.h>
#include <lib/cbuf.h>
#include <kernel/thread.h>
#include <platform.h>
#include <platform/interrupts.h>
#include <platform/debug.h>
#include <platform/sifive.h>
#include <sys/types.h>

#include "platform_p.h"

#define LOCAL_TRACE 0

static volatile unsigned int *const gpio_base = (unsigned int *)GPIO_BASE;
static volatile unsigned int *const uart_base = (unsigned int *)UART0_BASE;

#define UART_TXDATA 0
#define UART_RXDATA 1
#define UART_TXCTRL 2
#define UART_RXCTRL 3
#define UART_IE     4
#define UART_IP     5
#define UART_DIV    6

#define RXBUF_SIZE 128
static char uart_rx_buf_data[RXBUF_SIZE];
static struct cbuf uart_rx_buf;

void sifive_uart_write(int c) {
    uart_base[UART_TXDATA] = (c & 0xff);
}

int sifive_uart_read(char *c, bool wait) {
    if (cbuf_read_char(&uart_rx_buf, c, wait) == 1) {
        return 0;
    }
    return -1;
}

static enum handler_return sifive_uart_irq(void *unused) {
    LTRACE;

    enum handler_return ret = INT_NO_RESCHEDULE;
    for (;;) {
        int c = uart_base[UART_RXDATA];
        if (c & (1<<31))
            break; // nothing in the fifo

        // stuff this char in the cbuf and try again
        cbuf_write_char(&uart_rx_buf, c & 0xff, false);
        ret = INT_RESCHEDULE;
    }

    return ret;
}

void sifive_uart_early_init(void) {
    gpio_base[14] = (3<<16); // io function enable for pin 16/17

    uart_base[UART_DIV] = 0x9be; // divisor
    uart_base[UART_TXCTRL] = 1; // txen
}

void sifive_uart_init(void) {
    cbuf_initialize_etc(&uart_rx_buf, RXBUF_SIZE, uart_rx_buf_data);

    register_int_handler(SIFIVE_IRQ_UART0, sifive_uart_irq, NULL);

    uart_base[UART_RXCTRL] = 1; // rxen, rxcnt = 0
    uart_base[UART_IE] |= (1<<1); // rxwvm

    unmask_interrupt(SIFIVE_IRQ_UART0);
}

