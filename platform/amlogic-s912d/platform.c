/*
 * Copyright (c) 2018 The Fuchsia Authors
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <arch/arm64.h>
#include <lk/trace.h>
#include <assert.h>
#include <lk/err.h>
#include <lk/bits.h>
#include <kernel/spinlock.h>
#include <kernel/thread.h>
#include <kernel/mp.h>
#include <platform/interrupts.h>
#include <lk/init.h>
#include <kernel/vm.h>
#include <kernel/spinlock.h>
#include <dev/timer/arm_generic.h>
#include <platform.h>
#include <dev/interrupt/arm_gic.h>
#include <dev/timer/arm_generic.h>
#include <platform/s912d.h>
#include <dev/uart.h>

typedef struct arm64_iframe_long arm_platform_iframe_t;

/* initial memory mappings. parsed by start.S */
struct mmu_initial_mapping mmu_initial_mappings[] = {
    {
        .phys = SDRAM_BASE,
        .virt = KERNEL_BASE,
        .size = MEMSIZE,
        .flags = 0,
        .name = "memory"
    },

    /* peripherals */
    {
        .phys = AML_S912D_PERIPH_BASE_PHYS,
        .virt = AML_S912D_PERIPH_BASE_VIRT,
        .size = AML_S912D_PERIPH_BASE_SIZE,
        .flags = MMU_INITIAL_MAPPING_FLAG_DEVICE,
        .name = "peripherals"
    },
    /* null entry to terminate the list */
    { 0 }
};

static pmm_arena_t arena = {
    .name = "sdram",
    .base = SDRAM_BASE,
    .size = MEMSIZE,
    .flags = PMM_ARENA_FLAG_KMAP,
};

#define DEBUG_UART 0

void platform_dputc(char c) {
    if (c == '\n')
        uart_putc(DEBUG_UART, '\r');
    uart_putc(DEBUG_UART, c);
}

int platform_dgetc(char *c, bool wait) {
    int ret = uart_getc(DEBUG_UART, wait);
    if (ret == -1)
        return -1;
    *c = ret;
    return 0;
}

void platform_init(void) {
    uart_init();

}

void platform_early_init(void) {
    uart_init_early();

    /* initialize the interrupt controller */
    arm_gic_init();

    arm_generic_timer_init(30, 0);

    pmm_add_arena(&arena);

    // TODO: Reserve memory regions if needed
}

