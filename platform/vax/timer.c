/*
 * Copyright (c) 2019 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/debug.h>
#include <lk/err.h>
#include <platform.h>
#include <platform/timer.h>

// stubbed out timer
status_t platform_set_periodic_timer(platform_timer_callback callback, void *arg, lk_time_t interval) {
    return NO_ERROR;
    PANIC_UNIMPLEMENTED;
}

