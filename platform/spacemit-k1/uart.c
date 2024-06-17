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
#include <platform/spacemit-k1.h>
#include <sys/types.h>

#include "platform_p.h"

// simple 16550 driver for the emulated serial port on jh7110
// uart registers are 4 byte separated

static volatile uint8_t *const uart_base = (uint8_t *)UART0_BASE_VIRT;

#define RXBUF_SIZE 128
static char uart_rx_buf_data[RXBUF_SIZE];
static cbuf_t uart_rx_buf;

static inline uint8_t uart_read_8(size_t offset) {
    return uart_base[offset * 4];
}

static inline void uart_write_8(size_t offset, uint8_t val) {
    uart_base[offset * 4] = val;
}

static enum handler_return uart_irq_handler(void *arg) {
    unsigned char c;
    bool resched = false;

    while (uart_read_8(5) & (1<<0)) {
        c = uart_read_8(0);
        cbuf_write_char(&uart_rx_buf, c, false);
        resched = true;
    }

    return resched ? INT_RESCHEDULE : INT_NO_RESCHEDULE;
}

void uart_init(void) {
    /* finish uart init to get rx going */
    cbuf_initialize_etc(&uart_rx_buf, RXBUF_SIZE, uart_rx_buf_data);

    register_int_handler(IRQ_UART0, uart_irq_handler, NULL);

    uart_write_8(1, (1<<6) | 0x1); // enable receive data available interrupt
    uart_write_8(4, (1<<3)); // set OUT2, enabling IRQs to reach the cpu

    unmask_interrupt(IRQ_UART0);
}

static void uart_putc(char c) {
    while ((uart_read_8(5) & (1<<6)) == 0)
        ;
    uart_write_8(0, c);
}

static int uart_getc(char *c, bool wait) {
    return cbuf_read_char(&uart_rx_buf, c, wait);
}

void platform_dputc(char c) {
    if (c == '\n')
        platform_dputc('\r');
    uart_putc(c);
}

int platform_dgetc(char *c, bool wait) {
    int ret = uart_getc(c, wait);

    return ret;
}

/* panic-time getc/putc */
int platform_pgetc(char *c, bool wait) {
    if (uart_read_8(5) & (1<<0)) {
        *c = uart_read_8(0);
        return 0;
    }
    return -1;
}

