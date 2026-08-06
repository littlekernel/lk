/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

/**
 * @file
 * @brief  Spinlock ownership tracking
 *
 * Backing storage for the debug-only record of which locks each cpu holds. The
 * interesting part is all in <kernel/spinlock.h>; see the comment there for why
 * this is a separate per-cpu array rather than an owner field inside the lock
 * word.
 */
#include <kernel/spinlock.h>

#if SPIN_LOCK_TRACK_HELD

/* Deliberately plain bss with no initializer and no init routine: spinlocks are
 * taken long before any init hook runs, so anything needing to be called first
 * would either fault or silently mistrack the whole of early boot. Zeroed bss is
 * already the correct starting state.
 */
struct spin_lock_held_state spin_lock_held_state[SMP_MAX_CPUS];

void spin_lock_held_overflow(void) {
    panic("spinlock: cpu %u nested more than %d spinlocks deep\n",
          arch_curr_cpu_num(), SPIN_LOCK_HELD_MAX);
}

void spin_lock_held_not_held(spin_lock_t *lock) {
    /* Either a genuine unbalanced unlock, or the lock was acquired on a
     * different cpu than the one releasing it. The latter is legal for the
     * thread lock alone, which is handed across a context switch -- but always
     * on the same physical cpu, which is what makes this record work. If this
     * fires for the thread lock, a context switch moved a cpu underneath a held
     * lock and that is the bug, not this assert.
     */
    panic("spinlock: cpu %u released lock %p it does not hold\n",
          arch_curr_cpu_num(), lock);
}

#endif
