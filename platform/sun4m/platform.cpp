//
// Copyright (c) 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
//
#include <kernel/thread.h>
#include <lk/err.h>
#include <lk/reg.h>
#include <platform.h>
#include <platform/debug.h>
#include <platform/interrupts.h>
#include <platform/timer.h>
#include <sys/types.h>

#include <lk/debug.h>
#include <lk/main.h>
#include <kernel/novm.h>

#include "platform_p.h"

void platform_early_init(void) {
    sun4m_intc_early_init();
    sun4m_scc_early_init();
    sun4m_timer_early_init();

    platform_prom_init((void *)lk_boot_args[0]);

    uintptr_t base;
    size_t size;
    if (platform_prom_initialize_arena((void *)lk_boot_args[0], &base, &size) == NO_ERROR) {
        dprintf(INFO, "PROM: Available memory range: base 0x%lx, size 0x%zx (%zu MB)\n",
                base, size, size / (1024 * 1024));
        novm_add_arena("mem", base, size);
    } else {
        dprintf(INFO, "PROM: Failed to auto-configure memory from PROM, using fallback MEMBASE=0x%lx, MEMSIZE=0x%lx\n",
                (unsigned long)MEMBASE, (unsigned long)MEMSIZE);
        novm_add_arena("mem", (uintptr_t)MEMBASE, (size_t)MEMSIZE);
    }
}

void platform_init(void) {
    sun4m_intc_init();
    sun4m_timer_init();
    sun4m_scc_init();
}
