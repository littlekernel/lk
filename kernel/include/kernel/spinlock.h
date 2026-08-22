// Copyright (c) 2014 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
#pragma once

#include <arch/defines.h>
#include <arch/interrupts.h>
#include <arch/ops.h>
#include <arch/spinlock.h>
#include <assert.h>
#include <lk/compiler.h>
#include <lk/debug.h>
#include <sys/types.h>

__BEGIN_CDECLS

// Debug-only record of which locks the current cpu holds.
//
// arch_spin_lock_held() can only answer "is this lock held by *somebody*". That
// is a serviceable proxy while the kernel has exactly one lock -- interrupts are
// disabled and there is only the one, so if it is held it is probably held by
// you -- and it is worse than useless once there are several:
// spin_lock_held(sched_lock(remote)) is nearly always true and asserts nothing.
//
// The ownership record therefore lives *beside* the lock, never inside the lock
// word. Encoding an owner into the word would foreclose ticket locks, MCS locks
// and riscv's single-instruction amoswap acquire; a per-cpu array touches no
// arch code at all, does not change spin_lock_t, and does not disturb any
// SPIN_LOCK_INITIAL_VALUE site.
//
// Correctness under LK's lock handoff: the thread lock is acquired by one thread
// and released by another across a context switch, but always on the *same*
// physical cpu, since the incoming thread resumes on the cpu that switched. So a
// per-cpu record stays balanced where a per-thread one would not. (This is the
// same trap that killed the idea of bracketing THREAD_LOCK with a per-thread
// preempt counter -- see kernel/preempt.h.) Release is a search-and-remove
// rather than a stack pop for the same reason: locks are not guaranteed to be
// dropped in acquisition order across a handoff.
#if LK_DEBUGLEVEL > 1
#define SPIN_LOCK_TRACK_HELD 1
#endif

#if SPIN_LOCK_TRACK_HELD

// Deeper than anything in the tree nests; the assert below is the real check.
#define SPIN_LOCK_HELD_MAX 8

// One per cpu, each on its own cache line: every spin_lock() and spin_unlock()
// writes here, and adjacent cpus sharing a line turns that into a line
// bouncing between them on every lock operation.
struct spin_lock_held_state {
    spin_lock_t *locks[SPIN_LOCK_HELD_MAX];
    uint count;
} __CPU_ALIGN;

extern struct spin_lock_held_state spin_lock_held_state[SMP_MAX_CPUS];

// The failure paths stay out of line in spinlock.c. These helpers are inlined
// into every spin_lock()/spin_unlock() in the kernel, so a panic() with a format
// string in the body costs argument and string setup at hundreds of sites -- it
// measured ~20KB of text in a debug arm64 build. Keep the inline part to the
// array manipulation only.
void spin_lock_held_overflow(void) __NO_RETURN;
void spin_lock_held_not_held(spin_lock_t *lock) __NO_RETURN;

static inline void spin_lock_held_acquired(spin_lock_t *lock) {
    // Safe without further synchronization: spin_lock() requires interrupts to
    // be disabled already, so the current cpu number is stable and nothing else
    // can be touching this cpu's slot.
    struct spin_lock_held_state *s = &spin_lock_held_state[arch_curr_cpu_num()];
    if (unlikely(s->count >= SPIN_LOCK_HELD_MAX)) {
        spin_lock_held_overflow();
    }
    s->locks[s->count++] = lock;
}

static inline void spin_lock_held_released(spin_lock_t *lock) {
    struct spin_lock_held_state *s = &spin_lock_held_state[arch_curr_cpu_num()];
    for (uint i = 0; i < s->count; i++) {
        if (s->locks[i] == lock) {
            // order within the array carries no meaning, so fill the hole from
            // the end rather than shifting everything down
            s->locks[i] = s->locks[--s->count];
            return;
        }
    }
    spin_lock_held_not_held(lock);
}

#endif // SPIN_LOCK_TRACK_HELD

// interrupts should already be disabled
static inline void spin_lock(spin_lock_t *lock) {
    arch_spin_lock(lock);
#if SPIN_LOCK_TRACK_HELD
    spin_lock_held_acquired(lock);
#endif
}

// Returns 0 on success, non-0 on failure
static inline int spin_trylock(spin_lock_t *lock) {
    int ret = arch_spin_trylock(lock);
#if SPIN_LOCK_TRACK_HELD
    if (ret == 0) {
        spin_lock_held_acquired(lock);
    }
#endif
    return ret;
}

// interrupts should already be disabled
static inline void spin_unlock(spin_lock_t *lock) {
#if SPIN_LOCK_TRACK_HELD
    spin_lock_held_released(lock);
#endif
    arch_spin_unlock(lock);
}

static inline void spin_lock_init(spin_lock_t *lock) {
    arch_spin_lock_init(lock);
}

// Is this lock held by anyone at all? Rarely what you want once there is more
// than one lock -- prefer spin_lock_held_by_me().
static inline bool spin_lock_held(spin_lock_t *lock) {
    return arch_spin_lock_held(lock);
}

// Is this lock held by the cpu asking? This is the predicate the "you must call
// me with X held" asserts actually mean. Outside debug builds there is no
// ownership record, so it degrades to spin_lock_held() and the asserts weaken
// back to what they check today rather than becoming false.
static inline bool spin_lock_held_by_me(spin_lock_t *lock) {
#if SPIN_LOCK_TRACK_HELD
    const struct spin_lock_held_state *s = &spin_lock_held_state[arch_curr_cpu_num()];
    for (uint i = 0; i < s->count; i++) {
        if (s->locks[i] == lock) {
            return true;
        }
    }
    return false;
#else
    return spin_lock_held(lock);
#endif
}

// same as spin lock, but save disable and save interrupt state first
static inline arch_interrupt_saved_state_t spin_lock_irqsave(spin_lock_t *lock) {
    arch_interrupt_saved_state_t state = arch_interrupt_save();
    spin_lock(lock);
    return state;
}

// restore interrupt state before unlocking
static inline void spin_unlock_irqrestore(spin_lock_t *lock, arch_interrupt_saved_state_t old_state) {
    spin_unlock(lock);
    arch_interrupt_restore(old_state);
}

__END_CDECLS

#ifdef __cplusplus

#include <assert.h>
#include <lk/cpp.h>

// C++ wrapper around a C spinlock_t
class SpinLock {
  public:
    constexpr SpinLock() = default;
    ~SpinLock() { DEBUG_ASSERT(!is_held()); }

    void lock() { spin_lock(&lock_); }
    int trylock() { return spin_trylock(&lock_); }
    void unlock() { spin_unlock(&lock_); }
    bool is_held() { return spin_lock_held(&lock_); }

    arch_interrupt_saved_state_t lock_irqsave() {
        return spin_lock_irqsave(&lock_);
    }

    void unlock_irqrestore(arch_interrupt_saved_state_t state) {
        spin_unlock_irqrestore(&lock_, state);
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
    explicit AutoSpinLock(spin_lock_t *lock) : lock_(lock) { state_ = spin_lock_irqsave(lock_); }
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
    arch_interrupt_saved_state_t state_;
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
