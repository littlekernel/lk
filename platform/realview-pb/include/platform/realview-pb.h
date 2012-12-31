/*
 * Copyright (c) 2012 Travis Geiselbrecht
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
#ifndef __REALVIEW_PB_H
#define __REALVIEW_PB_H

#include <reg.h>

/* hardware base addresses */
#define UART0 (0x10009000)
#define UART1 (0x1000a000)
#define UART2 (0x1000b000)
#define UART3 (0x1000c000)

#define TIMER0 (0x10011000)
#define TIMER1 (0x10011020)
#define TIMER2 (0x10012000)
#define TIMER3 (0x10012020)
#define TIMER4 (0x10018000)
#define TIMER5 (0x10018020)
#define TIMER6 (0x10019000)
#define TIMER7 (0x10019020)

#define GIC0   (0x1e000000)
#define GIC1   (0x1e010000)
#define GIC2   (0x1e020000)
#define GIC3   (0x1e030000)
#define GICBASE(n) (GIC0 + (n) * 0x10000)

/* interrupts */
#define TIMER01_INT 36
#define TIMER23_INT 37
#define UART0_INT 44
#define UART1_INT 45
#define UART2_INT 46
#define UART3_INT 47
#define TIMER45_INT 73
#define TIMER67_INT 74

#define MAX_INT 96

#endif

