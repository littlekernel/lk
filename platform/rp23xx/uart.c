// Copyright (c) 2020 Brian Swetland
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#include <arch/arm/cm.h>
#include <assert.h>
#include <lib/cbuf.h>

#if CONSOLE_HAS_INPUT_BUFFER
#include <lib/io.h>
#endif

#include <hardware/regs/uart.h>

// Avoid symbol collisions with LK's uart_putc/uart_getc/uart_init API while using SDK helpers.
#define uart_getc pico_uart_getc
#define uart_putc pico_uart_putc
#include <hardware/uart.h>
#include <target/debugconfig.h>
#undef uart_putc
#undef uart_getc

#define UART_PORT_COUNT 2
#define RXBUF_SIZE      128

static char uart_rx_buf_data[UART_PORT_COUNT][RXBUF_SIZE];
static cbuf_t uart_rx_buf[UART_PORT_COUNT];
static volatile bool uart_rx_irq_enabled[UART_PORT_COUNT];

static int rp23xx_debug_uart_port(void) {
    return 0;
}

static uart_inst_t *rp23xx_get_uart(int port) {
    if (port < 0 || port >= UART_PORT_COUNT) {
        ASSERT(false);
        return uart0;
    }
    return uart_get_instance((uint)port);
}

static cbuf_t *rp23xx_get_rxbuf(int port) {
#if CONSOLE_HAS_INPUT_BUFFER
    if (port == rp23xx_debug_uart_port()) {
        return &console_input_cbuf;
    }
#endif
    return &uart_rx_buf[port];
}

static void rp23xx_uart_enable_rx_irq(int port, bool enabled) {
    uart_inst_t *uart = rp23xx_get_uart(port);
    IRQn_Type irqn = (port == 0) ? UART0_IRQ_IRQn : UART1_IRQ_IRQn;

    uart_set_irqs_enabled(uart, enabled, false);
    if (enabled) {
        NVIC_EnableIRQ(irqn);
    } else {
        NVIC_DisableIRQ(irqn);
    }
    uart_rx_irq_enabled[port] = enabled;
}

static void rp23xx_uart_init_rx_path(int port, bool enable_irq) {
#if CONSOLE_HAS_INPUT_BUFFER
    if (port != rp23xx_debug_uart_port())
#endif
    {
        cbuf_initialize_etc(&uart_rx_buf[port], RXBUF_SIZE, uart_rx_buf_data[port]);
    }
    rp23xx_uart_enable_rx_irq(port, enable_irq);
}

static void rp23xx_uart_drain_rx_fifo(int port, bool canresched) {
    uart_inst_t *uart = rp23xx_get_uart(port);
    cbuf_t *rxbuf = rp23xx_get_rxbuf(port);

    while (uart_is_readable(uart)) {
        char c = (char)uart_get_hw(uart)->dr;
        if (cbuf_write_char(rxbuf, c, canresched) != 1) {
            // Leave bytes in the hardware FIFO until userspace consumes data.
            rp23xx_uart_enable_rx_irq(port, false);
            break;
        }
    }

    uart_get_hw(uart)->icr = UART_UARTICR_RTIC_BITS | UART_UARTICR_RXIC_BITS;
}

static void rp23xx_uart_rx_irq(int port) {
    arm_cm_irq_entry();
    rp23xx_uart_drain_rx_fifo(port, true);
    arm_cm_irq_exit(true);
}

void UART0_IRQ_IRQHandler(void) {
    rp23xx_uart_rx_irq(0);
}

void UART1_IRQ_IRQHandler(void) {
    rp23xx_uart_rx_irq(1);
}

int uart_putc(int port, char c) {
    pico_uart_putc(rp23xx_get_uart(port), c);
    return 1;
}

int uart_getc(int port, bool wait) {
    cbuf_t *rxbuf = rp23xx_get_rxbuf(port);
    char c;

    if (cbuf_read_char(rxbuf, &c, wait) == 0) {
        return -1;
    }

    if (!uart_rx_irq_enabled[port] && cbuf_space_avail(rxbuf) > (RXBUF_SIZE / 2)) {
        rp23xx_uart_enable_rx_irq(port, true);
    }

    return (uint8_t)c;
}

void uart_flush_tx(int port) {
    uart_tx_wait_blocking(rp23xx_get_uart(port));
}

void uart_flush_rx(int port) {
    uart_inst_t *uart = rp23xx_get_uart(port);
    cbuf_reset(rp23xx_get_rxbuf(port));
    while (uart_is_readable(uart)) {
        (void)uart_get_hw(uart)->dr;
    }
    uart_get_hw(uart)->icr = UART_UARTICR_RTIC_BITS | UART_UARTICR_RXIC_BITS;
}

void uart_init_port(int port, uint baud) {
    uart_init(rp23xx_get_uart(port), baud);
    rp23xx_uart_init_rx_path(port, true);
}
