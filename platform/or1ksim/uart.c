/*
 * Copyright (c) 2008 Travis Geiselbrecht
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
#include <debug.h>
#include <reg.h>
#include <dev/uart.h>
#include <target/debugconfig.h>
#include <platform/or1ksim.h>

struct uart_stat {
    addr_t base;
    uint32_t clk_freq;
    uint shift;
};

static struct uart_stat uart[1] = {
    { UART1_BASE, UART1_CLOCK_FREQ, 0 },
};

static inline void write_uart_reg(int port, uint reg, unsigned char data)
{
    *(volatile unsigned char *)(uart[port].base + (reg << uart[port].shift)) = data;
}

static inline unsigned char read_uart_reg(int port, uint reg)
{
    return *(volatile unsigned char *)(uart[port].base + (reg << uart[port].shift));
}

#define UART_RHR    0
#define UART_THR    0
#define UART_DLL    0
#define UART_IER    1
#define UART_DLH    1
#define UART_IIR    2
#define UART_FCR    2
#define UART_EFR    2
#define UART_LCR    3
#define UART_MCR    4
#define UART_LSR    5
#define UART_MSR    6
#define UART_TCR    6
#define UART_SPR    7
#define UART_TLR    7
#define UART_MDR1   8
#define UART_MDR2   9
#define UART_SFLSR  10
#define UART_RESUME 11
#define UART_TXFLL  10
#define UART_TXFLH  11
#define UART_SFREGL 12
#define UART_SFREGH 13
#define UART_RXFLL  12
#define UART_RXFLH  13
#define UART_BLR    14
#define UART_UASR   14
#define UART_ACREG  15
#define UART_SCR    16
#define UART_SSR    17
#define UART_EBLR   18
#define UART_MVR    19
#define UART_SYSC   20

#define LCR_8N1     0x03

#define FCR_FIFO_EN 0x01        /* Fifo enable */
#define FCR_RXSR    0x02        /* Receiver soft reset */
#define FCR_TXSR    0x04        /* Transmitter soft reset */

#define MCR_DTR     0x01
#define MCR_RTS     0x02
#define MCR_DMA_EN  0x04
#define MCR_TX_DFR  0x08

#define LCR_WLS_MSK 0x03        /* character length select mask */
#define LCR_WLS_5   0x00        /* 5 bit character length */
#define LCR_WLS_6   0x01        /* 6 bit character length */
#define LCR_WLS_7   0x02        /* 7 bit character length */
#define LCR_WLS_8   0x03        /* 8 bit character length */
#define LCR_STB     0x04        /* Number of stop Bits, off = 1, on = 1.5 or 2) */
#define LCR_PEN     0x08        /* Parity eneble */
#define LCR_EPS     0x10        /* Even Parity Select */
#define LCR_STKP    0x20        /* Stick Parity */
#define LCR_SBRK    0x40        /* Set Break */
#define LCR_BKSE    0x80        /* Bank select enable */

#define LSR_DR      0x01        /* Data ready */
#define LSR_OE      0x02        /* Overrun */
#define LSR_PE      0x04        /* Parity error */
#define LSR_FE      0x08        /* Framing error */
#define LSR_BI      0x10        /* Break */
#define LSR_THRE    0x20        /* Xmit holding register empty */
#define LSR_TEMT    0x40        /* Xmitter empty */
#define LSR_ERR     0x80        /* Error */

#define LCRVAL LCR_8N1                  /* 8 data, 1 stop, no parity */
#define MCRVAL (MCR_DTR | MCR_RTS)      /* RTS/DTR */
#define FCRVAL (FCR_FIFO_EN | FCR_RXSR | FCR_TXSR)  /* Clear & enable FIFOs */

void uart_init_port(int port, uint baud)
{
    /* clear the tx & rx fifo and disable */
    uint16_t baud_divisor = (uart[port].clk_freq / 16 / baud);

    write_uart_reg(port, UART_IER, 0);
    write_uart_reg(port, UART_LCR, LCR_BKSE | LCRVAL); // config mode A
    write_uart_reg(port, UART_DLL, baud_divisor & 0xff);
    write_uart_reg(port, UART_DLH, (baud_divisor >> 8) & 0xff);
    write_uart_reg(port, UART_LCR, LCRVAL); // operational mode
    write_uart_reg(port, UART_MCR, MCRVAL);
    write_uart_reg(port, UART_FCR, FCRVAL);
}

void uart_init_early(void)
{
    uart_init_port(DEBUG_UART, 115200);
}

void uart_init(void)
{
}

int uart_putc(int port, char c )
{
    while (!(read_uart_reg(port, UART_LSR) & (1<<6))) // wait for the last char to get out
        ;
    write_uart_reg(port, UART_THR, c);
    return 0;
}

int uart_getc(int port, bool wait)  /* returns -1 if no data available */
{
    if (wait) {
        while (!(read_uart_reg(port, UART_LSR) & (1<<0))) // wait for data to show up in the rx fifo
            ;
    } else {
        if (!(read_uart_reg(port, UART_LSR) & (1<<0)))
            return -1;
    }
    return read_uart_reg(port, UART_RHR);
}

void uart_flush_tx(int port)
{
    while (!(read_uart_reg(port, UART_LSR) & (1<<6))) // wait for the last char to get out
        ;
}

void uart_flush_rx(int port)
{
    // empty the rx fifo
    while (read_uart_reg(port, UART_LSR) & (1<<0)) {
        volatile char c = read_uart_reg(port, UART_RHR);
        (void)c;
    }
}
