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
#include <lib/cmdline.h>
#include <lk/err.h>
#include <lk/reg.h>
#include <platform.h>
#include <platform/debug.h>
#include <platform/interrupts.h>
#include <platform/keyboard.h>
#include <platform/pc.h>
#include <platform/vga_console.h>
#include <stdarg.h>
#include <string.h>

#include <platform/fb_console.h>

#include "platform_p.h"

#ifndef DEBUG_BAUD_RATE
#define DEBUG_BAUD_RATE 115200
#endif
#ifndef DEBUG_COM_PORT
#define DEBUG_COM_PORT 1
#endif

static const int uart_baud_rate = DEBUG_BAUD_RATE;

static const uint16_t com_ports[4] = { COM1_REG, COM2_REG, COM3_REG, COM4_REG };

// the console uart, DEBUG_COM_PORT until platform_init_debug() reads pc.console
static uint16_t uart_io_port = com_ports[DEBUG_COM_PORT - 1];
static bool uart_present;

static unsigned int com_irq(unsigned int com) {
    return (com == 1 || com == 3) ? INT_COM1_COM3 : INT_COM2_COM4;
}

cbuf_t console_input_buf;

/*
 * Serializes console output, and guards the uart, VGA and framebuffer console
 * state. Output that re-enters on the cpu already holding it, from a fault or NMI
 * partway through a character, goes ahead without it so that the crash report
 * still comes out instead of deadlocking.
 */
static spin_lock_t console_lock = SPIN_LOCK_INITIAL_VALUE;
static int console_lock_owner = -1;

/*
 * THRE with the FIFO on means the whole FIFO is free, so the line status only has
 * to be polled once per FIFO's worth of output. Each port access is a trap under a
 * hypervisor.
 */
static unsigned int uart_tx_fifo_depth = 1;
static unsigned int uart_tx_free;

extern uint32_t _multiboot2_info;

// 16550 registers, offsets from the base port
#define UART_RBR 0 // receive buffer / transmit holding
#define UART_IER 1
#define UART_IIR 2 // read: interrupt id; write: FIFO control
#define UART_LCR 3
#define UART_MCR 4
#define UART_LSR 5
#define UART_SCR 7

#define UART_LSR_DR   (1u << 0)
#define UART_LSR_THRE (1u << 5)

// The receive irq drains at most this many bytes, so a port that reads back all ones
// (data ready stuck on) cannot hold the cpu.
#define UART_RX_DRAIN_LIMIT 64

static enum handler_return uart_irq_handler(void *arg) {
    bool resched = false;

    for (int i = 0; i < UART_RX_DRAIN_LIMIT && (inp(uart_io_port + UART_LSR) & UART_LSR_DR); i++) {
        unsigned char c = inp(uart_io_port + UART_RBR);
        cbuf_write_char(&console_input_buf, c, false);
        resched = true;
    }

    return resched ? INT_RESCHEDULE : INT_NO_RESCHEDULE;
}

const char *pc_uart_type_name(enum pc_uart_type type) {
    switch (type) {
        case PC_UART_NONE:    return "none";
        case PC_UART_8250:    return "8250";
        case PC_UART_16450:   return "16450";
        case PC_UART_16550:   return "16550";
        case PC_UART_16550A:  return "16550A";
    }
    return "unknown";
}

// Identify the uart at port, the classic way. A port with nothing behind it floats to
// all ones, so the interrupt enable register's low nibble failing to hold 0 and then 0xf
// means nothing is there. The scratch register arrived with the 16450, the FIFO with the
// 16550, and a FIFO that works with the 16550A. The port is left with interrupts and the
// FIFO off, DLAB clear.
enum pc_uart_type pc_uart_probe(uint16_t port) {
    outp(port + UART_LCR, 0x03);
    outp(port + UART_IER, 0);
    if (inp(port + UART_IER) & 0x0f) {
        return PC_UART_NONE;
    }
    outp(port + UART_IER, 0x0f);
    const uint8_t ier = inp(port + UART_IER) & 0x0f;
    outp(port + UART_IER, 0);
    if (ier != 0x0f) {
        return PC_UART_NONE;
    }

    outp(port + UART_SCR, 0x55);
    const bool scr_55 = inp(port + UART_SCR) == 0x55;
    outp(port + UART_SCR, 0xaa);
    const bool scratch = scr_55 && inp(port + UART_SCR) == 0xaa;

    outp(port + UART_IIR, 0x07); // enable and clear the FIFO, if there is one
    const uint8_t fifo = inp(port + UART_IIR) & 0xc0;
    outp(port + UART_IIR, 0);

    if (fifo == 0xc0) {
        return PC_UART_16550A;
    }
    if (fifo == 0x80) {
        return PC_UART_16550; // FIFO present but unusable
    }
    return scratch ? PC_UART_16450 : PC_UART_8250;
}

