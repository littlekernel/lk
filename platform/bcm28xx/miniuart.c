/*
 * Copyright (c) 2016 Gurjant Kalsi
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

// TODO(gkalsi): Unify the two UART codepaths and use the port parameter to
// select between the real uart and the miniuart.

#include <assert.h>
#include <kernel/thread.h>
#include <lib/cbuf.h>
#include <platform/bcm28xx.h>
#include <platform/debug.h>
#include <platform/interrupts.h>
#include <reg.h>
#include <stdio.h>
#include <trace.h>

#define RXBUF_SIZE 16

static cbuf_t uart_rx_buf;

struct bcm283x_mu_regs {
    uint32_t io;
    uint32_t ier;
    uint32_t iir;
    uint32_t lcr;
    uint32_t mcr;
    uint32_t lsr;
    uint32_t msr;
    uint32_t scratch;
    uint32_t cntl;
    uint32_t stat;
    uint32_t baud;
};

struct bcm283x_aux_regs {
    uint32_t auxirq;
    uint32_t auxenb;
};

#define AUX_IRQ_MINIUART     (1 << 0)
#define AUX_ENB_MINIUART     (1 << 0)

#define MU_IIR_BYTE_AVAIL    (1 << 2)   // For reading
#define MU_IIR_CLR_XMIT_FIFO (1 << 2)   // For writing.
#define MU_IIR_CLR_RECV_FIFO (1 << 1)

#define MU_IIR_EN_RX_IRQ     (1 << 0)   // Enable the recv interrupt.

#define MU_LSR_TX_EMPTY      (1 << 5)

static enum handler_return aux_irq(void *arg)
{
    volatile struct bcm283x_mu_regs *mu_regs =
        (struct bcm283x_mu_regs *)MINIUART_BASE;
    volatile struct bcm283x_aux_regs *aux_regs =
        (struct bcm283x_aux_regs *)AUX_BASE;

    // Make sure this interrupt is intended for the miniuart.
    uint32_t auxirq = readl(&aux_regs->auxirq);
    if ((auxirq & AUX_IRQ_MINIUART) == 0) {
        return INT_NO_RESCHEDULE;
    }

    bool resched = false;

    while (true) {
        uint32_t iir = readl(&mu_regs->iir);
        if ((iir & MU_IIR_BYTE_AVAIL) == 0) break;

        resched = true;
        char ch = readl(&mu_regs->io);
        cbuf_write_char(&uart_rx_buf, ch, false);
    }

    return resched ? INT_RESCHEDULE : INT_NO_RESCHEDULE;
}

int uart_putc(int port, char c)
{
    // There's only one UART for now.
    // TODO(gkalsi): Unify the two UART code paths using the port.
    struct bcm283x_mu_regs *regs = (struct bcm283x_mu_regs *)MINIUART_BASE;

    /* Wait until there is space in the FIFO */
    while (!(readl(&regs->lsr) & MU_LSR_TX_EMPTY))
        ;

    /* Send the character */
    writel(c, &regs->io);    

    return 1;
}

void uart_init(void)
{
    volatile struct bcm283x_mu_regs *mu_regs =
        (struct bcm283x_mu_regs *)MINIUART_BASE;
    volatile struct bcm283x_aux_regs *aux_regs =
        (struct bcm283x_aux_regs *)AUX_BASE;

    // Create circular buffer to hold received data.
    cbuf_initialize(&uart_rx_buf, RXBUF_SIZE);

    // AUX Interrupt handler handles interrupts for SPI1, SPI2, and miniuart
    // Interrupt handler must decode IRQ.
    register_int_handler(INTERRUPT_AUX, &aux_irq, NULL);

    // Enable the Interrupt.
    unmask_interrupt(INTERRUPT_AUX);

    writel(MU_IIR_CLR_RECV_FIFO | MU_IIR_CLR_XMIT_FIFO, &mu_regs->iir);

    // Enable the miniuart peripheral. This also enables Miniuart register
    // access. It's likely that the VideoCore chip already enables this 
    // peripheral for us, but we hit the enable bit just to be sure.
    writel(AUX_ENB_MINIUART, &aux_regs->auxenb);

    // Enable the receive interrupt on the UART peripheral. 
    writel(MU_IIR_EN_RX_IRQ, &mu_regs->ier);
}

void uart_init_early(void)
{
}

int uart_getc(int port, bool wait)
{
    cbuf_t *rxbuf = &uart_rx_buf;

    char c;
    if (cbuf_read_char(rxbuf, &c, wait) == 1)
        return c;

    return -1;
}

void uart_flush_tx(int port)
{
    volatile struct bcm283x_mu_regs *mu_regs =
        (struct bcm283x_mu_regs *)MINIUART_BASE;
    writel(MU_IIR_CLR_XMIT_FIFO, &mu_regs->iir);
}

void uart_flush_rx(int port)
{
    volatile struct bcm283x_mu_regs *mu_regs =
        (struct bcm283x_mu_regs *)MINIUART_BASE;
    writel(MU_IIR_CLR_RECV_FIFO, &mu_regs->iir);
}

void uart_init_port(int port, uint baud)
{
}



