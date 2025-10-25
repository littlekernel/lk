/*
 * Copyright (c) 2013 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <kernel/init.h>

#include <kernel/debug.h>
#include <kernel/mp.h>
#include <kernel/port.h>
#include <kernel/thread.h>
#include <kernel/timer.h>
#include <lk/compiler.h>
#include <lk/debug.h>

void kernel_init_early(void) {
    // get us into some sort of thread context
    thread_init_early();
}

void kernel_init(void) {
    // if enabled, configure the kernel's event log
    kernel_evlog_init();

#if WITH_SMP
    // initialize multiprocessor support
    dprintf(SPEW, "initializing mp\n");
    mp_init();
#endif

    // initialize the threading system
    dprintf(SPEW, "initializing threads\n");
    thread_init();

    // initialize kernel timers
    dprintf(SPEW, "initializing timers\n");
    timer_init();

    // initialize ports
    dprintf(SPEW, "initializing ports\n");
    port_init();
}

