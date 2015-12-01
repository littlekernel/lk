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

/*
 * The plain mips target for qemu has an emulated PC style UART mapped
 * into the ISA io port apterture at 0x14000000
 */
#define ISA_IO_BASE ((volatile uint8_t *)0x14000000)
#define UART_PORT_BASE (0x3f8)

static inline void isa_write_8(uint16_t port, uint8_t val)
{
    volatile uint8_t *addr = ISA_IO_BASE + port;

    *addr = val;
}

static inline uint8_t isa_read_8(uint16_t port)
{
    volatile uint8_t *addr = ISA_IO_BASE + port;

    return *addr;
}

void uart_putc(char c)
{
    isa_write_8(UART_PORT_BASE + 0, c);
}

int uart_getc(bool wait)
{
    while ((isa_read_8(UART_PORT_BASE + 5) & (1<<0)) == 0)
        ;

    return isa_read_8(UART_PORT_BASE + 0);
}

void platform_dputc(char c)
{
    if (c == '\n')
        uart_putc('\r');
    uart_putc(c);
}

int platform_dgetc(char *c, bool wait)
{
    for (;;) {
        int ret = uart_getc(wait);
        if (ret >= 0) {
            *c = ret;
            return 0;
        }

        if (!wait)
            return -1;

        thread_yield();
    }
}

