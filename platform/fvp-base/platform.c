/*
 * Copyright (c) 2025 Mykola Hohsadze
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <platform/fvp-base.h>

#include <arch.h>
#include <lk/err.h>
#include <lk/debug.h>
#include <lk/trace.h>
#include <platform.h>
#include <platform/gic.h>
#include <platform/interrupts.h>
#include <dev/interrupt/arm_gic.h>
#include <dev/timer/arm_generic.h>
#include <dev/power/psci.h>
#include <dev/uart/pl011.h>
#include <lk/init.h>
#include <lib/fdtwalk.h>
#include <kernel/vm.h>
#include <kernel/spinlock.h>

#define LOCAL_TRACE 0

struct mmu_initial_mapping mmu_initial_mappings[] = {
    {
        .phys = MEMBASE,
        .virt = KERNEL_BASE,
        .size = MEMORY_APERTURE_SIZE,
        .flags = 0,  // normal memory
        .name = "memory",
    },
    {
        .phys = PERIPHERAL_BASE_PHYS,
        .virt = PERIPHERAL_BASE_VIRT,
        .size = PERIPHERAL_BASE_SIZE,
        .flags = MMU_INITIAL_MAPPING_FLAG_DEVICE,
        .name = "peripherals",
    },
    { 0 }
};

const void *fdt = (void *)DTB_BASE_VIRT;

const void *get_fdt(void) {
    return fdt;
}

void platform_early_init(void) {
    const struct pl011_config uart_config = {
        .base = UART_BASE,
        .irq = PL011_UART0_INT,
        .flag = PL011_FLAG_DEBUG_UART,
    };

    pl011_init_early(0, &uart_config);

    arm_gic_init();

    arm_generic_timer_init(ARM_GENERIC_TIMER_VIRTUAL_INT, 0);

    if (LOCAL_TRACE) {
        LTRACEF("dumping FDT at %p\n", fdt);
        fdt_walk_dump(fdt);
    }

    // detect physical memory layout from the device tree
    fdtwalk_setup_memory(fdt, MEMORY_BASE_PHYS, MEMORY_BASE_PHYS, MEMSIZE);
}

void platform_init(void) {
    pl011_init(0);
}

void platform_halt(platform_halt_action suggested_action, platform_halt_reason reason) {
    // Use the default halt implementation using psci as the reset and shutdown implementation.
    platform_halt_default(suggested_action, reason, &psci_system_reset, &psci_system_off);
}
