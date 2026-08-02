/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <sys/types.h>

// Look up the ACPI HPET table, map its registers, and start its free-running
// counter. Returns NO_ERROR only if a usable HPET was found and initialized.
status_t hpet_init(void);

lk_time_t hpet_current_time(void);
lk_bigtime_t hpet_current_time_hires(void);
