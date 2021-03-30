/*
 * Copyright (c) 2015 Stefan Kristiansson
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/reg.h>
#include <kernel/thread.h>
#include <dev/uart.h>
#include <dev/timer/or1k_ticktimer.h>
#include <platform.h>
#include <platform/interrupts.h>
#include <platform/debug.h>
#include <platform/timer.h>
#include <platform/or1ksim.h>
#include <sys/types.h>
#include <target/debugconfig.h>
#if WITH_KERNEL_VM
#include <kernel/vm.h>
#endif

struct mmu_initial_mapping mmu_initial_mappings[] = {
    /* 32 MB of RAM space */
    {
        .phys = 0x0,
        .virt = KERNEL_BASE,
        .size = 32*1024*1024,
        .flags = 0,
        .name = "memory"
    },

    /* peripherals */
    {
        .phys = 0x90000000,
        .virt = 0x90000000,
        .size = 0x04000000,
        .flags = MMU_INITIAL_MAPPING_FLAG_DEVICE,
        .name = "peripherals"
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

static pmm_arena_t ram_arena = {
    .name = "ram",
    .base = 0,
    .size = MEMSIZE,
    .flags = PMM_ARENA_FLAG_KMAP
};

void platform_dputc(char c) {
    if (c == '\n')
        uart_putc(DEBUG_UART, '\r');
    uart_putc(DEBUG_UART, c);
}

int platform_dgetc(char *c, bool wait) {
    int _c;

    if ((_c = uart_getc(DEBUG_UART, false)) < 0)
        return -1;

    *c = _c;
    return 0;
}

void platform_early_init(void) {
    uart_init_early();
    or1k_ticktimer_init(TIMER_CLOCK_FREQ);
#if WITH_KERNEL_VM
    pmm_add_arena(&ram_arena);
#endif
}
