/*
 * Copyright (c) 2009 Corey Tabaka
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <arch/x86.h>
#include <hw/multiboot.h>
#include <kernel/spinlock.h>
#include <kernel/thread.h>
#include <lib/cbuf.h>
#include <lk/reg.h>
#include <platform.h>
#include <platform/debug.h>
#include <platform/interrupts.h>
#include <platform/keyboard.h>
#include <platform/pc.h>
#include <platform/vga_console.h>
#include <stdarg.h>

#include <platform/fb_console.h>

#include "platform_p.h"

#ifndef DEBUG_BAUD_RATE
#define DEBUG_BAUD_RATE 115200
#endif
#ifndef DEBUG_COM_PORT
#define DEBUG_COM_PORT 1
#endif

static const int uart_baud_rate = DEBUG_BAUD_RATE;
static const int uart_io_port = (DEBUG_COM_PORT == 1)   ? COM1_REG
                                : (DEBUG_COM_PORT == 2) ? COM2_REG
                                : (DEBUG_COM_PORT == 3) ? COM3_REG
                                                        : COM4_REG;
static const int uart_irq =
    (DEBUG_COM_PORT == 1 || DEBUG_COM_PORT == 3) ? INT_COM1_COM3 : INT_COM2_COM4;

cbuf_t console_input_buf;

/* serializes console output, and guards the uart and VGA console state */
static spin_lock_t console_lock = SPIN_LOCK_INITIAL_VALUE;

/*
 * THRE with the FIFO on means the whole FIFO is free, so the line status only has
 * to be polled once per FIFO's worth of output. Each port access is a trap under a
 * hypervisor.
 */
static unsigned int uart_tx_fifo_depth = 1;
static unsigned int uart_tx_free;

extern uint32_t _multiboot2_info;

static enum handler_return uart_irq_handler(void *arg) {
    unsigned char c;
    bool resched = false;

    while (inp(uart_io_port + 5) & (1 << 0)) {
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
    outp(uart_io_port + 1, 0);              // mask all irqs
    outp(uart_io_port + 3, 0x80);           // set up to load divisor latch
    outp(uart_io_port + 0, divisor & 0xff); // lsb
    outp(uart_io_port + 1, divisor >> 8);   // msb
    outp(uart_io_port + 3, 3);              // 8N1
    outp(uart_io_port + 2, 0x07);           // enable FIFO, clear both, 1-byte rx threshold
    outp(uart_io_port + 4, 0x3);            // drive flow control bits high

    // a 16550A reports a working FIFO in IIR[7:6], an 8250 or 16550 does not
    if ((inp(uart_io_port + 2) & 0xc0) == 0xc0) {
        uart_tx_fifo_depth = 16;
    }
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

// console_lock held
static void debug_uart_putc(char c) {
    if (uart_tx_free == 0) {
        while ((inp(uart_io_port + 5) & (1 << 5)) == 0); // THRE
        uart_tx_free = uart_tx_fifo_depth;
    }
    outp(uart_io_port + 0, c);
    uart_tx_free--;
}

// console_lock held
static void console_putc(char c, bool fb_console) {
    if (fb_console) {
        fb_console_dputc(c);
    } else if (vga_console_present()) {
        vga_console_putc(c);
    }
    debug_uart_putc(c);
}

void platform_dputc(char c) {
    const bool fb_console = fb_console_present();

    arch_interrupt_saved_state_t state = spin_lock_irqsave(&console_lock);
    if (c == '\n') {
        console_putc('\r', fb_console);
    }
    console_putc(c, fb_console);
    spin_unlock_irqrestore(&console_lock, state);
}

int platform_dgetc(char *c, bool wait) {
    // whoever reads input is looking at the cursor
    if (!fb_console_present() && vga_console_present()) {
        arch_interrupt_saved_state_t state = spin_lock_irqsave(&console_lock);
        vga_console_sync_cursor();
        spin_unlock_irqrestore(&console_lock, state);
    }
    return cbuf_read_char(&console_input_buf, c, wait);
}
