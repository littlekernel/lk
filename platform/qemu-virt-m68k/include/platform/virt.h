/*
 * Copyright (c) 2021 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

// Top level #defines for the 68k-virt machine in qemu 6.0
//
// From qemu/hw/m68k/virt.c

/*
 * 6 goldfish-pic for CPU IRQ #1 to IRQ #6
 * CPU IRQ #1 -> PIC #1
 *               IRQ #1 to IRQ #31 -> unused
 *               IRQ #32 -> goldfish-tty
 * CPU IRQ #2 -> PIC #2
 *               IRQ #1 to IRQ #32 -> virtio-mmio from 1 to 32
 * CPU IRQ #3 -> PIC #3
 *               IRQ #1 to IRQ #32 -> virtio-mmio from 33 to 64
 * CPU IRQ #4 -> PIC #4
 *               IRQ #1 to IRQ #32 -> virtio-mmio from 65 to 96
 * CPU IRQ #5 -> PIC #5
 *               IRQ #1 to IRQ #32 -> virtio-mmio from 97 to 128
 * CPU IRQ #6 -> PIC #6
 *               IRQ #1 -> goldfish-rtc
 *               IRQ #2 to IRQ #32 -> unused
 * CPU IRQ #7 -> NMI
 */

#define VIRT_GF_PIC_MMIO_BASE 0xff000000     /* MMIO: 0xff000000 - 0xff005fff */
#define VIRT_GF_PIC_IRQ_BASE  1              /* IRQ: #1 -> #6 */
#define VIRT_GF_PIC_NB        6

#define NUM_IRQS              (VIRT_GF_PIC_NB * 32) // PIC 1 - 6

/* maps (pic + irq) base one to a linear number zero based */
#define PIC_IRQ(pic, irq)     (((pic) - 1) * 32 + ((irq) - 1))

/* 2 goldfish-rtc (and timer) */
#define VIRT_GF_RTC_MMIO_BASE 0xff006000     /* MMIO: 0xff006000 - 0xff007fff */
#define VIRT_GF_RTC_IRQ_BASE  PIC_IRQ(6, 1)  /* PIC: #6, IRQ: #1 */
#define VIRT_GF_RTC_NB        2

/* 1 goldfish-tty */
#define VIRT_GF_TTY_MMIO_BASE 0xff008000     /* MMIO: 0xff008000 - 0xff008fff */
#define VIRT_GF_TTY_IRQ_BASE  PIC_IRQ(1, 32) /* PIC: #1, IRQ: #32 */

/* 1 virt-ctrl */
#define VIRT_CTRL_MMIO_BASE 0xff009000    /* MMIO: 0xff009000 - 0xff009fff */
#define VIRT_CTRL_IRQ_BASE    PIC_IRQ(1, 1) /* PIC: #1, IRQ: #1 */

/*
 * virtio-mmio size is 0x200 bytes
 * we use 4 goldfish-pic to attach them,
 * we can attach 32 virtio devices / goldfish-pic
 * -> we can manage 32 * 4 = 128 virtio devices
 */
#define VIRT_VIRTIO_MMIO_BASE 0xff010000     /* MMIO: 0xff010000 - 0xff01ffff */
#define VIRT_VIRTIO_IRQ_BASE  PIC_IRQ(2, 1)  /* PIC: 2, 3, 4, 5, IRQ: ALL */

#define NUM_VIRT_VIRTIO 128
