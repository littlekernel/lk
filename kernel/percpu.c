// Copyright (c) 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#include <kernel/percpu.h>

#include <lk/compiler.h>
#include <stddef.h>

/* Plain bss, no initializer and no init routine: spinlocks are taken and the
 * ownership record written long before any init hook runs, so anything that
 * needed to be called first would either fault or silently mistrack early
 * boot. Zeroed bss is the correct starting state for the locks and the
 * counters; the list heads are set up by sched_init_early() and timer_init()
 * before anything is queued on them. */
struct percpu percpu_array[SMP_MAX_CPUS];

#if WITH_SMP
/* The layout in the header is only worth anything if the groups really do land
 * on separate lines; check it here rather than trusting the attributes. */
STATIC_ASSERT(offsetof(struct percpu, sched_lock) == 0);
STATIC_ASSERT(offsetof(struct percpu, runnable_count) == CACHE_LINE);
STATIC_ASSERT(offsetof(struct percpu, run_queue) == 2 * CACHE_LINE);
STATIC_ASSERT(offsetof(struct percpu, timer_lock) % CACHE_LINE == 0);
STATIC_ASSERT(sizeof(struct percpu) % CACHE_LINE == 0);
#endif
