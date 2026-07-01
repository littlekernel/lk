//
// Copyright (c) 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
//
#include <sys/types.h>
#include <lk/trace.h>

#include "platform_p.h"

#define LOCAL_TRACE 1

// sun4m timer

// each cpu offset is 0x1000, only handle UP for now
#define TIMER_PERCPU_PHYS (0xff1300000)

enum {
    TIMER_LIMIT_REG = 0,
    TIMER_COUNTER_REG = 4,
    TIMER_LIMIT_NO_RESET_REG = 8,
    TIMER_USER_START_STOP_REG = 0xc,
};

void sun4m_timer_early_init(void) {
    // stop the timer
    write_control_space_32(TIMER_PERCPU_PHYS, TIMER_LIMIT_REG, 0);
}

void sun4m_timer_init(void) {
    // nothing to do here
}
