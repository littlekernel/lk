/*
 * Copyright (c) 2025 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#pragma once

#include <platform/timer.h>
#include <stdbool.h>
#include <sys/types.h>
#include <kernel/mp.h>

// local apic
void lapic_init(void);
uint32_t lapic_get_id_from_hardware(void);
status_t lapic_timer_init(bool invariant_tsc_supported);
void lapic_eoi(unsigned int vector);
void lapic_send_init_ipi(uint32_t apic_id, bool level);
void lapic_send_startup_ipi(uint32_t apic_id, uint32_t startup_vector);
void lapic_send_ipi(uint32_t apic_id, mp_ipi_t ipi);

status_t lapic_set_oneshot_timer(platform_timer_callback callback, void *arg, lk_time_t interval);
void lapic_cancel_timer(void);

