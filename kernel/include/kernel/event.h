// Copyright (c) 2008-2014 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
#pragma once

#include <kernel/thread.h>
#include <lk/compiler.h>
#include <stdbool.h>
#include <sys/types.h>
#include <stdint.h>

// This file defines the kernel event, which is a synchronization primitive
// that allows threads to wait for an event to be signaled. It is primarily used
// for inter-thread communication and synchronization.

__BEGIN_CDECLS

// Rules for Events:
// - Events may be signaled from interrupt context *but* the reschedule
//   parameter must be false in that case.
// - Events may not be waited upon from interrupt context.
// - Events without FLAG_AUTOUNSIGNAL:
//   - Wake up any waiting threads when signaled.
//   - Continue to do so (no threads will wait) until unsignaled.
// - Events with FLAG_AUTOUNSIGNAL:
//   - If one or more threads are waiting when signaled, one thread will
//     be woken up and return.  The signaled state will not be set.
//   - If no threads are waiting when signaled, the Event will remain
//     in the signaled state until a thread attempts to wait (at which
//     time it will unsignal atomicly and return immediately) or
//     event_unsignal() is called.

#define EVENT_MAGIC (0x65766E74)  // "evnt"

typedef struct event {
    uint32_t magic;
    bool signaled;
    uint flags;
    wait_queue_t wait;
} event_t;

#define EVENT_FLAG_AUTOUNSIGNAL 1

// Initializer for an event structure. Events may be initialized with a
// non-signaled state, or a signaled state. The flags parameter can be used
// to specify additional behavior, such as FLAG_AUTOUNSIGNAL.
#define EVENT_INITIAL_VALUE(e, initial, _flags) \
{ \
    .magic = EVENT_MAGIC, \
    .signaled = (initial), \
    .flags = (_flags), \
    .wait = WAIT_QUEUE_INITIAL_VALUE((e).wait), \
}

// Dynamically initialize an event structure.
void event_init(event_t *, bool initial, uint flags);

// Check if an event is initialized.
static inline bool event_initialized(event_t *e) {
    return e->magic == EVENT_MAGIC;
}

// Destroy an event. Any threads waiting on the event will be woken up
// with an error status.
void event_destroy(event_t *);

// Wait on the event until it is signaled or a timeout occurs.
// If timeout is INFINITE_TIME, it will wait indefinitely.
// If timeout is 0, it will return immediately if it is unable to acquire the event.
// Returns an error status if the event is not signaled within the timeout period.
// If the event is signaled, it will return NO_ERROR.
status_t event_wait_timeout(event_t *, lk_time_t);

// Signal the event, waking up any threads waiting on it.
// If reschedule is true, it will reschedule the thread that was waiting.
// May be called during interrupt context, but in that case reschedule must be false.
status_t event_signal(event_t *, bool reschedule);

// Unsignal the event, clearing its signaled state.
status_t event_unsignal(event_t *);

// Shortcut for waiting for an event indefinitely.
static inline status_t event_wait(event_t *e) {
    return event_wait_timeout(e, INFINITE_TIME);
}

__END_CDECLS
