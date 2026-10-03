/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <inttypes.h>
#include <lk/err.h>
#include <lk/main.h>
#include <lk/reg.h>
#include <lk/trace.h>
#include <kernel/thread.h>
#include <kernel/vm.h>
#include <platform.h>
#include <platform/interrupts.h>
#include <platform/debug.h>
#include <platform/timer.h>
#include <platform/eic7700.h>
#include <sys/types.h>
#include <lib/fdtwalk.h>
#include <dev/interrupt/riscv_plic.h>

#include "platform_p.h"

#define LOCAL_TRACE 0

static const void *fdt;

// The L3 comes out of reset with only way 0 of its 16 enabled as cache, the rest
// mapped as L3 LIM scratchpad, and nothing in the boot chain enables them (the vendor
// Linux ccache driver does it itself). WayEnable holds the highest enabled way and
// can only grow.
static void l3_enable_ways(void) {
    const uint32_t config = *REG32(L3_CACHE_BASE_VIRT + 0x0);
    const uint32_t ways = (config >> 8) & 0xff;
    const uint32_t enabled = *REG32(L3_CACHE_BASE_VIRT + 0x8) & 0xf;
    if (ways > 0) {
        *REG32(L3_CACHE_BASE_VIRT + 0x8) = ways - 1;
    }
    dprintf(INFO, "L3: %u banks, %u ways, largest enabled way %u -> %u\n", config & 0xff, ways,
            enabled, *REG32(L3_CACHE_BASE_VIRT + 0x8) & 0xf);
}

void platform_early_init(void) {
    // bring the console uart up first, so early output has somewhere to go
    platform_init_uart_early();

    l3_enable_ways();

    TRACE;
    // every hart has both an M and an S mode context, so the targets are flat
    plic_early_init(PLIC_BASE_VIRT, NUM_IRQS, false);

    LTRACEF("starting FDT scan\n");

    /* look for a flattened device tree in the second arg passed to us */
    fdt = (void *)lk_boot_args[1];
    fdt = (const void *)((uintptr_t)fdt + PERIPHERAL_BASE_VIRT);

    if (LOCAL_TRACE) {
        LTRACEF("dumping FDT at %p\n", fdt);
        fdt_walk_dump(fdt);
    }

    // detect physical memory layout from the device tree
    fdtwalk_setup_memory(fdt, lk_boot_args[1], MEMORY_BASE_PHYS, MEMSIZE);

    // detect secondary cores to start
    fdtwalk_setup_cpus_riscv(fdt);

    // pick up /chosen/bootargs, which is how lk.autorun gets in from U-Boot
    fdtwalk_setup_cmdline(fdt);

    LTRACEF("done scanning FDT\n");
}

void platform_init(void) {
    plic_init();
    platform_init_uart();
}

// OpenSBI's board support forwards both of these to the board management
// MCU over UART2, which is the only way to cut power or reset from the SoC.
static void reboot_(void) {
    sbi_system_reset(SBI_RESET_TYPE_COLD_REBOOT, SBI_RESET_REASON_NONE);
}

static void shutdown_(void) {
    sbi_system_reset(SBI_RESET_TYPE_SHUTDOWN, SBI_RESET_REASON_NONE);
}

void platform_halt(platform_halt_action suggested_action, platform_halt_reason reason) {
    platform_halt_default(suggested_action, reason, &reboot_, &shutdown_);
}

status_t platform_allocate_interrupts(size_t count, uint align_log2, bool msi, unsigned int *vector) {
    return ERR_NOT_SUPPORTED;
}

status_t platform_compute_msi_values(unsigned int vector, unsigned int cpu, bool edge,
        uint64_t *msi_address_out, uint16_t *msi_data_out) {
    return ERR_NOT_SUPPORTED;
}
