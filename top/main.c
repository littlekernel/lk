/*
 * Copyright (c) 2013-2015 Travis Geiselbrecht
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

/*
 * Main entry point to the OS. Initializes modules in order and creates
 * the default thread.
 */
#include <compiler.h>
#include <debug.h>
#include <string.h>
#include <app.h>
#include <arch.h>
#include <platform.h>
#include <target.h>
#include <lib/heap.h>
#include <kernel/mutex.h>
#include <kernel/novm.h>
#include <kernel/thread.h>
#include <lk/init.h>
#include <lk/main.h>

/* saved boot arguments from whoever loaded the system */
ulong lk_boot_args[4];

extern void *__ctor_list;
extern void *__ctor_end;
extern int __bss_start;
extern int _end;

#if WITH_SMP
static thread_t *secondary_bootstrap_threads[SMP_MAX_CPUS - 1];
static uint secondary_bootstrap_thread_count;
#endif

static int bootstrap2(void *arg);

extern void kernel_init(void);

static void call_constructors(void)
{
    void **ctor;

    ctor = &__ctor_list;
    while (ctor != &__ctor_end) {
        void (*func)(void);

        func = (void ( *)(void))*ctor;

        func();
        ctor++;
    }
}

/* called from arch code */
void lk_main(ulong arg0, ulong arg1, ulong arg2, ulong arg3)
{
    // save the boot args
    lk_boot_args[0] = arg0;
    lk_boot_args[1] = arg1;
    lk_boot_args[2] = arg2;
    lk_boot_args[3] = arg3;

    // get us into some sort of thread context
    thread_init_early();

    // early arch stuff
    lk_primary_cpu_init_level(LK_INIT_LEVEL_EARLIEST, LK_INIT_LEVEL_ARCH_EARLY - 1);
    arch_early_init();

    // do any super early platform initialization
    lk_primary_cpu_init_level(LK_INIT_LEVEL_ARCH_EARLY, LK_INIT_LEVEL_PLATFORM_EARLY - 1);
    platform_early_init();

    // do any super early target initialization
    lk_primary_cpu_init_level(LK_INIT_LEVEL_PLATFORM_EARLY, LK_INIT_LEVEL_TARGET_EARLY - 1);
    target_early_init();

#if WITH_SMP
    dprintf(INFO, "\nwelcome to lk/MP\n\n");
#else
    dprintf(INFO, "\nwelcome to lk\n\n");
#endif
    dprintf(INFO, "boot args 0x%lx 0x%lx 0x%lx 0x%lx\n",
            lk_boot_args[0], lk_boot_args[1], lk_boot_args[2], lk_boot_args[3]);

    // bring up the kernel heap
    lk_primary_cpu_init_level(LK_INIT_LEVEL_TARGET_EARLY, LK_INIT_LEVEL_HEAP - 1);
    dprintf(SPEW, "initializing heap\n");
    heap_init();

    // deal with any static constructors
    dprintf(SPEW, "calling constructors\n");
    call_constructors();

    // initialize the kernel
    lk_primary_cpu_init_level(LK_INIT_LEVEL_HEAP, LK_INIT_LEVEL_KERNEL - 1);
    kernel_init();

    lk_primary_cpu_init_level(LK_INIT_LEVEL_KERNEL, LK_INIT_LEVEL_THREADING - 1);

    // create a thread to complete system initialization
    dprintf(SPEW, "creating bootstrap completion thread\n");
    thread_t *t = thread_create("bootstrap2", &bootstrap2, NULL, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
    thread_set_pinned_cpu(t, 0);
    thread_detach(t);
    thread_resume(t);

    // become the idle thread and enable interrupts to start the scheduler
    thread_become_idle();
}

static int bootstrap2(void *arg)
{
    dprintf(SPEW, "top of bootstrap2()\n");

    lk_primary_cpu_init_level(LK_INIT_LEVEL_THREADING, LK_INIT_LEVEL_ARCH - 1);
    arch_init();

    // initialize the rest of the platform
    dprintf(SPEW, "initializing platform\n");
    lk_primary_cpu_init_level(LK_INIT_LEVEL_ARCH, LK_INIT_LEVEL_PLATFORM - 1);
    platform_init();

    // initialize the target
    dprintf(SPEW, "initializing target\n");
    lk_primary_cpu_init_level(LK_INIT_LEVEL_PLATFORM, LK_INIT_LEVEL_TARGET - 1);
    target_init();

    dprintf(SPEW, "calling apps_init()\n");
    lk_primary_cpu_init_level(LK_INIT_LEVEL_TARGET, LK_INIT_LEVEL_APPS - 1);
    apps_init();

    lk_primary_cpu_init_level(LK_INIT_LEVEL_APPS, LK_INIT_LEVEL_LAST);

    return 0;
}

#if WITH_SMP
void lk_secondary_cpu_entry(void)
{
    uint cpu = arch_curr_cpu_num();

    if (cpu > secondary_bootstrap_thread_count) {
        dprintf(CRITICAL, "Invalid secondary cpu num %d, SMP_MAX_CPUS %d, secondary_bootstrap_thread_count %d\n",
                cpu, SMP_MAX_CPUS, secondary_bootstrap_thread_count);
        return;
    }

    thread_secondary_cpu_init_early();
    thread_resume(secondary_bootstrap_threads[cpu - 1]);

    dprintf(SPEW, "entering scheduler on cpu %d\n", cpu);
    thread_secondary_cpu_entry();
}

static int secondary_cpu_bootstrap2(void *arg)
{
    /* secondary cpu initialize from threading level up. 0 to threading was handled in arch */
    lk_init_level(LK_INIT_FLAG_SECONDARY_CPUS, LK_INIT_LEVEL_THREADING, LK_INIT_LEVEL_LAST);

    return 0;
}

void lk_init_secondary_cpus(uint secondary_cpu_count)
{
    if (secondary_cpu_count >= SMP_MAX_CPUS) {
        dprintf(CRITICAL, "Invalid secondary_cpu_count %d, SMP_MAX_CPUS %d\n",
                secondary_cpu_count, SMP_MAX_CPUS);
        secondary_cpu_count = SMP_MAX_CPUS - 1;
    }
    for (uint i = 0; i < secondary_cpu_count; i++) {
        dprintf(SPEW, "creating bootstrap completion thread for cpu %d\n", i + 1);
        thread_t *t = thread_create("secondarybootstrap2",
                                    &secondary_cpu_bootstrap2, NULL,
                                    DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
        t->pinned_cpu = i + 1;
        thread_detach(t);
        secondary_bootstrap_threads[i] = t;
    }
    secondary_bootstrap_thread_count = secondary_cpu_count;
}
#endif
