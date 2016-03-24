/*
 * Copyright (c) 2014 Travis Geiselbrecht
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
#pragma once

#include <arch/ops.h>
#include <stdbool.h>

#define SPIN_LOCK_INITIAL_VALUE (0)

typedef unsigned long spin_lock_t;

typedef unsigned int spin_lock_saved_state_t;
typedef unsigned int spin_lock_save_flags_t;

#if WITH_SMP
void arch_spin_lock(spin_lock_t *lock);
int arch_spin_trylock(spin_lock_t *lock);
void arch_spin_unlock(spin_lock_t *lock);
#else
static inline void arch_spin_lock(spin_lock_t *lock)
{
    *lock = 1;
}

static inline int arch_spin_trylock(spin_lock_t *lock)
{
    return 0;
}

static inline void arch_spin_unlock(spin_lock_t *lock)
{
    *lock = 0;
}
#endif

static inline void arch_spin_lock_init(spin_lock_t *lock)
{
    *lock = SPIN_LOCK_INITIAL_VALUE;
}

static inline bool arch_spin_lock_held(spin_lock_t *lock)
{
    return *lock != 0;
}

enum {
    /* Possible future flags:
     * SPIN_LOCK_FLAG_PMR_MASK         = 0x000000ff,
     * SPIN_LOCK_FLAG_PREEMPTION       = 0x10000000,
     * SPIN_LOCK_FLAG_SET_PMR          = 0x20000000,
     */

    /* ARM specific flags */
    SPIN_LOCK_FLAG_IRQ              = 0x40000000,
    SPIN_LOCK_FLAG_FIQ              = 0x80000000, /* Do not use unless IRQs are already disabled */
    SPIN_LOCK_FLAG_IRQ_FIQ          = SPIN_LOCK_FLAG_IRQ | SPIN_LOCK_FLAG_FIQ,

    /* Generic flags */
    SPIN_LOCK_FLAG_INTERRUPTS       = SPIN_LOCK_FLAG_IRQ,
};

/* default arm flag is to just disable plain irqs */
#define ARCH_DEFAULT_SPIN_LOCK_FLAG_INTERRUPTS  SPIN_LOCK_FLAG_INTERRUPTS

enum {
    /* private */
    SPIN_LOCK_STATE_RESTORE_IRQ = 1,
    SPIN_LOCK_STATE_RESTORE_FIQ = 2,
};

static inline void
arch_interrupt_save(spin_lock_saved_state_t *statep, spin_lock_save_flags_t flags)
{
    spin_lock_saved_state_t state = 0;
    if ((flags & SPIN_LOCK_FLAG_IRQ) && !arch_ints_disabled()) {
        state |= SPIN_LOCK_STATE_RESTORE_IRQ;
        arch_disable_ints();
    }
    if ((flags & SPIN_LOCK_FLAG_FIQ) && !arch_fiqs_disabled()) {
        state |= SPIN_LOCK_STATE_RESTORE_FIQ;
        arch_disable_fiqs();
    }
    *statep = state;
}

static inline void
arch_interrupt_restore(spin_lock_saved_state_t old_state, spin_lock_save_flags_t flags)
{
    if ((flags & SPIN_LOCK_FLAG_FIQ) && (old_state & SPIN_LOCK_STATE_RESTORE_FIQ))
        arch_enable_fiqs();
    if ((flags & SPIN_LOCK_FLAG_IRQ) && (old_state & SPIN_LOCK_STATE_RESTORE_IRQ))
        arch_enable_ints();
}



