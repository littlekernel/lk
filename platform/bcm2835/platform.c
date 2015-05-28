/*
 * Copyright (c) 2015 Travis Geiselbrecht
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
#include <reg.h>
#include <err.h>
#include <debug.h>
#include <trace.h>
#include <dev/uart.h>
#include <arch.h>
#include <arch/arm.h>
#include <arch/arm/mmu.h>
#include <lk/init.h>
#include <kernel/vm.h>
#include <kernel/spinlock.h>
#include <dev/timer/arm_generic.h>
#include <platform.h>
#include <platform/interrupts.h>
#include <platform/bcm2835.h>

extern void intc_init(void);
extern void arm_reset(void);

/* initial memory mappings. parsed by start.S */
struct mmu_initial_mapping mmu_initial_mappings[] = {
    /* 1GB of sdram space */
    { .phys = SDRAM_BASE,
      .virt = KERNEL_BASE,
      .size = MEMSIZE,
      .flags = 0,
      .name = "memory" },

    /* peripherals */
    { .phys = BCM_PERIPH_BASE_PHYS,
      .virt = BCM_PERIPH_BASE_VIRT,
      .size = BCM_PERIPH_SIZE,
      .flags = MMU_INITIAL_MAPPING_FLAG_DEVICE,
      .name = "bcm peripherals" },

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
    .size = MEMSIZE,
    .flags = PMM_ARENA_FLAG_KMAP,
};

void platform_init_mmu_mappings(void)
{
}

void platform_early_init(void)
{
    uart_init_early();

    intc_init();

    arm_generic_timer_init(INTERRUPT_ARM_LOCAL_CNTPNSIRQ, 1000000);

    /* add the main memory arena */
    pmm_add_arena(&arena);

#if WITH_SMP
    /* start the other cpus */
    uintptr_t sec_entry = (uintptr_t)&arm_reset;
    sec_entry -= (KERNEL_BASE - MEMBASE);
    for (uint i = 1; i <= 3; i++) {
        *REG32(ARM_LOCAL_BASE + 0x8c + 0x10 * i) = sec_entry;
    }
#endif
}

void platform_init(void)
{
    uart_init();
}

#define DEBUG_UART 0

void platform_dputc(char c)
{
    if (c == '\n')
        uart_putc(DEBUG_UART, '\r');
    uart_putc(DEBUG_UART, c);
}

int platform_dgetc(char *c, bool wait)
{
    int ret = uart_getc(DEBUG_UART, wait);
    if (ret == -1)
        return -1;
    *c = ret;
    return 0;
}

