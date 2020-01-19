/*
 * Copyright (c) 2018 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

// memory and irq layout of qemu's riscv virt platform
//
// mostly taken from the top of qemu/hw/riscv/virt.c and similar headers

#define IRQ_VIRTIO_BASE 1
#define IRQ_UART0       10
#define IRQ_PCIE_BASE   0x20
#define NUM_IRQS        127

#define CLINT_BASE  0x02000000
#define PLIC_BASE   0x0c000000
#define UART0_BASE  0x10000000
#define VIRTIO_BASE 0x10001000
#define DRAM_BASE   0x80000000

#if RISCV_XMODE_OFFSET == RISCV_MACH_OFFSET
#define PLIC_HART_IDX(hart)    (2 * (hart))
#elif RISCV_XMODE_OFFSET == RISCV_SUPER_OFFSET
#define PLIC_HART_IDX(hart)    ((2 * (hart)) + 1)
#endif

