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
#include <stdio.h>
#include <arch/arm/mmu.h>
#include <kernel/vm.h>
#include <dev/uart.h>
#include <dev/interrupt/arm_gic.h>
#include <dev/timer/arm_cortex_a9.h>
#include <platform.h>
#include <platform/zynq.h>
#include <platform/gem.h>
#include <platform/timer.h>
#include "platform_p.h"

/* target can specify this as the initial jam table to set up the soc */
__WEAK void ps7_init(void) { }

STATIC_ASSERT(IS_ALIGNED(SDRAM_BASE, MB));
STATIC_ASSERT(IS_ALIGNED(SDRAM_SIZE, MB));

#if SDRAM_SIZE != 0
/* if we have sdram, the first 1MB is covered by sram */
#define RAM_SIZE (MB + (SDRAM_SIZE - MB))
#else
#define RAM_SIZE (MB)
#endif

/* initial memory mappings. parsed by start.S */
struct mmu_initial_mapping mmu_initial_mappings[] = {
    /* 1GB of sram + sdram space */
    { .phys = SRAM_BASE,
      .virt = KERNEL_BASE,
      .size = RAM_SIZE,
      .flags = 0,
      .name = "memory" },

    /* AXI fpga fabric bus 0 */
    { .phys = 0x40000000,
      .virt = 0x40000000,
      .size = (128*1024*1024),
      .flags = MMU_INITIAL_MAPPING_FLAG_UNCACHED,
      .name = "axi0" },

    /* AXI fpga fabric bus 1 */
    { .phys = 0x80000000,
      .virt = 0x80000000,
      .size = (16*1024*1024),
      .flags = MMU_INITIAL_MAPPING_FLAG_UNCACHED,
      .name = "axi1" },
    /* 0xe0000000 hardware devices */
    { .phys = 0xe0000000,
      .virt = 0xe0000000,
      .size = 0x00300000,
      .flags = MMU_INITIAL_MAPPING_FLAG_DEVICE,
      .name = "hw-e0000000" },

    /* 0xe1000000 hardware devices */
    { .phys = 0xe1000000,
      .virt = 0xe1000000,
      .size = 0x05000000,
      .flags = MMU_INITIAL_MAPPING_FLAG_DEVICE,
      .name = "hw-e1000000" },

    /* 0xf8000000 hardware devices */
    { .phys = 0xf8000000,
      .virt = 0xf8000000,
      .size = 0x01000000,
      .flags = MMU_INITIAL_MAPPING_FLAG_DEVICE,
      .name = "hw-f8000000" },

    /* 0xfc000000 hardware devices */
    { .phys = 0xfc000000,
      .virt = 0xfc000000,
      .size = 0x02000000,
      .flags = MMU_INITIAL_MAPPING_FLAG_DEVICE,
      .name = "hw-fc000000" },

    /* identity map to let the boot code run */
    { .phys = SRAM_BASE,
      .virt = SRAM_BASE,
      .size = RAM_SIZE,
      .flags = MMU_INITIAL_MAPPING_TEMPORARY },

    /* null entry to terminate the list */
    { 0 }
};

#if SDRAM_SIZE != 0
static pmm_arena_t sdram_arena = {
    .name = "sdram",
    .base = SDRAM_BASE,
    .size = SDRAM_SIZE - MB, /* first 1MB is covered by SRAM */
    .flags = PMM_ARENA_FLAG_KMAP
};
#endif

static pmm_arena_t sram_arena = {
    .name = "sram",
    .base = SRAM_BASE,
    .size = SRAM_SIZE,
    .priority = 1,
    .flags = PMM_ARENA_FLAG_KMAP
};

void platform_init_mmu_mappings(void)
{
}

void platform_early_init(void)
{
    ps7_init();

    /* zynq manual says this is mandatory */
    *REG32(SLCR_BASE + 0xa1c) = 0x020202;

    /* early initialize the uart so we can printf */
    uart_init_early();

    /* initialize the interrupt controller */
    arm_gic_init();

    /* initialize the timer block */
    arm_cortex_a9_timer_init(CPUPRIV_BASE, zynq_get_arm_timer_freq());

    /* add the main memory arena */
#if !ZYNQ_CODE_IN_SDRAM && SDRAM_SIZE != 0
    /* In the case of running from SRAM, and we are using SDRAM,
     * there is a discontinuity between the end of SRAM (256K) and the start of SDRAM (1MB),
     * so intentionally bump the boot-time allocator to start in the base of SDRAM.
     */
    extern uintptr_t boot_alloc_start;
    extern uintptr_t boot_alloc_end;

    boot_alloc_start = KERNEL_BASE + MB;
    boot_alloc_end = KERNEL_BASE + MB;
#endif

#if SDRAM_SIZE != 0
    pmm_add_arena(&sdram_arena);
#endif
    pmm_add_arena(&sram_arena);
}

void platform_init(void)
{
    uart_init();

    /* enable if we want to see some hardware boot status */
#if 0
    printf("zynq boot status:\n");
    printf("\tREBOOT_STATUS 0x%x\n", SLCR_REG(REBOOT_STATUS));
    printf("\tBOOT_MODE 0x%x\n", SLCR_REG(BOOT_MODE));

    zynq_dump_clocks();

    printf("zynq mio:\n");
    for (size_t i = 0; i < 54; i++) {
        printf("\t%02u: 0x%08x", i, *REG32((uintptr_t)&SLCR->MIO_PIN_00 + (i * 4)));
        if (i % 4 == 3 || i == 53) {
            putchar('\n');
        }
    }
#endif

    gem_init(GEM0_BASE, 256*1024);
}

void platform_quiesce(void)
{
    gem_disable();

    platform_stop_timer();
}

