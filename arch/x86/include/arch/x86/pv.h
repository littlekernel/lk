/*
 * Copyright (c) 2025 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <sys/types.h>
#include <stdbool.h>

#if !X86_LEGACY

status_t pvclock_init(void);
uint64_t pvclock_get_tsc_freq(void);
bool pv_clock_is_stable(void);

#endif // !X86_LEGACY
