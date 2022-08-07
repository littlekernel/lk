/*
 * Copyright (c) 2021 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include "platform_p.h"

#include <assert.h>
#include <inttypes.h>
#include <lk/err.h>
#include <lk/debug.h>
#include <lk/reg.h>
#include <lk/trace.h>
#include <kernel/debug.h>
#include <kernel/thread.h>
#include <platform/interrupts.h>
#include <platform/virt.h>
#include <platform/timer.h>
#include <platform.h>

#define LOCAL_TRACE 0

// implementation of RTC at
// https://github.com/qemu/qemu/blob/master/hw/rtc/goldfish_rtc.c
volatile uint32_t * const goldfish_rtc_base = (void *)VIRT_GF_RTC_MMIO_BASE;

// registers
enum {
    RTC_TIME_LOW        = 0x00,
    RTC_TIME_HIGH       = 0x04,
    RTC_ALARM_LOW       = 0x08,
    RTC_ALARM_HIGH      = 0x0c,
    RTC_IRQ_ENABLED     = 0x10,
    RTC_CLEAR_ALARM     = 0x14,
    RTC_ALARM_STATUS    = 0x18,
    RTC_CLEAR_INTERRUPT = 0x1c,
};

static uint64_t system_boot_offset;

static platform_timer_callback t_callback;
static void *t_arg;

static void write_reg(unsigned int reg, uint32_t val) {
    goldfish_rtc_base[reg / 4] = val;
}

static uint32_t read_reg(unsigned int reg) {
    return goldfish_rtc_base[reg / 4];
}

// raw time from the RTC is ns wall time
static uint64_t read_raw_time(void) {
    uint32_t low, high;

    // read both registers and assemble a 64bit counter
    // reading low first latches a shadow high register which will prevent wraparound
    low = read_reg(RTC_TIME_LOW);
    high = read_reg(RTC_TIME_HIGH);

    return ((uint64_t)high << 32) | low;
}

enum handler_return rtc_irq(void *unused) {
    enum handler_return ret = INT_NO_RESCHEDULE;

    write_reg(RTC_CLEAR_ALARM, 1);
    write_reg(RTC_CLEAR_INTERRUPT, 1);

    if (t_callback) {
        ret = t_callback(t_arg, current_time());
    }

    return ret;
}

void goldfish_rtc_early_init(void) {
    // sample the timer and use it as a offset for system start
    system_boot_offset = read_raw_time();

    // clear and stop any pending irqs on the timer
    platform_stop_timer();

    register_int_handler(VIRT_GF_RTC_IRQ_BASE, &rtc_irq, NULL);
    unmask_interrupt(VIRT_GF_RTC_IRQ_BASE);

    // its okay to enable the irq since we've cleared the alarm and any pending interrupts
    write_reg(RTC_IRQ_ENABLED, 1);
}

void goldfish_rtc_init(void) {
}

lk_bigtime_t current_time_hires(void) {
    uint64_t t = read_raw_time() - system_boot_offset;

    return t / 1000ULL; // ns -> us
}

lk_time_t current_time(void) {
    uint64_t t = read_raw_time() - system_boot_offset;

    return (lk_time_t)(t / 1000000ULL); // ns -> ms
}

status_t platform_set_oneshot_timer (platform_timer_callback callback, void *arg, lk_time_t interval) {
    LTRACEF("callback %p, arg %p, interval %u\n", callback, arg, interval);

    t_callback = callback;
    t_arg = arg;

    uint64_t delta = read_raw_time();
    delta += interval * 1000000ULL;

    write_reg(RTC_ALARM_HIGH, delta >> 32);
    write_reg(RTC_ALARM_LOW, delta & 0xffffffff);

    return NO_ERROR;
}

void platform_stop_timer(void) {
    LTRACE;

    write_reg(RTC_CLEAR_ALARM, 1);
    write_reg(RTC_CLEAR_INTERRUPT, 1);
}


