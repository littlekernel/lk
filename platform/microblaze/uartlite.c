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
#include <kernel/thread.h>
#include <platform.h>
#include <platform/interrupts.h>
#include <platform/debug.h>
#include <platform/timer.h>
#include <sys/types.h>
#include <target/qemu-microblaze.h>

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

void uartlite_putc(char c)
{
    UART_REG(R_TX) = c;
}

int uartlite_getc(void)
{
    uint32_t status = UART_REG(R_STATUS);
    if (status & STATUS_RXVALID) {
        return UART_REG(R_RX);
    }
    return -1;
}



