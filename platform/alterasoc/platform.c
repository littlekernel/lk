/*
 * Copyright (c) 2012-2014 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/err.h>
#include <lk/debug.h>
#include <assert.h>
#include <lk/trace.h>
#include <arch/arm/mmu.h>
#include <dev/uart.h>
#include <dev/interrupt/arm_gic.h>
#include <dev/timer/arm_cortex_a9.h>
#include <kernel/vm.h>
#include <platform.h>
#include <platform/alterasoc.h>
#include "platform_p.h"

/* initial memory mappings. parsed by start.S */
struct mmu_initial_mapping mmu_initial_mappings[] = {
    /* 1GB of sdram space */
    {
        .phys = 0x0,
        .virt = KERNEL_BASE,
        .size = 1024*1024*1024,
        .flags = 0,
        .name = "memory"
    },

    /* HPS peripherals */
    {
        .phys = 0xfc000000,
        .virt = 0xfc000000,
        .size = 0x04000000,
        .flags = MMU_INITIAL_MAPPING_FLAG_DEVICE,
        .name = "hps_periphs"
    },

    /* identity map to let the boot code run */
    {
        .phys = 0,
        .virt = 0,
        .size = 1024*1024*1024,
        .flags = MMU_INITIAL_MAPPING_TEMPORARY
    },

    /* null entry to terminate the list */
    { 0 }
};

static pmm_arena_t sdram_arena = {
    .name = "sdram",
    .base = 0,
    .size = MEMSIZE,
    .flags = PMM_ARENA_FLAG_KMAP
};

void platform_init_mmu_mappings(void) {
}

void platform_early_init(void) {
    uart_init_early();

    printf("stat 0x%x\n", *REG32(0xffd05000));

    /* initialize the interrupt controller */
    arm_gic_init();

    /* initialize the timer block */
    arm_cortex_a9_timer_init(CPUPRIV_BASE, TIMER_CLOCK_FREQ);

    pmm_add_arena(&sdram_arena);

    /* start the secondary cpu */
    *REG32(0xffd05010) = 0;
}

void platform_init(void) {
    uart_init();
}

