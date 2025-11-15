// Copyright (c) 2025 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#pragma once

#include <lk/compiler.h>
#include <stdbool.h>

__BEGIN_CDECLS

// Disable preemption of the current CPU, deferring reschedules caused by wakeups.
void preempt_disable(void);

// Enable preemption for the current CPU. If the local preempt count reaches zero
// and a wakeup occurred while disabled, this will trigger a reschedule at a safe point.
void preempt_enable(void);

// Enable preemption without rescheduling. Drops the local preempt count and
// clears any pending reschedule flag, but never performs a reschedule itself.
// Returns true if a reschedule should occur as soon as the caller exits its
// critical context (e.g., at end of interrupt), false otherwise.
bool preempt_enable_no_resched(void);

// Set a pending reschedule if preemption is currently disabled.
// Returns true if preemption was disabled, false otherwise.
bool preempt_set_pending_if_disabled(void);

__END_CDECLS
