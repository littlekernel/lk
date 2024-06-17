/*
 * Copyright (c) 2024 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

// Taken from device tree from booted Linux system

// memory and irq layout of Spacemit-K1
#define MEMORY_BASE_PHYS     (0)
// up to 16 GB of ram
#define MEMORY_APERTURE_SIZE (16ULL * 1024 * 1024 * 1024)

// map all of 0-2GB into kernel space in one shot
#define PERIPHERAL_BASE_PHYS (0x80000000UL)
#define PERIPHERAL_BASE_SIZE (0x80000000UL) // 2GB

// use the giant mapping at the bottom of the kernel as our peripheral space
#define PERIPHERAL_BASE_VIRT (KERNEL_ASPACE_BASE + PERIPHERAL_BASE_PHYS)

// interrupts
#define IRQ_VIRTIO_BASE 1
#define IRQ_UART0       0x2a
#define NUM_IRQS        0x9f

// addresses of some peripherals
#define CLINT_BASE          0xe4000000
#define CLINT_BASE_VIRT     (PERIPHERAL_BASE_VIRT + CLINT_BASE - PERIPHERAL_BASE_PHYS)
#define PLIC_BASE           0xe0000000
#define PLIC_BASE_VIRT      (PERIPHERAL_BASE_VIRT + PLIC_BASE - PERIPHERAL_BASE_PHYS)
#define UART0_BASE          0xd4017000
#define UART0_BASE_VIRT     (PERIPHERAL_BASE_VIRT + UART0_BASE - PERIPHERAL_BASE_PHYS)
#define DRAM_BASE           0
#define DRAM_BASE_VIRT      (PERIPHERAL_BASE_VIRT + DRAM_BASE - PERIPHERAL_BASE_PHYS)
#define DRAM_BASE2          0x10000000UL
#define DRAM_BASE2_VIRT      (PERIPHERAL_BASE_VIRT + DRAM_BASE2 - PERIPHERAL_BASE_PHYS)
