/*
 * Copyright (c) 2015 Travis Geiselbrecht
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
#include <reg.h>
#include <trace.h>
#include <lib/cbuf.h>
#include <lk/init.h>
#include <kernel/thread.h>
#include <platform.h>
#include <platform/interrupts.h>
#include <sys/types.h>
#include <target/microblaze-config.h>

#define LOCAL_TRACE 0

#define R_RX            0
#define R_TX            1
#define R_STATUS        2
#define R_CTRL          3
#define R_MAX           4

#define STATUS_RXVALID    0x01
#define STATUS_RXFULL     0x02
#define STATUS_TXEMPTY    0x04
#define STATUS_TXFULL     0x08
#define STATUS_IE         0x10
#define STATUS_OVERRUN    0x20
#define STATUS_FRAME      0x40
#define STATUS_PARITY     0x80

#define CONTROL_RST_TX    0x01
#define CONTROL_RST_RX    0x02
#define CONTROL_IE        0x10

#define UART_REG(reg) (*REG32(UARTLITE_BASEADDR + (reg) * 4))

#define RXBUF_SIZE 128
static cbuf_t uart_rx_buf;

void uartlite_putc(char c)
{
    while (UART_REG(R_STATUS) & STATUS_TXFULL)
        ;
    UART_REG(R_TX) = c;
}

int uartlite_getc(bool wait)
{
#if 0
    char c;
    if (cbuf_read_char(&uart_rx_buf, &c, wait) == 1)
        return c;
#else
    do {
        if (UART_REG(R_STATUS) & STATUS_RXVALID) {
            char c = UART_REG(R_RX);
            return c;
        }
    } while (wait);
#endif

    return -1;
}

enum handler_return uartlite_irq(void *arg)
{
    bool resched = false;

    /* while receive fifo not empty, read a char */
    while (UART_REG(R_STATUS) & STATUS_RXVALID) {
        char c = UART_REG(R_RX);
        cbuf_write_char(&uart_rx_buf, c, false);

        resched = true;
    }

    return resched ? INT_RESCHEDULE : INT_NO_RESCHEDULE;
}

static void uartlite_init(uint level)
{
    TRACE;

    //UART_REG(R_CTRL) = CONTROL_RST_TX | CONTROL_RST_RX;
//    UART_REG(R_CTRL) |= CONTROL_IE;

    cbuf_initialize(&uart_rx_buf, RXBUF_SIZE);

//    register_int_handler(UARTLITE_IRQ, uartlite_irq, NULL);
//    unmask_interrupt(UARTLITE_IRQ);
}

LK_INIT_HOOK(uartlite, uartlite_init, LK_INIT_LEVEL_PLATFORM);

