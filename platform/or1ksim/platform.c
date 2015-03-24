/*
 * Copyright (c) 2015 Stefan Kristiansson
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
#include <kernel/thread.h>
#include <kernel/vm.h>
#include <dev/uart.h>
#include <dev/timer/or1k_ticktimer.h>
#include <platform.h>
#include <platform/interrupts.h>
#include <platform/debug.h>
#include <platform/timer.h>
#include <platform/or1ksim.h>
#include <sys/types.h>
#include <target/debugconfig.h>

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

void platform_dputc(char c)
{
    if (c == '\n')
        uart_putc(DEBUG_UART, '\r');
    uart_putc(DEBUG_UART, c);
}

int platform_dgetc(char *c, bool wait)
{
    int _c;

    if ((_c = uart_getc(DEBUG_UART, false)) < 0)
        return -1;

    *c = _c;
    return 0;
}

void platform_early_init(void)
{
    uart_init_early();
    or1k_ticktimer_init(TIMER_CLOCK_FREQ);
#if WITH_KERNEL_VM
    pmm_add_arena(&ram_arena);
#endif
}
