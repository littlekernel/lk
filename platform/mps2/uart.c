/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <arch/arm/cm.h>
#include <assert.h>
#include <dev/uart.h>
#include <lib/cbuf.h>
#include <platform/mps2.h>
#include <stdint.h>

/*
 * ARM CMSDK APB UART, as modelled by qemu's hw/char/cmsdk-apb-uart.c. The
 * same block appears on every MPS2 and MPS3 FPGA image, only the base address
 * and the interrupt numbers change.
 */
typedef struct {
    volatile uint32_t data;
    volatile uint32_t state;
    volatile uint32_t ctrl;
    volatile uint32_t intstatus;
    volatile uint32_t bauddiv;
} cmsdk_uart_t;

#define UART_STATE_TXFULL (1u << 0)
#define UART_STATE_RXFULL (1u << 1)

#define UART_CTRL_TX_EN     (1u << 0)
#define UART_CTRL_RX_EN     (1u << 1)
#define UART_CTRL_TX_INTEN  (1u << 2)
#define UART_CTRL_RX_INTEN  (1u << 3)

#define UART_INT_TX (1u << 0)
#define UART_INT_RX (1u << 1)

#define UART_BAUD_RATE 115200

/*
 * The divisor is only used to compute the nominal line rate, and the hardware
 * rejects anything below 16. Nothing on the transmit path depends on it, but
 * an out of range value makes qemu log a guest error, so keep it sane.
 */
STATIC_ASSERT(MPS2_UART_PCLK_HZ / UART_BAUD_RATE >= 16);

#define RXBUF_SIZE 16

static cmsdk_uart_t *const uart0 = (cmsdk_uart_t *)MPS2_UART0_BASE;

static cbuf_t uart0_rx_buf;

void uart_init_early(void) {
    uart0->ctrl = 0;
    uart0->bauddiv = MPS2_UART_PCLK_HZ / UART_BAUD_RATE;
    uart0->ctrl = UART_CTRL_TX_EN | UART_CTRL_RX_EN;
}

void uart_init(void) {
    cbuf_initialize(&uart0_rx_buf, RXBUF_SIZE);

    uart0->ctrl |= UART_CTRL_RX_INTEN;
    NVIC_EnableIRQ(UARTRX0_IRQn);

    /*
     * A byte that arrived before the interrupt was enabled is sitting in the
     * receiver with nothing to announce it, and would block everything behind
     * it forever. Pend the interrupt by hand to go collect it, rather than
     * reading it here, so the handler stays the only reader of the receiver.
     */
    if (uart0->state & UART_STATE_RXFULL) {
        arm_cm_trigger_interrupt(UARTRX0_IRQn);
    }
}

static cbuf_t *mps2_get_rxbuf(int port) {
    DEBUG_ASSERT(port == 0);
    return &uart0_rx_buf;
}

static cmsdk_uart_t *mps2_get_uart(int port) {
    DEBUG_ASSERT(port == 0);
    return uart0;
}

void mps2_uart0_rx_irq(void) {
    arm_cm_irq_entry();

    /*
     * Acknowledge before draining, not after. The status bit is write one to
     * clear and the interrupt line simply follows it, so a byte that lands
     * while we are draining has to be able to leave the bit set behind us.
     * Clearing afterwards would drop that byte's interrupt while the receiver
     * stays full, which wedges the uart for good.
     */
    uart0->intstatus = UART_INT_RX;

    /*
     * Always empty the receiver, dropping the byte if there is nowhere to put
     * it. Leaving it full is not an option to hold the sender off with: the
     * receiver stops accepting bytes while it is full, and the status bit is
     * only ever set as a byte arrives, so a full receiver with the bit already
     * acked raises no further interrupts and the port never recovers.
     */
    bool resched = false;
    while (uart0->state & UART_STATE_RXFULL) {
        char c = uart0->data;

        if (cbuf_space_avail(&uart0_rx_buf) > 0) {
            cbuf_write_char(&uart0_rx_buf, c, false);
            resched = true;
        }
    }

    arm_cm_irq_exit(resched);
}

int uart_putc(int port, char c) {
    cmsdk_uart_t *uart = mps2_get_uart(port);

    while (uart->state & UART_STATE_TXFULL)
        ;
    uart->data = c;

    return 1;
}

int uart_getc(int port, bool wait) {
    cbuf_t *rxbuf = mps2_get_rxbuf(port);

    char c;
    if (cbuf_read_char(rxbuf, &c, wait) == 0) {
        return -1;
    }

    return c;
}

void uart_flush_tx(int port) {}

void uart_flush_rx(int port) {}

void uart_init_port(int port, uint baud) {
    cmsdk_uart_t *uart = mps2_get_uart(port);

    uart->bauddiv = MPS2_UART_PCLK_HZ / baud;
}
