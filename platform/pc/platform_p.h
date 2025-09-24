/*
 * Copyright (c) 2009 Corey Tabaka
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lib/cbuf.h>
#include <platform/timer.h>

extern cbuf_t console_input_buf;

void platform_init_debug_early(void);
void platform_init_debug(void);
void platform_init_interrupts(void);
void platform_init_interrupts_postvm(void);
void platform_init_timer(void);

// legacy programmable interrupt controller
void pic_init(void);
void pic_enable(unsigned int vector, bool enable);
void pic_eoi(unsigned int vector);
void pic_mask_interrupts(void);

// programable interval timer
void pit_init(void);
status_t pit_set_periodic_timer(platform_timer_callback callback, void *arg, lk_time_t interval);
status_t pit_set_oneshot_timer(platform_timer_callback callback, void *arg, lk_time_t interval);
void pit_cancel_timer(void);
void pit_stop_timer(void);
lk_time_t pit_current_time(void);
lk_bigtime_t pit_current_time_hires(void);
uint64_t pit_calibrate_tsc(void);

// secondary cpus
void platform_start_secondary_cpus(void);