// program the console uart; probes it first and leaves it alone if nothing answers
static void uart_configure(uint16_t port) {
    const enum pc_uart_type type = pc_uart_probe(port);

    uart_io_port = port;
    uart_present = (type != PC_UART_NONE);
    uart_tx_fifo_depth = 1;
    uart_tx_free = 0;
    if (!uart_present) {
        return;
    }

    const int divisor = 115200 / uart_baud_rate;

    outp(port + UART_IER, 0);              // mask all irqs
    outp(port + UART_LCR, 0x80);           // set up to load divisor latch
    outp(port + UART_RBR, divisor & 0xff); // lsb
    outp(port + UART_IER, divisor >> 8);   // msb
    outp(port + UART_LCR, 3);              // 8N1
    outp(port + UART_MCR, 0x3);            // drive flow control bits high

    if (type == PC_UART_16550A) {
        outp(port + UART_IIR, 0x07); // enable FIFO, clear both, 1-byte rx threshold
        uart_tx_fifo_depth = 16;
    }
}

void platform_init_debug_early(void) {
    uart_configure(uart_io_port);
}

// pc.console=comN moves the console off DEBUG_COM_PORT once the command line is known
static unsigned int console_com_port(void) {
    char buf[8];
    if (cmdline_get_string("pc.console", buf, sizeof(buf), NULL) == NO_ERROR) {
        if (strlen(buf) == 4 && !strncmp(buf, "com", 3) && buf[3] >= '1' && buf[3] <= '4') {
            return buf[3] - '0';
        }
        dprintf(ALWAYS, "PC: ignoring pc.console=%s, want com1-com4\n", buf);
    }
    return DEBUG_COM_PORT;
}

void platform_init_debug(void) {
    /* finish uart init to get rx going */
    cbuf_initialize(&console_input_buf, 1024);

    unsigned int com = console_com_port();
    if (com != DEBUG_COM_PORT) {
        if (pc_uart_probe(com_ports[com - 1]) == PC_UART_NONE) {
            dprintf(ALWAYS, "PC: pc.console=com%u but no uart answers at %#x, staying on com%u\n",
                    com, com_ports[com - 1], DEBUG_COM_PORT);
            com = DEBUG_COM_PORT;
        } else {
            dprintf(INFO, "PC: console moving to com%u\n", com);
            arch_interrupt_saved_state_t state = arch_interrupt_save();
            spin_lock(&console_lock);
            uart_configure(com_ports[com - 1]);
            spin_unlock(&console_lock);
            arch_interrupt_restore(state);
        }
    }

    if (!uart_present) {
        return;
    }

    register_int_handler(com_irq(com), uart_irq_handler, NULL);
    unmask_interrupt(com_irq(com));

    outp(uart_io_port + UART_IER, 0x1); // enable receive data available interrupt

    // modem control register: Auxiliary Output 2 is another IRQ enable bit
    const uint8_t mcr = inp(uart_io_port + UART_MCR);
    outp(uart_io_port + UART_MCR, mcr | 0x8);
}

// console_lock held
static void debug_uart_putc(char c) {
    if (!uart_present) {
        return;
    }
    if (uart_tx_free == 0) {
        while ((inp(uart_io_port + UART_LSR) & UART_LSR_THRE) == 0);
        uart_tx_free = uart_tx_fifo_depth;
    }
    outp(uart_io_port + UART_RBR, c);
    uart_tx_free--;
}

// interrupts disabled; returns false if this cpu already held the lock
static bool console_lock_acquire(void) {
    const int cpu = (int)arch_curr_cpu_num();
    if (console_lock_owner == cpu) {
        return false;
    }
    spin_lock(&console_lock);
    console_lock_owner = cpu;
    return true;
}

static void console_lock_release(bool acquired) {
    if (acquired) {
        console_lock_owner = -1;
        spin_unlock(&console_lock);
    }
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

    arch_interrupt_saved_state_t state = arch_interrupt_save();
    const bool acquired = console_lock_acquire();
    if (c == '\n') {
        console_putc('\r', fb_console);
    }
    console_putc(c, fb_console);
    console_lock_release(acquired);
    arch_interrupt_restore(state);
}

int platform_dgetc(char *c, bool wait) {
    // whoever reads input is looking at the cursor
    if (!fb_console_present() && vga_console_present()) {
        arch_interrupt_saved_state_t state = arch_interrupt_save();
        const bool acquired = console_lock_acquire();
        vga_console_sync_cursor();
        console_lock_release(acquired);
        arch_interrupt_restore(state);
    }
    return cbuf_read_char(&console_input_buf, c, wait);
}

/*
 * Panic time i/o, for the panic shell: the polled uart only, without console_lock
 * and without touching the screens, so it works whatever state a crashed cpu left
 * the console in. Input is polled since the panic shell runs with interrupts off.
 */
static void panic_uart_putc(char c) {
    if (!uart_present) {
        return;
    }
    while ((inp(uart_io_port + UART_LSR) & UART_LSR_THRE) == 0);
    outp(uart_io_port + UART_RBR, c);
}

void platform_pputc(char c) {
    if (c == '\n') {
        panic_uart_putc('\r');
    }
    panic_uart_putc(c);
}

int platform_pgetc(char *c, bool wait) {
    do {
        if (uart_present && (inp(uart_io_port + UART_LSR) & UART_LSR_DR)) {
            *c = inp(uart_io_port + UART_RBR);
            return 0;
        }
    } while (wait);

    return ERR_NOT_READY;
}
