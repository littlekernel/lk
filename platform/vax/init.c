/*
 * Copyright (c) 2019 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/debug.h>
#include <platform.h>
#include <platform/timer.h>

// stubbed out time
static lk_time_t t = 0;
lk_time_t current_time() {
    return ++t;
}

lk_bigtime_t current_time_hires() {
    return (++t) * 1000;
}
