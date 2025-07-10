// Copyright (c) 2008-2009 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
#pragma once

#include <lk/compiler.h>
#include <lk/list.h>
#include <sys/types.h>
#include <stdint.h>

// This file defines the timer API for the LK kernel.
// Timers are used to schedule callbacks to occur at a specific time in the future.

// Rules for Timers:
// - Timer callbacks occur from interrupt context.
// - Timers may be programmed or canceled from interrupt or thread context.
// - Timers may be canceled or reprogrammed from within their callback.
//

__BEGIN_CDECLS

// Initializes the timer subsystem.
void timer_init(void);

struct timer;
typedef enum handler_return (*timer_callback)(struct timer *, lk_time_t now, void *arg);

#define TIMER_MAGIC (0x74696D72)  //'timr'

typedef struct timer {
    uint32_t magic;
    struct list_node node;

    lk_time_t scheduled_time;
    lk_time_t periodic_time;

    timer_callback callback;
    void *arg;
} timer_t;

// Initializes a timer to the default state. Can statically initialize a timer
// with the TIMER_INITIAL_VALUE macro or dynamically initialize it with timer_initialize().
#define TIMER_INITIAL_VALUE(t) \
{ \
    .magic = TIMER_MAGIC, \
    .node = LIST_INITIAL_CLEARED_VALUE, \
    .scheduled_time = 0, \
    .periodic_time = 0, \
    .callback = NULL, \
    .arg = NULL, \
}

void timer_initialize(timer_t *);

// Sets a timer to fire once after a delay.
void timer_set_oneshot(timer_t *, lk_time_t delay, timer_callback, void *arg);

// Sets a timer to fire periodically at the specified interval.
void timer_set_periodic(timer_t *, lk_time_t period, timer_callback, void *arg);

// Cancels a timer, removing it from the timer queue and preventing it from firing.
// If the timer is currently running, it will not be canceled until the callback returns.
// May be called from interrupt or thread context.
// The callback will not be called again after this.
void timer_cancel(timer_t *);

__END_CDECLS
