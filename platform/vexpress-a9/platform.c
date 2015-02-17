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
#include <arch.h>
#include <arch/arm.h>
#include <arch/arm/mmu.h>
#include <err.h>
#include <debug.h>
#include <dev/interrupt/arm_gic.h>
#include <dev/timer/arm_cortex_a9.h>
#include <dev/uart.h>
#include <dev/virtio.h>
#include <lk/init.h>
#include <kernel/vm.h>
#include <kernel/spinlock.h>
#include <platform.h>
#include <platform/gic.h>
#include <platform/interrupts.h>
#include <platform/vexpress-a9.h>
#include "platform_p.h"

#define SDRAM_SIZE (512*1024*1024) // XXX get this from the emulator somehow

/* initial memory mappings. parsed by start.S */
struct mmu_initial_mapping mmu_initial_mappings[] = {
    /* 1GB of sdram space */
    { .phys = SDRAM_BASE,
      .virt = KERNEL_BASE,
      .size = SDRAM_SIZE,
      .flags = 0,
      .name = "memory" },

    /* CS0 - CS6 devices */
    { .phys = MOTHERBOARD_CS0_PHYS,
      .virt = MOTHERBOARD_CS0_VIRT,
      .size = MOTHERBOARD_CS_SIZE * 7,
      .flags = MMU_INITIAL_MAPPING_FLAG_DEVICE,
      .name = "cs0-cs6" },

    /* CS7 devices */
    { .phys = MOTHERBOARD_CS7_PHYS,
      .virt = MOTHERBOARD_CS7_VIRT,
      .size = MOTHERBOARD_CS_SIZE,
      .flags = MMU_INITIAL_MAPPING_FLAG_DEVICE,
      .name = "cs7" },

    /* cortex-a9 private memory area */
    { .phys = CPUPRIV_BASE_PHYS,
      .virt = CPUPRIV_BASE_PHYS, // XXX move back to CPUPRIV_BASE_VIRT
      .size = CPUPRIV_SIZE,
      .flags = MMU_INITIAL_MAPPING_FLAG_DEVICE,
      .name = "cpu_priv"},

    /* identity map to let the boot code run */
    { .phys = SDRAM_BASE,
      .virt = SDRAM_BASE,
      .size = 16*1024*1024,
      .flags = MMU_INITIAL_MAPPING_TEMPORARY },

    /* null entry to terminate the list */
    { 0 }
};

static pmm_arena_t arena = {
    .name = "sdram",
    .base = SDRAM_BASE,
    .size = SDRAM_SIZE,
    .flags = PMM_ARENA_FLAG_KMAP,
};

void platform_init_mmu_mappings(void)
{
}

void platform_early_init(void)
{
    /* initialize the interrupt controller */
    arm_gic_init();

    /* initialize the timer block */
    arm_cortex_a9_timer_init(CPUPRIV_BASE_PHYS, 100000000);

    uart_init_early();

    /* add the main memory arena */
    pmm_add_arena(&arena);
}

void platform_init(void)
{
    uart_init();

    /* detect any virtio devices */
    const uint virtio_irqs[] = { VIRTIO0_INT, VIRTIO1_INT, VIRTIO2_INT, VIRTIO3_INT };
    virtio_mmio_detect((void *)VIRTIO_BASE, 4, virtio_irqs);
}
