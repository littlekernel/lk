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
#define MOTHERBOARD_CS0_PHYS (0x40000000)
#define MOTHERBOARD_CS1_PHYS (0x44000000)
#define MOTHERBOARD_CS2_PHYS (0x48000000)
#define MOTHERBOARD_CS3_PHYS (0x4c000000)
#define MOTHERBOARD_CS4_PHYS (0x50000000)
#define MOTHERBOARD_CS5_PHYS (0x54000000)
#define MOTHERBOARD_CS6_PHYS (0x58000000)
#define MOTHERBOARD_CS7_PHYS (0x10000000)
#define MOTHERBOARD_CS_SIZE  (0x04000000)

#define MOTHERBOARD_CS0_VIRT (0xe0000000)
#define MOTHERBOARD_CS1_VIRT (0xe4000000)
#define MOTHERBOARD_CS2_VIRT (0xe8000000)
#define MOTHERBOARD_CS3_VIRT (0xec000000)
#define MOTHERBOARD_CS4_VIRT (0xf0000000)
#define MOTHERBOARD_CS5_VIRT (0xf4000000)
#define MOTHERBOARD_CS6_VIRT (0xf8000000)
#define MOTHERBOARD_CS7_VIRT (0xfc000000)

#define SDRAM_BASE          (0x60000000)
#define SDRAM_APERTURE_SIZE (0x40000000)

/* most of the peripherals live on the motherboard CS7 */
#define UART0_BASE  (MOTHERBOARD_CS7_VIRT + 0x9000)
#define UART1_BASE  (MOTHERBOARD_CS7_VIRT + 0xa000)
#define UART2_BASE  (MOTHERBOARD_CS7_VIRT + 0xb000)
#define UART3_BASE  (MOTHERBOARD_CS7_VIRT + 0xc000)
#define VIRTIO_BASE (MOTHERBOARD_CS7_VIRT + 0x13000)

#define CPUPRIV_SIZE        (0x00100000)
#define CPUPRIV_BASE_PHYS   (0x1e000000)
#define CPUPRIV_BASE_VIRT   (MOTHERBOARD_CS0_VIRT - CPUPRIV_SIZE)

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

