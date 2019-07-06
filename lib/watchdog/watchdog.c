/*
 * Copyright (c) 2013 Google Inc.
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <assert.h>
#include <lk/compiler.h>
#include <lk/err.h>
#include <platform.h>

#include <kernel/thread.h>
#include <kernel/timer.h>
#include <kernel/spinlock.h>

#include <lib/watchdog.h>

static spin_lock_t lock = SPIN_LOCK_INITIAL_VALUE;

__WEAK void watchdog_handler(watchdog_t *dog) {
    dprintf(INFO, "Watchdog \"%s\" (timeout %u mSec) just fired!!\n",
            dog->name, (uint32_t)dog->timeout);
    platform_halt(HALT_ACTION_HALT, HALT_REASON_SW_WATCHDOG);
}

static enum handler_return watchdog_timer_callback(struct timer *timer, lk_time_t now, void *arg) {
    watchdog_handler((watchdog_t *)arg);

    /* We should never get here; watchdog handlers should always be fatal. */
    DEBUG_ASSERT(false);

    return INT_NO_RESCHEDULE;
}

status_t watchdog_init(watchdog_t *dog, lk_time_t timeout, const char *name) {
    DEBUG_ASSERT(NULL != dog);
    DEBUG_ASSERT(INFINITE_TIME != timeout);

    dog->magic   = WATCHDOG_MAGIC;
    dog->name    = name ? name : "unnamed watchdog";
    dog->enabled = false;
    dog->timeout = timeout;
    timer_initialize(&dog->expire_timer);

    return NO_ERROR;
}

void watchdog_set_enabled(watchdog_t *dog, bool enabled) {
    spin_lock_saved_state_t state;
    spin_lock_irqsave(&lock, state);

    DEBUG_ASSERT((NULL != dog) && (WATCHDOG_MAGIC == dog->magic));

    if (dog->enabled == enabled)
        goto done;

    dog->enabled = enabled;
    if (enabled)
        timer_set_oneshot(&dog->expire_timer, dog->timeout, watchdog_timer_callback, dog);
    else
        timer_cancel(&dog->expire_timer);

done:
    spin_unlock_irqrestore(&lock, state);
}

void watchdog_pet(watchdog_t *dog) {
    spin_lock_saved_state_t state;
    spin_lock_irqsave(&lock, state);

    DEBUG_ASSERT((NULL != dog) && (WATCHDOG_MAGIC == dog->magic));

    if (!dog->enabled)
        goto done;

    timer_cancel(&dog->expire_timer);
    timer_set_oneshot(&dog->expire_timer, dog->timeout, watchdog_timer_callback, dog);

done:
    spin_unlock_irqrestore(&lock, state);
}


static timer_t   hw_watchdog_timer;
static bool      hw_watchdog_enabled;
static lk_time_t hw_watchdog_pet_timeout;

static enum handler_return hw_watchdog_timer_callback(struct timer *timer, lk_time_t now, void *arg) {
    platform_watchdog_pet();
    return INT_NO_RESCHEDULE;
}

status_t watchdog_hw_init(lk_time_t timeout) {
    DEBUG_ASSERT(INFINITE_TIME != timeout);
    timer_initialize(&hw_watchdog_timer);
    return platform_watchdog_init(timeout, &hw_watchdog_pet_timeout);
}

void watchdog_hw_set_enabled(bool enabled) {
    spin_lock_saved_state_t state;
    spin_lock_irqsave(&lock, state);

    if (hw_watchdog_enabled == enabled)
        goto done;

    hw_watchdog_enabled = enabled;
    platform_watchdog_set_enabled(enabled);
    if (enabled)
        timer_set_periodic(&hw_watchdog_timer,
                           hw_watchdog_pet_timeout,
                           hw_watchdog_timer_callback,
                           NULL);
    else
        timer_cancel(&hw_watchdog_timer);

done:
    spin_unlock_irqrestore(&lock, state);
}
