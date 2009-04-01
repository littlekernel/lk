/*
 * Copyright (c) 2009 Corey Tabaka
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
#ifndef __PLATFORM_PC_H
#define __PLATFORM_PC_H

#include <platform/pc/memmap.h>
#include <platform/pc/iomap.h>

/* NOTE: keep arch/x86/crt0.S in sync with these definitions */

/* interrupts */
#define INT_VECTORS 0x31

/* defined interrupts */
#define INT_BASE			0x20
#define INT_PIT				0x20
#define INT_KEYBOARD		0x21
#define INT_PIC2			0x22

#define INT_BASE2			0x28
#define INT_CMOSRTC			0x28
#define INT_PS2MOUSE		0x2c
#define INT_IDE0			0x2e
#define INT_IDE1			0x2f

/* exceptions */
#define INT_DIVIDE_0		0x00
#define INT_DEBUG_EX		0x01
#define INT_INVALID_OP		0x06
#define INT_DEV_NA_EX		0x07

/* faults */
#define INT_STACK_FAULT		0x0c
#define INT_GP_FAULT		0x0d
#define INT_PAGE_FAULT		0x0e

/* APIC vectors */
#define INT_APIC_TIMER		0x22

#define INT_SYSCALL			0x30

/* PIC remap bases */
#define PIC1_BASE 0x20
#define PIC2_BASE 0x28

#endif

