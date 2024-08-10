/*
 * Copyright (c) 2009 Corey Tabaka
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <stdarg.h>
#include <lk/reg.h>
#include <stdio.h>
#include <kernel/thread.h>
#include <arch/x86.h>
#include <lib/cbuf.h>
#include <platform.h>
#include <platform/interrupts.h>
#include <platform/pc.h>
#include <platform/console.h>
#include <platform/keyboard.h>
#include <platform/debug.h>

#include "platform_p.h"

#ifndef DEBUG_BAUD_RATE
#define DEBUG_BAUD_RATE 115200
#endif
#ifndef DEBUG_COM_PORT
#define DEBUG_COM_PORT 1
#endif

static const int uart_baud_rate = DEBUG_BAUD_RATE;
static const int uart_io_port = (DEBUG_COM_PORT == 1) ? COM1_REG :
                                (DEBUG_COM_PORT == 2) ? COM2_REG :
                                (DEBUG_COM_PORT == 3) ? COM3_REG :
                                COM4_REG;
static const int uart_irq = (DEBUG_COM_PORT == 1 || DEBUG_COM_PORT == 3) ? INT_COM1_COM3 : INT_COM2_COM4;

cbuf_t console_input_buf;

static enum handler_return uart_irq_handler(void *arg) {
    unsigned char c;
    bool resched = false;

    while (inp(uart_io_port + 5) & (1<<0)) {
        c = inp(uart_io_port + 0);
        cbuf_write_char(&console_input_buf, c, false);
        resched = true;
    }

    return resched ? INT_RESCHEDULE : INT_NO_RESCHEDULE;
}

void platform_init_debug_early(void) {
    /* configure the uart */
    const int divisor = 115200 / uart_baud_rate;

    /* get basic config done so that tx functions */
    outp(uart_io_port + 1, 0); // mask all irqs
    outp(uart_io_port + 3, 0x80); // set up to load divisor latch
    outp(uart_io_port + 0, divisor & 0xff); // lsb
    outp(uart_io_port + 1, divisor >> 8); // msb
    outp(uart_io_port + 3, 3); // 8N1
    outp(uart_io_port + 2, 0x07); // enable FIFO, clear, 14-byte threshold
    outp(uart_io_port + 4, 0x3); // drive flow control bits high
}

void platform_init_debug(void) {
    /* finish uart init to get rx going */
    cbuf_initialize(&console_input_buf, 1024);

    register_int_handler(uart_irq, uart_irq_handler, NULL);
    unmask_interrupt(uart_irq);

    outp(uart_io_port + 1, 0x1); // enable receive data available interrupt

    // modem control register: Auxiliary Output 2 is another IRQ enable bit
    const uint8_t mcr = inp(uart_io_port + 4);
    outp(uart_io_port + 4, mcr | 0x8);
}

static void debug_uart_putc(char c) {
    while ((inp(uart_io_port + 5) & (1<<6)) == 0)
        ;
    outp(uart_io_port + 0, c);
}

void platform_dputc(char c) {
    if (c == '\n')
        platform_dputc('\r');

    cputc(c);
    debug_uart_putc(c);
}

int platform_dgetc(char *c, bool wait) {
    return cbuf_read_char(&console_input_buf, c, wait);
}
