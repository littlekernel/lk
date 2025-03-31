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

// legacy programmable interrupt controller
void pic_init(void);
void pic_enable(unsigned int vector, bool enable);
void pic_eoi(unsigned int vector);
void pic_mask_interrupts(void);
uint64_t pit_calibrate_tsc(void);

// local apic
void lapic_init(void);
void lapic_eoi(unsigned int vector);
void lapic_send_init_ipi(uint32_t apic_id, bool level);
void lapic_send_startup_ipi(uint32_t apic_id, uint32_t startup_vector);
void lapic_send_ipi(uint32_t apic_id, uint32_t vector);

// programable interval timer
void pit_init(void);
status_t pit_set_periodic_timer(platform_timer_callback callback, void *arg, lk_time_t interval);
status_t pit_set_oneshot_timer(platform_timer_callback callback, void *arg, lk_time_t interval);
void pit_stop_timer(void);
lk_time_t pit_current_time(void);
lk_bigtime_t pit_current_time_hires(void);

// secondary cpus
void platform_start_secondary_cpus(void);
