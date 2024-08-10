/*
 * Copyright (c) 2018 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include "config.h"
#include <inttypes.h>
#include <lk/err.h>
#include <lk/main.h>
#include <lk/reg.h>
#include <lk/trace.h>
#include <kernel/thread.h>
#include <platform.h>
#include <platform/interrupts.h>
#include <platform/debug.h>
#include <platform/timer.h>
#include <platform/spacemit-k1.h>
#include <sys/types.h>
#include <lib/fdtwalk.h>
#include <dev/interrupt/riscv_plic.h>
#if WITH_LIB_MINIP
#include <lib/minip.h>
#endif
#include <kernel/vm.h>

#include "platform_p.h"

#define LOCAL_TRACE 0

static const void *fdt;

void platform_early_init(void) {
    TRACE;
    plic_early_init(PLIC_BASE_VIRT, NUM_IRQS, false);

    LTRACEF("starting FDT scan\n");

    /* look for a flattened device tree in the second arg passed to us */
    fdt = (void *)lk_boot_args[1];
    fdt = (const void *)((uintptr_t)fdt + KERNEL_ASPACE_BASE);

    if (LOCAL_TRACE) {
        LTRACEF("dumping FDT at %p\n", fdt);
        fdt_walk_dump(fdt);
    }

    // detect physical memory layout from the device tree
    fdtwalk_setup_memory(fdt, lk_boot_args[1], MEMORY_BASE_PHYS, MEMSIZE);

    // detect secondary cores to start
    fdtwalk_setup_cpus_riscv(fdt);

    LTRACEF("done scanning FDT\n");

    // disable the watchdog
    mmio_write32((volatile uint32_t *)paddr_to_kvaddr(0xd40800b0), 0xbaba);
    mmio_write32((volatile uint32_t *)paddr_to_kvaddr(0xd40800b4), 0xeb10);
    mmio_write32((volatile uint32_t *)paddr_to_kvaddr(0xd40800b8), 0);
}

void platform_init(void) {
    plic_init();
    uart_init();
}

static void reboot_(void) {
#if RISCV_S_MODE
    sbi_system_reset(SBI_RESET_TYPE_COLD_REBOOT, SBI_RESET_REASON_NONE);
#endif
}

static void shutdown_(void) {
#if RISCV_S_MODE
    // try to use sbi as a cleaner way to stop
    sbi_system_reset(SBI_RESET_TYPE_SHUTDOWN, SBI_RESET_REASON_NONE);
#endif
}

void platform_halt(platform_halt_action suggested_action, platform_halt_reason reason) {
    // Use the default halt implementation using sbi as the reset and shutdown implementation.
    platform_halt_default(suggested_action, reason, &reboot_, &shutdown_);
}

status_t platform_pci_int_to_vector(unsigned int pci_int, unsigned int *vector) {
    // at the moment there's no translation between PCI IRQs and native irqs
    *vector = pci_int;
    return NO_ERROR;
}

status_t platform_allocate_interrupts(size_t count, uint align_log2, bool msi, unsigned int *vector) {
    return ERR_NOT_SUPPORTED;
}

status_t platform_compute_msi_values(unsigned int vector, unsigned int cpu, bool edge,
        uint64_t *msi_address_out, uint16_t *msi_data_out) {
    return ERR_NOT_SUPPORTED;
}
