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

// 8250 family uart identification, for the console and the legacy device probe
enum pc_uart_type {
    PC_UART_NONE,
    PC_UART_8250,
    PC_UART_16450,
    PC_UART_16550,
    PC_UART_16550A,
};
enum pc_uart_type pc_uart_probe(uint16_t port);
const char *pc_uart_type_name(enum pc_uart_type type);
// the console's port, which must not be probed again while it is in use
uint16_t pc_console_uart(enum pc_uart_type *type);

#if X86_LEGACY
// legacy (ISA) device discovery from the BIOS data area, CMOS and port probes
void pc_legacy_probe(void);
#endif

void platform_init_interrupts(void);
void platform_init_interrupts_postvm(void);
void platform_init_timer(void);

// legacy programmable interrupt controller
void pic_init(void);
void pic_enable(unsigned int vector, bool enable);
bool pic_is_spurious(unsigned int vector);
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
uint64_t pit_measure_freq(void);

// secondary cpus
void platform_start_secondary_cpus(void);

// pci (and the acpi namespace, which needs pci config access up first)
void pc_pci_init(bool have_acpi);
