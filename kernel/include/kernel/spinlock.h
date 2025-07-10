// Copyright (c) 2014 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
#pragma once

#include <arch/spinlock.h>
#include <lk/compiler.h>

__BEGIN_CDECLS

// interrupts should already be disabled
static inline void spin_lock(spin_lock_t *lock) {
    arch_spin_lock(lock);
}

// Returns 0 on success, non-0 on failure
static inline int spin_trylock(spin_lock_t *lock) {
    return arch_spin_trylock(lock);
}

// interrupts should already be disabled
static inline void spin_unlock(spin_lock_t *lock) {
    arch_spin_unlock(lock);
}

static inline void spin_lock_init(spin_lock_t *lock) {
    arch_spin_lock_init(lock);
}

static inline bool spin_lock_held(spin_lock_t *lock) {
    return arch_spin_lock_held(lock);
}

// spin lock irq save flags:

// Possible future flags:
// SPIN_LOCK_FLAG_PMR_MASK         = 0x000000ff
// SPIN_LOCK_FLAG_PREEMPTION       = 0x00000100
// SPIN_LOCK_FLAG_SET_PMR          = 0x00000200

// Generic flags
#define SPIN_LOCK_FLAG_INTERRUPTS ARCH_DEFAULT_SPIN_LOCK_FLAG_INTERRUPTS

// same as spin lock, but save disable and save interrupt state first
static inline void spin_lock_save(
    spin_lock_t *lock,
    spin_lock_saved_state_t *statep,
    spin_lock_save_flags_t flags) {
    arch_interrupt_save(statep, flags);
    spin_lock(lock);
}

// restore interrupt state before unlocking
static inline void spin_unlock_restore(
    spin_lock_t *lock,
    spin_lock_saved_state_t old_state,
    spin_lock_save_flags_t flags) {
    spin_unlock(lock);
    arch_interrupt_restore(old_state, flags);
}

// hand(ier) routines
#define spin_lock_irqsave(lock, statep) spin_lock_save(lock, &(statep), SPIN_LOCK_FLAG_INTERRUPTS)
#define spin_unlock_irqrestore(lock, statep) spin_unlock_restore(lock, statep, SPIN_LOCK_FLAG_INTERRUPTS)

__END_CDECLS

#ifdef __cplusplus

#include <lk/cpp.h>
#include <assert.h>

// C++ wrapper around a C spinlock_t
class SpinLock {
public:
    constexpr SpinLock() = default;
    ~SpinLock() { DEBUG_ASSERT(!is_held()); }

    void lock() { spin_lock(&lock_); }
    int trylock() { return spin_trylock(&lock_); }
    void unlock() { spin_unlock(&lock_); }
    bool is_held() { return spin_lock_held(&lock_); }

    void lock_irqsave(spin_lock_saved_state_t *state) {
        spin_lock_save(&lock_, state, SPIN_LOCK_FLAG_INTERRUPTS);
    }

    void unlock_irqrestore(spin_lock_saved_state_t state) {
        spin_unlock_restore(&lock_, state, SPIN_LOCK_FLAG_INTERRUPTS);
    }

    // suppress default constructors
    DISALLOW_COPY_ASSIGN_AND_MOVE(SpinLock);

private:
    spin_lock_t lock_ = SPIN_LOCK_INITIAL_VALUE;

    // friend classes to get to the inner lock
    friend class AutoSpinLock;
    friend class AutoSpinLockNoIrqSave;
};

// RAII wrappers for a spinlock, with and without IRQ Save
class AutoSpinLock {
public:
    explicit AutoSpinLock(spin_lock_t *lock) : lock_(lock) { spin_lock_irqsave(lock_, state_); }
    explicit AutoSpinLock(SpinLock *lock) : AutoSpinLock(&lock->lock_) {}
    ~AutoSpinLock() { release(); }

    void release() {
        if (likely(lock_)) {
            spin_unlock_irqrestore(lock_, state_);
            lock_ = nullptr;
        }
    }

    // suppress default constructors
    DISALLOW_COPY_ASSIGN_AND_MOVE(AutoSpinLock);

private:
    spin_lock_t *lock_;
    spin_lock_saved_state_t state_;
};

class AutoSpinLockNoIrqSave {
public:
    explicit AutoSpinLockNoIrqSave(spin_lock_t *lock) : lock_(lock) { spin_lock(lock_); }
    explicit AutoSpinLockNoIrqSave(SpinLock *lock) : AutoSpinLockNoIrqSave(&lock->lock_) {}
    ~AutoSpinLockNoIrqSave() { release(); }

    void release() {
        if (likely(lock_)) {
            spin_unlock(lock_);
            lock_ = nullptr;
        }
    }

    // suppress default constructors
    DISALLOW_COPY_ASSIGN_AND_MOVE(AutoSpinLockNoIrqSave);

private:
    spin_lock_t *lock_;
};

#endif // __cplusplus
