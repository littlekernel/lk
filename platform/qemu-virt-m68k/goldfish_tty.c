/*
 * Copyright (c) 2021 Travis Geiselbrecht
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
#include <platform/virt.h>
#include <sys/types.h>

#include "platform_p.h"

// goldfish tty
// from https://github.com/qemu/qemu/blob/master/hw/char/goldfish_tty.c
volatile unsigned int * const goldfish_tty_base = (void *)VIRT_GF_TTY_MMIO_BASE;

// registers
enum {
    REG_PUT_CHAR      = 0x00,
    REG_BYTES_READY   = 0x04,
    REG_CMD           = 0x08,
    REG_DATA_PTR      = 0x10,
    REG_DATA_LEN      = 0x14,
    REG_DATA_PTR_HIGH = 0x18,
    REG_VERSION       = 0x20,
};

// commands

enum {
    CMD_INT_DISABLE   = 0x00,
    CMD_INT_ENABLE    = 0x01,
    CMD_WRITE_BUFFER  = 0x02,
    CMD_READ_BUFFER   = 0x03,
};

#define RXBUF_SIZE 128
static char uart_rx_buf_data[RXBUF_SIZE];
static cbuf_t uart_rx_buf;

static char transfer_buf[1]; // static pointer used to transfer MMIO data

#if 0

// simple 16550 driver for the emulated serial port on qemu riscv virt machine

static volatile uint8_t *const uart_base = (uint8_t *)UART0_BASE_VIRT;


static inline uint8_t uart_read_8(size_t offset) {
    return uart_base[offset];
}

static inline void uart_write_8(size_t offset, uint8_t val) {
    uart_base[offset] = val;
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

#endif

static void write_reg(int reg, uint32_t val) {
    goldfish_tty_base[reg / 4] = val;
}

static uint32_t read_reg(int reg) {
    return goldfish_tty_base[reg / 4];
}

void goldfish_tty_early_init(void) {
    // make sure irqs are disabled
    write_reg(REG_CMD, CMD_INT_DISABLE);

    // set up the transfer buffer for receives
    write_reg(REG_DATA_PTR, (uint32_t)transfer_buf);
    write_reg(REG_DATA_PTR_HIGH, 0);
    write_reg(REG_DATA_LEN, sizeof(transfer_buf));
}

void goldfish_tty_init(void) {
    /* finish uart init to get rx going */
    cbuf_initialize_etc(&uart_rx_buf, RXBUF_SIZE, uart_rx_buf_data);

#if 0
    register_int_handler(IRQ_UART0, uart_irq_handler, NULL);

    uart_write_8(1, 0x1); // enable receive data available interrupt

    unmask_interrupt(IRQ_UART0);
#endif
}

void uart_putc(char c) {
    write_reg(REG_PUT_CHAR, c);
}

int uart_getc(char *c, bool wait) {
#if 0
    return cbuf_read_char(&uart_rx_buf, c, wait);
#else
    return platform_pgetc(c, false);
#endif
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
void platform_pputc(char c) {
    return uart_putc(c);
}

int platform_pgetc(char *c, bool wait) {
    // use a DMA read of one byte if a byte is ready
    if (read_reg(REG_BYTES_READY) > 0) {
        write_reg(REG_CMD, CMD_READ_BUFFER);
        *c = transfer_buf[0];
        return 0;
    }
    return -1;
}
