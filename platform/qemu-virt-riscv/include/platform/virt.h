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

#define MEMORY_BASE_PHYS     (0x80000000)
#if __riscv_xlen == 64
// up to 64 GB of ram, which seems to be a soft cap
#define MEMORY_APERTURE_SIZE (64ULL * 1024 * 1024 * 1024)
#else
// cap after 1GB
#define MEMORY_APERTURE_SIZE (1UL * 1024 * 1024 * 1024)
#endif

// map all of 0-1GB into kernel space in one shot
#define PERIPHERAL_BASE_PHYS (0)
#define PERIPHERAL_BASE_SIZE (0x40000000UL) // 1GB

// use the giant mapping at the bottom of the kernel as our peripheral space
#if WITH_KERNEL_VM
#define PERIPHERAL_BASE_VIRT (KERNEL_ASPACE_BASE + PERIPHERAL_BASE_PHYS)
#else
// if no mmu, just treat virt == phys
#define PERIPHERAL_BASE_VIRT (PERIPHERAL_BASE_PHYS)
#endif

// interrupts
#define IRQ_VIRTIO_BASE 1
#define IRQ_UART0       10
#define IRQ_PCIE_BASE   0x20
#define NUM_IRQS        128

// addresses of some peripherals
#define CLINT_BASE          0x02000000
#define CLINT_BASE_VIRT     (PERIPHERAL_BASE_VIRT + CLINT_BASE)
#define PLIC_BASE           0x0c000000
#define PLIC_BASE_VIRT      (PERIPHERAL_BASE_VIRT + PLIC_BASE)
#define UART0_BASE          0x10000000
#define UART0_BASE_VIRT     (PERIPHERAL_BASE_VIRT + UART0_BASE)
#define VIRTIO_BASE         0x10001000
#define VIRTIO_BASE_VIRT    (PERIPHERAL_BASE_VIRT + VIRTIO_BASE)
#define DRAM_BASE           0x80000000
#define DRAM_BASE_VIRT      (PERIPHERAL_BASE_VIRT + DRAM_BASE)

#define NUM_VIRTIO_TRANSPORTS 8
#define VIRTIO_STRIDE 0x1000
