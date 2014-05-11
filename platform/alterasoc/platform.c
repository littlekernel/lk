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
#include <platform.h>
#include <platform/alterasoc.h>
#include "platform_p.h"

void platform_init_mmu_mappings(void)
{
#define MB (1024*1024)
    STATIC_ASSERT((MEMBASE % MB) == 0);
    STATIC_ASSERT((MEMSIZE % MB) == 0);

    /* map dram as full cacheable */
    for (addr_t a = MEMBASE; a < MEMSIZE; a += MB) {
        arm_mmu_map_section(a, a, MMU_MEMORY_L1_TYPE_NORMAL_WRITE_BACK_ALLOCATE | MMU_MEMORY_L1_AP_P_RW_U_NA);
    }
}

void platform_early_init(void)
{
    uart_init_early();

    /* initialize the interrupt controller */
    arm_gic_init();

    /* initialize the timer block */
    arm_cortex_a9_timer_init(CPUPRIV_BASE, TIMER_CLOCK_FREQ);
}

void platform_init(void)
{
    uart_init();
}

