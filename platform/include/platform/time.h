/*
 * Copyright (c) 2008 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lk/compiler.h>
#include <sys/types.h>

/* Routines to read the current system time since an arbitrary point in the
 * past. Usually system boot time.
 */

__BEGIN_CDECLS

/* Time in units of milliseconds */
lk_time_t current_time(void);

/* Time in units of microseconds */
lk_bigtime_t current_time_hires(void);

__END_CDECLS

