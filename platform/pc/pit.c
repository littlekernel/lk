/*
 * Copyright (c) 2009 Corey Tabaka
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <sys/types.h>
#include <lk/err.h>
#include <lk/reg.h>
#include <lk/debug.h>
#include <lk/trace.h>
#include <kernel/thread.h>
#include <kernel/spinlock.h>
#include <platform.h>
#include <platform/interrupts.h>
#include <platform/console.h>
#include <platform/timer.h>
#include <platform/pc.h>
#include "platform_p.h"
#include <arch/x86.h>
#include <inttypes.h>

#define LOCAL_TRACE 0

static platform_timer_callback t_callback;
static void *callback_arg;
static spin_lock_t lock = SPIN_LOCK_INITIAL_VALUE;

static uint64_t ticks_per_ms;

// next callback event time in 32.32 fixed point milliseconds
static uint64_t next_trigger_time;

// if periodic, the delta to set to the next event. if oneshot, 0
static uint64_t next_trigger_delta;

// time in 32.32 fixed point milliseconds
static volatile uint64_t timer_current_time;
// delta time per periodic tick in 32.32
static uint64_t timer_delta_time;

#define INTERNAL_FREQ 1193182ULL
#define INTERNAL_FREQ_3X 3579546ULL

/* Maximum amount of time that can be program on the timer to schedule the next
 *  interrupt, in milliseconds */
#define MAX_TIMER_INTERVAL 55

lk_time_t pit_current_time(void) {
    spin_lock_saved_state_t state;
    spin_lock_irqsave(&lock, state);

    lk_time_t time = (lk_time_t) (timer_current_time >> 32);

    spin_unlock_irqrestore(&lock, state);

    return time;
}

lk_bigtime_t pit_current_time_hires(void) {
    spin_lock_saved_state_t state;
    spin_lock_irqsave(&lock, state);

    lk_bigtime_t time = (lk_bigtime_t) ((timer_current_time >> 22) * 1000) >> 10;

    spin_unlock_irqrestore(&lock, state);

    return time;
}

static enum handler_return pit_timer_tick(void *arg) {
    if (next_trigger_time != 0 || next_trigger_delta) {
        LTRACEF("ntt %#" PRIx64 ", ntd %#" PRIx64 "\n", next_trigger_time, next_trigger_delta);
    }

    spin_lock(&lock);
    timer_current_time += timer_delta_time;
    spin_unlock(&lock);

    lk_time_t time = current_time();

    if (t_callback && next_trigger_time != 0 && timer_current_time >= next_trigger_time) {
        if (next_trigger_delta != 0) {
            uint64_t delta = timer_current_time - next_trigger_time;
            next_trigger_time = timer_current_time + next_trigger_delta - delta;
        } else {
            next_trigger_time = 0;
        }

        return t_callback(callback_arg, time);
    } else {
        return INT_NO_RESCHEDULE;
    }
}

static void set_pit_frequency(uint32_t frequency) {
    uint32_t count, remainder;

    LTRACEF("frequency %u\n", frequency);

    /* figure out the correct divisor for the desired frequency */
    if (frequency <= 18) {
        count = 0xffff;
    } else if (frequency >= INTERNAL_FREQ) {
        count = 1;
    } else {
        count = INTERNAL_FREQ_3X / frequency;
        remainder = INTERNAL_FREQ_3X % frequency;

        if (remainder >= INTERNAL_FREQ_3X / 2) {
            count += 1;
        }

        count /= 3;
        remainder = count % 3;

        if (remainder >= 1) {
            count += 1;
        }
    }

    uint16_t divisor = count & 0xffff;

    /*
     * funky math that i don't feel like explaining. essentially 32.32 fixed
     * point representation of the configured timer delta.
     */
    timer_delta_time = (3685982306ULL * count) >> 10;

    LTRACEF("dt 0x%016" PRIx64 "\n", timer_delta_time);
    LTRACEF("divisor 0x%04" PRIx16 "\n", divisor);

    /*
     * setup the Programmable Interval Timer
     * timer 0, mode 2, binary counter, LSB followed by MSB
     */
    outp(I8253_CONTROL_REG, 0x34);
    outp(I8253_DATA_REG, divisor & 0xff); // LSB
    outp(I8253_DATA_REG, divisor >> 8); // MSB
}

void pit_init(void) {
    timer_current_time = 0;
    ticks_per_ms = INTERNAL_FREQ/1000;
    set_pit_frequency(1000); // ~1ms granularity
    register_int_handler(INT_PIT, &pit_timer_tick, NULL);
}

status_t pit_set_periodic_timer(platform_timer_callback callback, void *arg, lk_time_t interval) {
    LTRACEF("pit_set_periodic_timer: interval %u\n", interval);

    spin_lock_saved_state_t state;
    spin_lock_irqsave(&lock, state);

    t_callback = callback;
    callback_arg = arg;

    next_trigger_delta = (uint64_t) interval << 32;
    next_trigger_time = timer_current_time + next_trigger_delta;

    unmask_interrupt(INT_PIT);
    spin_unlock_irqrestore(&lock, state);

    return NO_ERROR;
}

status_t pit_set_oneshot_timer(platform_timer_callback callback, void *arg, lk_time_t interval) {
    LTRACEF("pit_set_oneshot_timer: interval %u\n", interval);

    spin_lock_saved_state_t state;
    spin_lock_irqsave(&lock, state);

    t_callback = callback;
    callback_arg = arg;

    next_trigger_delta = 0;
    next_trigger_time = timer_current_time + ((uint64_t)interval << 32);

    unmask_interrupt(INT_PIT);
    spin_unlock_irqrestore(&lock, state);

    return NO_ERROR;
}

void pit_stop_timer(void) {
    LTRACE;

    spin_lock_saved_state_t state;
    spin_lock_irqsave(&lock, state);

    mask_interrupt(INT_PIT);
    spin_unlock_irqrestore(&lock, state);
}