/*
 * Copyright (c) 2018 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/reg.h>
#include <lk/trace.h>
#include <lib/cbuf.h>
#include <kernel/thread.h>
#include <platform.h>
#include <platform/interrupts.h>
#include <platform/debug.h>
#include <platform/sifive.h>
#include <sys/types.h>

#include "platform_p.h"

#define LOCAL_TRACE 0
#define POLLING_RX 0

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
static spin_lock_t uart_tx_spin_lock = 0;

void sifive_uart_write(int c) {
    spin_lock_saved_state_t state;
    spin_lock_irqsave(&uart_tx_spin_lock, state);

    // wait for tx fifo to clear
    while (uart_base[UART_TXDATA] & (1<<31))
        ;
    uart_base[UART_TXDATA] = (c & 0xff);

    spin_unlock_irqrestore(&uart_tx_spin_lock, state);
}

int sifive_uart_read(char *c, bool wait) {
#if !POLLING_RX
    if (cbuf_read_char(&uart_rx_buf, c, wait) == 1) {
        return 0;
    }
    return -1;
#else
    // full polling mode for reads
    int i = uart_base[UART_RXDATA];
    if (i & (1<<31))
        return -1;
    *c = i & 0xff;
    return 0;
#endif
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
    uart_base[UART_DIV] = SIFIVE_FREQ / 115200;
    uart_base[UART_TXCTRL] = 1; // txen
}

void sifive_uart_init(void) {
    cbuf_initialize_etc(&uart_rx_buf, RXBUF_SIZE, uart_rx_buf_data);

    register_int_handler(SIFIVE_IRQ_UART0, sifive_uart_irq, NULL);

    uart_base[UART_RXCTRL] = 1; // rxen, rxcnt = 0
    uart_base[UART_IE] |= (1<<1); // rxwvm

#if !POLLING_RX
    unmask_interrupt(SIFIVE_IRQ_UART0);
#endif
}

