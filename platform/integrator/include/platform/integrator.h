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
#ifndef __INTEGRATOR_H
#define __INTEGRATOR_H

/* memory map */
#define SDRAM_BASE	0x00000000

#define INTEGRATOR_CORE_REG_BASE  0x10000000
#define INTEGRATOR_SYS_REG_BASE   0x11000000
#define INTEGRATOR_EBI_REG_BASE   0x12000000
#define INTEGRATOR_TIMER_REG_BASE 0x13000000
#define INTEGRATOR_INT_REG_BASE   0x14000000
#define INTEGRATOR_UART0_REG_BASE 0x16000000
#define INTEGRATOR_UART1_REG_BASE 0x17000000
#define INTEGRATOR_LEDS_REG_BASE  0x1a000000
#define INTEGRATOR_GPIO_REG_BASE  0x1b000000

/* uart stuff */
#define PL011_UARTDR (0)
#define PL011_UARTRSR (1)
#define PL011_UARTECR (1)
#define PL011_UARTFR (6)
#define PL011_UARTILPR (8)
#define PL011_UARTIBRD (9)
#define PL011_UARTFBRD (10)
#define PL011_UARTLCR_H (11)
#define PL011_UARTCR (12)
#define PL011_UARTIFLS (13)
#define PL011_UARTIMSC (14)
#define PL011_UARTTRIS (15)
#define PL011_UARTTMIS (16)
#define PL011_UARTICR (17)
#define PL011_UARTMACR (18)

#define INT_VECTORS 32 // XXX just made this up

#endif

