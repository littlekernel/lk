/*
 * Copyright (c) 2025 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <stdint.h>
#include <sys/types.h>

// A few shared timer routines needed by the arch/x86 layer
uint32_t pit_calibrate_lapic(uint32_t (*lapic_read_tick)(void));
uint64_t time_to_tsc_ticks(lk_time_t time);
