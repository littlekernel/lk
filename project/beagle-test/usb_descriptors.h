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
#ifndef __USB_DESCRIPTORS_H
#define __USB_DESCRIPTORS_H

#include <sys/types.h>
#include <compiler.h>

#define W(w) (w & 0xff), (w >> 8)

/* top level device descriptor */
static uint8_t __ALIGNED(2) dev_descr[] = {
    0x12,           /* descriptor length */
    0x01,           /* Device Descriptor type */
    W(0x0200),      /* USB Version */
    0x00,           /* class */
    0x00,           /* subclass */
    0x00,           /* protocol */
    0x40,           /* max packet size, ept0 */
    W(0x1234),      /* vendor */
    W(0x5678), /* product */
    W(0x1000),      /* release */
    0x01,           /* manufacturer string */
    0x02,           /* product string */
    0x00,           /* serialno string */
    0x01,           /* num configs */
};

/* high/low speed device qualifier */
static uint8_t devqual_descr[] = {
	0x0a,			/* len */
	0x06,			/* Device Qualifier type */
	W(0x0200),		/* USB version */
    0x00,           /* class */
    0x00,           /* subclass */
    0x00,           /* protocol */
    0x40,           /* max packet size, ept0 */
    0x01,           /* num configs */
	0x00			/* reserved */
};

static uint8_t cfg_descr_lowspeed[] = {
    0x09,           /* Length of Cfg Descr */
    0x02,           /* Type of Cfg Descr */
    W(0x09),        /* Total Length (incl ifc, ept) */
    0x00,           /* # Interfaces */
    0x01,           /* Cfg Value */
    0x00,           /* Cfg String */
    0xc0,           /* Attributes -- self powered */
    0,              /* Power Consumption - none */
};

static uint8_t cfg_descr_highspeed[] = {
    0x09,           /* Length of Cfg Descr */
    0x02,           /* Type of Cfg Descr */
    W(0x09),        /* Total Length (incl ifc, ept) */
    0x00,           /* # Interfaces */
    0x01,           /* Cfg Value */
    0x00,           /* Cfg String */
    0xc0,           /* Attributes -- self powered */
    0,              /* Power Consumption - none */
};

static uchar langid[] __ALIGNED(2) = { 0x04, 0x03, 0x09, 0x04 };

#endif

