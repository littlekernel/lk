/*
 * Copyright (c) 2025 Mykola Hohsadze
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

/* up to 2 GB of ram */
#define MEMORY_BASE_PHYS     (0x80000000UL)
#define MEMORY_APERTURE_SIZE (2ULL * 1024 * 1024 * 1024)

#define DTB_BASE_VIRT (KERNEL_BASE + 0x2000000)

/* map all of 0-2GB into kernel space in one shot */
#define PERIPHERAL_BASE_PHYS (0)
#define PERIPHERAL_BASE_SIZE (0x80000000UL) // 2GB
#define PERIPHERAL_BASE_VIRT (0xffffffff80000000ULL) // -2GB

/* individual peripherals in this mapping */
#define UART_BASE (PERIPHERAL_BASE_VIRT + 0x001c090000)

/* interrupts */
#define ARM_GENERIC_TIMER_VIRTUAL_INT (27)
#define PL011_UART0_INT (32 + 5)

#define MAX_INT 128
