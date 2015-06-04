/*
 * Copyright (c) 2012-2014 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <err.h>
#include <debug.h>
#include <assert.h>
#include <trace.h>
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
    { .phys = 0x0,
      .virt = KERNEL_BASE,
      .size = 1024*1024*1024,
      .flags = 0,
      .name = "memory" },

    /* HPS peripherals */
    { .phys = 0xfc000000,
      .virt = 0xfc000000,
      .size = 0x04000000,
      .flags = MMU_INITIAL_MAPPING_FLAG_DEVICE,
      .name = "hps_periphs" },

    /* identity map to let the boot code run */
    { .phys = 0,
      .virt = 0,
      .size = 1024*1024*1024,
      .flags = MMU_INITIAL_MAPPING_TEMPORARY },

    /* null entry to terminate the list */
    { 0 }
};

static pmm_arena_t sdram_arena = {
    .name = "sdram",
    .base = 0,
    .size = MEMSIZE,
    .flags = PMM_ARENA_FLAG_KMAP
};

void platform_init_mmu_mappings(void)
{
}

void platform_early_init(void)
{
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

void platform_init(void)
{
    uart_init();
}

