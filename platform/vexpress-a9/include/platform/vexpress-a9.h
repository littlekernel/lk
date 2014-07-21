/*
 * Copyright (c) 2012-2014 Travis Geiselbrecht
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

/* memory map of the motherboard */
#define MOTHERBOARD_CS0 (0x40000000)
#define MOTHERBOARD_CS1 (0x44000000)
#define MOTHERBOARD_CS2 (0x48000000)
#define MOTHERBOARD_CS3 (0x4c000000)
#define MOTHERBOARD_CS4 (0x50000000)
#define MOTHERBOARD_CS5 (0x54000000)
#define MOTHERBOARD_CS6 (0x58000000)
#define MOTHERBOARD_CS7 (0x10000000)

/* most of the peripherals live on the motherboard CS7 */
#define UART0_BASE  (MOTHERBOARD_CS7 + 0x9000)
#define UART1_BASE  (MOTHERBOARD_CS7 + 0xa000)
#define UART2_BASE  (MOTHERBOARD_CS7 + 0xb000)
#define UART3_BASE  (MOTHERBOARD_CS7 + 0xc000)
#define VIRTIO_BASE (MOTHERBOARD_CS7 + 0x13000)

#define CPUPRIV_BASE        (0x1e000000)

/* interrupts */
#define ARM_GENERIC_TIMER_INT 29
#define TIMER01_INT (32 + 2)
#define TIMER23_INT (32 + 3)
#define UART0_INT   (32 + 5)
#define UART1_INT   (32 + 6)
#define UART2_INT   (32 + 7)
#define UART3_INT   (32 + 8)
#define VIRTIO0_INT (32 + 40)
#define VIRTIO1_INT (32 + 41)
#define VIRTIO2_INT (32 + 42)
#define VIRTIO3_INT (32 + 43)

#define MAX_INT 96

