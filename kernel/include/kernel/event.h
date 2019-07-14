/*
 * Copyright (c) 2008-2014 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <kernel/thread.h>
#include <lk/compiler.h>
#include <stdbool.h>
#include <sys/types.h>

__BEGIN_CDECLS

#define EVENT_MAGIC (0x65766E74)  // "evnt"

typedef struct event {
    int magic;
    bool signaled;
    uint flags;
    wait_queue_t wait;
} event_t;

#define EVENT_FLAG_AUTOUNSIGNAL 1

#define EVENT_INITIAL_VALUE(e, initial, _flags) \
{ \
    .magic = EVENT_MAGIC, \
    .signaled = initial, \
    .flags = _flags, \
    .wait = WAIT_QUEUE_INITIAL_VALUE((e).wait), \
}

/* Rules for Events:
 * - Events may be signaled from interrupt context *but* the reschedule
 *   parameter must be false in that case.
 * - Events may not be waited upon from interrupt context.
 * - Events without FLAG_AUTOUNSIGNAL:
 *   - Wake up any waiting threads when signaled.
 *   - Continue to do so (no threads will wait) until unsignaled.
 * - Events with FLAG_AUTOUNSIGNAL:
 *   - If one or more threads are waiting when signaled, one thread will
 *     be woken up and return.  The signaled state will not be set.
 *   - If no threads are waiting when signaled, the Event will remain
 *     in the signaled state until a thread attempts to wait (at which
 *     time it will unsignal atomicly and return immediately) or
 *     event_unsignal() is called.
*/

void event_init(event_t *, bool initial, uint flags);
void event_destroy(event_t *);
status_t event_wait_timeout(event_t *, lk_time_t); /* wait on the event with a timeout */
status_t event_signal(event_t *, bool reschedule);
status_t event_unsignal(event_t *);

static inline bool event_initialized(event_t *e) {
    return e->magic == EVENT_MAGIC;
}

static inline status_t event_wait(event_t *e) {
    return event_wait_timeout(e, INFINITE_TIME);
}

__END_CDECLS
