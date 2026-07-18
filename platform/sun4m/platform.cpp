//
// Copyright (c) 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
//
#include <kernel/thread.h>
#include <lk/err.h>
#include <lk/reg.h>
#include <platform.h>
#include <platform/debug.h>
#include <platform/interrupts.h>
#include <platform/timer.h>
#include <sys/types.h>

#include "platform_p.h"

void platform_early_init(void) {
    sun4m_intc_early_init();
    sun4m_scc_early_init();
    sun4m_timer_early_init();
}

void platform_init(void) {
    sun4m_intc_init();
    sun4m_timer_init();
    sun4m_scc_init();
}
