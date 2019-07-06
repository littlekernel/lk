/*
 * Copyright (c) 2015 MediaTek Inc.
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/debug.h>
#include <stdlib.h>
#include <string.h>
#include <lk/err.h>
#include <lk/reg.h>
#include <sys/types.h>
#include <kernel/vm.h>
#include <platform.h>
#include <dev/uart.h>
#include <arch/arm.h>
#include <arch/arm/mmu.h>
#include <arch/ops.h>
#include <platform/mt_reg_base.h>
#include <platform/mt_typedefs.h>
#include <platform/mt_gpt.h>
#include <platform/mt_irq.h>

#define MTK_WDT_MODE TOPRGU_BASE

struct mmu_initial_mapping mmu_initial_mappings[] = {
    // XXX needs to be filled in

    /* Note: mapping entry should be 1MB alignment (address and size will be masked to 1MB boundaries in arch/arm/arm/start.S) */

    /* mcusys (peripherals) */
    {
        .phys = (uint64_t)0,
        .virt = (uint32_t)0,
        .size = 0x40000000,
        .flags = MMU_INITIAL_MAPPING_FLAG_DEVICE,
        .name = "mcusys"
    },
    /* ram */
    {
        .phys = (uint64_t)0x40000000,
        .virt = (uint32_t)0x40000000,
        .size = 0xc0000000,
        .flags = 0,
        .name = "ram"
    },

    /* null entry to terminate the list */
    { 0 }
};

static pmm_arena_t arena = {
    .name = "dram",
    .base = MEMBASE,
    .size = MEMSIZE,
    .flags = PMM_ARENA_FLAG_KMAP,
};

void platform_init_mmu_mappings(void) {
}

void platform_early_init(void) {
    uart_init_early();

    platform_init_interrupts();

    gpt_init();

    /* disable WDT */
    DRV_WriteReg32(MTK_WDT_MODE, 0x22000000);

    pmm_add_arena(&arena);
}

void platform_init(void) {
}
