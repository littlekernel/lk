// Copyright (c) 2013-2015 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
#include <assert.h>
#include <kernel/thread.h>
#include <lk/compiler.h>
#include <lk/debug.h>
#include <lk/init.h>
#include <lk/main.h>

// Top level entry points for secondary cpus

#if WITH_SMP
static thread_t *secondary_bootstrap_threads[SMP_MAX_CPUS - 1];
static uint secondary_bootstrap_thread_count;

void lk_secondary_cpu_entry_early(void) {
    // get the cpu into threading context
    thread_secondary_cpu_init_early();

    // run early secondary cpu init routines up to the threading level
    lk_init_level(LK_INIT_FLAG_SECONDARY_CPUS, LK_INIT_LEVEL_EARLIEST, LK_INIT_LEVEL_THREADING - 1);
}

void lk_secondary_cpu_entry(void) {
    uint cpu = arch_curr_cpu_num();
    DEBUG_ASSERT(cpu <= secondary_bootstrap_thread_count);

    thread_resume(secondary_bootstrap_threads[cpu - 1]);

    dprintf(SPEW, "entering scheduler on cpu %d\n", cpu);
    thread_secondary_cpu_entry();
}

// Secondary cpu bootstrap thread, which gives enough thread context to run the secondary cpu init routines
// from LK_INIT_LEVEL_THREADING to LK_INIT_LEVEL_LAST.
static int secondary_cpu_bootstrap_thread(void *arg) {
    // Secondary cpu initialize from threading level up. 0 to threading was handled in arch
    lk_init_level(LK_INIT_FLAG_SECONDARY_CPUS, LK_INIT_LEVEL_THREADING, LK_INIT_LEVEL_LAST);

    return 0;
}

void lk_init_secondary_cpus(uint secondary_cpu_count) {
    if (secondary_cpu_count >= SMP_MAX_CPUS) {
        dprintf(CRITICAL, "Invalid secondary_cpu_count %d, SMP_MAX_CPUS %d\n",
                secondary_cpu_count, SMP_MAX_CPUS);
        secondary_cpu_count = SMP_MAX_CPUS - 1;
    }
    // Construct the idle and bootstrap threads for each secondary cpu
    for (uint i = 0; i < secondary_cpu_count; i++) {
        thread_create_secondary_cpu_idle_thread(i + 1);

        dprintf(SPEW, "creating bootstrap completion thread for cpu %d\n", i + 1);
        thread_t *t = thread_create("secondarybootstrap2",
                                    &secondary_cpu_bootstrap_thread, NULL,
                                    DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
        thread_set_pinned_cpu(t, i + 1);
        secondary_bootstrap_threads[i] = t;
        thread_detach(t);
    }
    secondary_bootstrap_thread_count = secondary_cpu_count;
}
#endif // WITH_SMP
