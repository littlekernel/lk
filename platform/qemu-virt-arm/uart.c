/*
 * Copyright (c) 2014-2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/reg.h>
#include <stdio.h>
#include <lk/trace.h>
#include <lib/cbuf.h>
#include <dev/uart.h>
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

#define RXBUF_SIZE 16
#define NUM_UART 1

static cbuf_t uart_rx_buf[NUM_UART];

// ARM & ARM64 specific register accessors that use specific instructions to avoid
// trapping into a virtual machine.
// TODO: make a generic version of this since trapping nonstandard instructions is
// a general problem while running under a VM.
#if __aarch64__
static inline uint32_t __arm64_read_reg32(uint32_t *addr) {
    uint32_t val;
    __asm__ volatile("ldr %w0, %1" : "=r"(val) : "m"(*addr) : "memory");
    return val;
}

static inline void __arm64_write_reg32(uint32_t *addr, uint32_t val) {
    __asm__ volatile("str %w0, %1" :: "r"(val), "m"(*addr) : "memory");
}
#elif __arm__
static inline uint32_t __arm64_read_reg32(uint32_t *addr) {
    uint32_t val;
    __asm__ volatile("ldr %0, %1" : "=r"(val) : "m"(*addr) : "memory");
    return val;
}

static inline void __arm64_write_reg32(uint32_t *addr, uint32_t val) {
    __asm__ volatile("str %0, %1" :: "r"(val), "m"(*addr) : "memory");
}
#else
#error need for this arch
#endif

static inline void write_uart_reg(uintptr_t base, size_t offset, uint32_t val) {
    __arm64_write_reg32((uint32_t *)(base + offset), val);
}

static inline uint32_t read_uart_reg(uintptr_t base, size_t offset) {
    return __arm64_read_reg32((uint32_t *)(base + offset));
}

static inline void set_uart_reg_bits(uintptr_t base, size_t offset, uint32_t bits) {
    write_uart_reg(base, offset, read_uart_reg(base, offset) | bits);
}

static inline void clear_uart_reg_bits(uintptr_t base, size_t offset, uint32_t bits) {
    write_uart_reg(base, offset, read_uart_reg(base, offset) & ~bits);
}

static inline uintptr_t uart_to_ptr(unsigned int n) {
    switch (n) {
        default:
        case 0:
            return UART_BASE;
    }
}

static enum handler_return uart_irq(void *arg) {
    bool resched = false;
    uint port = (uintptr_t)arg;
    uintptr_t base = uart_to_ptr(port);

    /* read interrupt status and mask */
    uint32_t isr = read_uart_reg(base, UART_TMIS);

    if (isr & (1<<4)) { // rxmis
        cbuf_t *rxbuf = &uart_rx_buf[port];

        /* while fifo is not empty, read chars out of it */
        while ((read_uart_reg(base, UART_TFR) & (1<<4)) == 0) {
#if CONSOLE_HAS_INPUT_BUFFER
            if (port == DEBUG_UART) {
                char c = read_uart_reg(base, UART_DR);
                cbuf_write_char(&console_input_cbuf, c, false);
            } else
#endif
            {
                /* if we're out of rx buffer, mask the irq instead of handling it */
                if (cbuf_space_avail(rxbuf) == 0) {
                    clear_uart_reg_bits(base, UART_IMSC, (1<<4)); // !rxim
                    break;
                }

                char c = read_uart_reg(base, UART_DR);
                cbuf_write_char(rxbuf, c, false);
            }

            resched = true;
        }
    }

    return resched ? INT_RESCHEDULE : INT_NO_RESCHEDULE;
}

void uart_init(void) {
    for (size_t i = 0; i < NUM_UART; i++) {
        uintptr_t base = uart_to_ptr(i);

        // create circular buffer to hold received data
        cbuf_initialize(&uart_rx_buf[i], RXBUF_SIZE);

        // assumes interrupts are contiguous
        register_int_handler(UART0_INT + i, &uart_irq, (void *)i);

        // clear all irqs
        write_uart_reg(base, UART_ICR, 0x3ff);

        // set fifo trigger level
        write_uart_reg(base, UART_IFLS, 0); // 1/8 rxfifo, 1/8 txfifo

        // enable rx interrupt
        write_uart_reg(base, UART_IMSC, 1<<4); // rxim

        // enable receive
        set_uart_reg_bits(base, UART_CR, (1<<9)); // rxen

        // enable interrupt
        unmask_interrupt(UART0_INT + i);
    }
}

void uart_init_early(void) {
    for (size_t i = 0; i < NUM_UART; i++) {
        write_uart_reg(uart_to_ptr(i), UART_CR, (1<<8)|(1<<0)); // tx_enable, uarten
    }
}

int uart_putc(int port, char c) {
    uintptr_t base = uart_to_ptr(port);

    /* spin while fifo is full */
    while (read_uart_reg(base, UART_TFR) & (1<<5))
        ;
    write_uart_reg(base, UART_DR, c);

    return 1;
}

int uart_getc(int port, bool wait) {
    cbuf_t *rxbuf = &uart_rx_buf[port];

    char c;
    if (cbuf_read_char(rxbuf, &c, wait) == 1) {
        write_uart_reg(uart_to_ptr(port), UART_IMSC, (1<<4)); // rxim
        return c;
    }

    return -1;
}

/* panic-time getc/putc */
int uart_pputc(int port, char c) {
    uintptr_t base = uart_to_ptr(port);

    /* spin while fifo is full */
    while (read_uart_reg(base, UART_TFR) & (1<<5))
        ;
    write_uart_reg(base, UART_DR, c);

    return 1;
}

int uart_pgetc(int port) {
    uintptr_t base = uart_to_ptr(port);

    if ((read_uart_reg(base, UART_TFR) & (1<<4)) == 0) {
        return read_uart_reg(base, UART_DR);
    } else {
        return -1;
    }
}


void uart_flush_tx(int port) {
}

void uart_flush_rx(int port) {
}

void uart_init_port(int port, uint baud) {
}

