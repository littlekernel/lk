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
#pragma once

#include <stdint.h>

/*
 * The plain mips target for qemu has an emulated PC style UART mapped
 * into the ISA io port apterture at 0x14000000
 */
#define ISA_IO_BASE ((volatile uint8_t *)0x14000000 + 0x80000000)
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

#define INT_VECTORS 8
