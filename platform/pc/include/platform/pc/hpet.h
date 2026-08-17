/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

// Look up the ACPI HPET table, map its registers, and start its free-running
// counter. Returns NO_ERROR only if a usable HPET was found and initialized.
// Safe to call more than once; later calls are a no-op.
status_t hpet_init(void);

// True once hpet_init() has found and set up a usable HPET.
bool hpet_is_available(void);

// Measure the TSC frequency in Hz against the HPET counter. Returns 0 if no HPET
// is available.
uint64_t hpet_calibrate_tsc(void);

lk_time_t hpet_current_time(void);
lk_bigtime_t hpet_current_time_hires(void);
