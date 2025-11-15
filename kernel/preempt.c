// Copyright (c) 2025
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#include <kernel/preempt.h>

#include <arch/defines.h>
#include <arch/ops.h>
#include <assert.h>
#include <kernel/thread.h>
#include <lk/debug.h>

static inline int *get_local_count_ptr(void) {
    return &get_current_thread()->preempt_disable_count;
}

static inline bool *get_local_pending_ptr(void) {
    return &get_current_thread()->pending_reschedule;
}

static inline int bump_preempt_count(int delta) {
    CF;
    int *cntp = get_local_count_ptr();
    int old = *cntp;
    *cntp += delta;
    CF;
    return old;
}

static inline bool preempt_is_disabled(void) {
    return *get_local_count_ptr() != 0;
}

static inline void preempt_set_pending(void) {
    *get_local_pending_ptr() = true;
}

static inline bool preempt_clear_pending(void) {
    bool *pendp = get_local_pending_ptr();
    bool was_pending = *pendp;
    *pendp = false;
    return was_pending;
}

void preempt_disable(void) {
    int old = bump_preempt_count(1);
    DEBUG_ASSERT(old >= 0);
}

void preempt_enable(void) {
    // Drop the count; if it hits zero and a wakeup is pending, clear it and yield.
    int old = bump_preempt_count(-1);
    DEBUG_ASSERT(old > 0);
    if (old == 1) {
        if (preempt_clear_pending()) {
            // If we need to reschedule due to a pending wakeup, do it now.
            thread_preempt();
        }
    }
}

bool preempt_enable_no_resched(void) {
    // Drop the count; if it hits zero and a wakeup is pending, clear it and return if
    // a reschedule should occur.
    bool should_resched = false;
    int old = bump_preempt_count(-1);
    DEBUG_ASSERT(old > 0);
    if (old == 1) {
        should_resched = preempt_clear_pending();
    }

    return should_resched;
}

bool preempt_set_pending_if_disabled(void) {
    if (preempt_is_disabled()) {
        preempt_set_pending();
        return true;
    }
    return false;
}
