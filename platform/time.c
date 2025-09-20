/*
 * Copyright (c) 2025 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <platform/time.h>

void spin(uint32_t usecs) {
    lk_bigtime_t start = current_time_hires();

    while ((current_time_hires() - start) < usecs)
        ;
}
