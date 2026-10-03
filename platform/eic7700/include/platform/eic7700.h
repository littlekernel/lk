/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

// Memory and irq layout of the ESWIN EIC7700X, from the EIC7700X TRM and the
// device tree U-Boot hands us. Everything below 2GB is MCPU internals and
// peripherals, DRAM starts at 2GB.

#define MEMORY_BASE_PHYS     (0x80000000UL)
// up to 32 GB of ram
#define MEMORY_APERTURE_SIZE (32ULL * 1024 * 1024 * 1024)

// map all of 0-2GB into kernel space in one shot
#define PERIPHERAL_BASE_PHYS (0)
#define PERIPHERAL_BASE_SIZE (0x80000000UL) // 2GB

// use the giant mapping at the bottom of the kernel as our peripheral space
#define PERIPHERAL_BASE_VIRT (KERNEL_ASPACE_BASE + PERIPHERAL_BASE_PHYS)

// interrupts (PLIC source ids)
#define IRQ_UART0       100
#define IRQ_UART1       101
#define IRQ_UART2       102
#define NUM_IRQS        520

// addresses of some peripherals
#define CLINT_BASE          0x02000000
#define L3_CACHE_BASE       0x02010000 // SiFive L3 cache controller
#define L3_CACHE_BASE_VIRT  (PERIPHERAL_BASE_VIRT + L3_CACHE_BASE)
#define CLINT_BASE_VIRT     (PERIPHERAL_BASE_VIRT + CLINT_BASE)
#define PLIC_BASE           0x0c000000
#define PLIC_BASE_VIRT      (PERIPHERAL_BASE_VIRT + PLIC_BASE)
// DesignWare 16550, 4 byte register stride, 200MHz reference clock
#define UART0_BASE          0x50900000
#define UART0_BASE_VIRT     (PERIPHERAL_BASE_VIRT + UART0_BASE)
#define UART1_BASE          0x50910000
#define UART2_BASE          0x50920000
#define DRAM_BASE           MEMORY_BASE_PHYS
#define DRAM_BASE_VIRT      (PERIPHERAL_BASE_VIRT + DRAM_BASE)
