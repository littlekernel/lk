/*
 * Copyright (c) 2013 Google Inc.
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lk/compiler.h>
#include <stdbool.h>
#include <stdint.h>
#include <kernel/thread.h>
#include <kernel/timer.h>

#define WATCHDOG_MAGIC 'wdog'

__BEGIN_CDECLS

typedef struct watchdog {
    uint32_t                magic;
    const char             *name;
    bool                    enabled;
    lk_time_t               timeout;
    timer_t                 expire_timer;
} watchdog_t;

/* A global weak-reference to the common watchdog handler.  By default, this
 * function will dprintf a message indicating which watchdog fired, and then
 * request a halt using platform_halt, indicating that a SW watchdog was the
 * reason for the reset.  It is the platform/target's responsibility to either
 * implement the panic behavior they want when a watchdog fires in
 * platform_halt, or to replace the implementation of the watchdog handler with
 * their own appropriate implementation.
 */
void watchdog_handler(watchdog_t *dog) __NO_RETURN;

status_t watchdog_init(watchdog_t *dog, lk_time_t timeout, const char *name);
void     watchdog_set_enabled(watchdog_t *dog, bool enabled);
void     watchdog_pet(watchdog_t *dog);

/* HW watchdog support.  This is nothing but a simple helper used to
 * automatically dismiss a platform's HW watchdog using LK timers.  Platforms
 * must supply
 *
 * platform_watchdog_init
 * platform_watchdog_set_enabled
 * platform_watchdog_pet
 *
 * in order to use the HW watchdog helper functions.  After initialized, users
 * may enable and disable the HW watchdog whenever appropriate.  The helper will
 * maintain a timer which dismisses the watchdog at the pet interval recommended
 * by the platform.  Any programming error which prevents the scheduler timer
 * mechanism from running properly will eventually result in the watchdog firing
 * and the system rebooting.  Whenever possible, when using SW based watchdogs,
 * it is recommended that systems provide platform support for a HW watchdog and
 * enable the HW watchdog.  SW watchdogs are based on LK timers, and should be
 * reliable as long as the scheduler and timer mechanism is running properly;
 * the HW watchdog functionality provided here should protect the system in case
 * something managed to break timers on LK.
 */

extern status_t platform_watchdog_init(lk_time_t  target_timeout,
                                       lk_time_t *recommended_pet_period);
extern void platform_watchdog_set_enabled(bool enabled);
extern void platform_watchdog_pet(void);

status_t watchdog_hw_init(lk_time_t timeout);
void watchdog_hw_set_enabled(bool enabled);

__END_CDECLS
