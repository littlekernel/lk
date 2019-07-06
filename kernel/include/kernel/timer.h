/*
 * Copyright (c) 2008-2009 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lk/compiler.h>
#include <lk/list.h>
#include <sys/types.h>

__BEGIN_CDECLS

void timer_init(void);

struct timer;
typedef enum handler_return (*timer_callback)(struct timer *, lk_time_t now, void *arg);

#define TIMER_MAGIC (0x74696D72)  //'timr'

typedef struct timer {
    int magic;
    struct list_node node;

    lk_time_t scheduled_time;
    lk_time_t periodic_time;

    timer_callback callback;
    void *arg;
} timer_t;

#define TIMER_INITIAL_VALUE(t) \
{ \
    .magic = TIMER_MAGIC, \
    .node = LIST_INITIAL_CLEARED_VALUE, \
    .scheduled_time = 0, \
    .periodic_time = 0, \
    .callback = NULL, \
    .arg = NULL, \
}

/* Rules for Timers:
 * - Timer callbacks occur from interrupt context
 * - Timers may be programmed or canceled from interrupt or thread context
 * - Timers may be canceled or reprogrammed from within their callback
 * - Timers currently are dispatched from a 10ms periodic tick
*/
void timer_initialize(timer_t *);
void timer_set_oneshot(timer_t *, lk_time_t delay, timer_callback, void *arg);
void timer_set_periodic(timer_t *, lk_time_t period, timer_callback, void *arg);
void timer_cancel(timer_t *);

__END_CDECLS
