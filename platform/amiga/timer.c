/*
 * Copyright (c) 2025-2026 Josh Cummings
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include "platform_p.h"

#include <kernel/spinlock.h>
#include <kernel/timer.h>
#include <lk/err.h>
#include <platform.h>
#include <platform/interrupts.h>
#include <platform/timer.h>
#include <sys/types.h>

static volatile uint8_t *const cia_base = (volatile uint8_t *)CIA_B_BASE;

uint32_t eclockfreq;
uint32_t ciafreq;
uint32_t eclock_hz;
uint32_t cia_clock_interval;

static uint16_t ta_last;
static uint64_t ta_ticks;

static void *callback_arg;
static platform_timer_callback t_callback;
static spin_lock_t lock = SPIN_LOCK_INITIAL_VALUE;

static uint8_t read_reg(unsigned int reg) {
    return cia_base[reg];
}

static void write_reg(unsigned int reg, uint8_t val) {
    cia_base[reg] = val;
}

static uint16_t get_ta_tick(void) {
    uint8_t ta_lo = read_reg(CIA_B_TALO);
    uint8_t ta_hi1 = read_reg(CIA_B_TAHI);
    uint8_t ta_hi2 = read_reg(CIA_B_TAHI);

    if (ta_hi1 != ta_hi2) {
        ta_lo = read_reg(CIA_B_TALO);
        ta_hi1 = ta_hi2;
    }

    return (((uint16_t)ta_hi1 << 8) | ta_lo);
}

static uint32_t calculate_eclock(void) {
    const uint32_t tod_ticks = 128;
    uint32_t seen_ticks = 0;
    uint64_t tick_accum = 0;
    uint64_t hz;

    // Configure and start CIA-B Timer A in free-running, continuous mode
    write_reg(CIA_B_CRA, 0x00); // STOP TA
    write_reg(CIA_B_TAHI, 0xFF);
    write_reg(CIA_B_TALO, 0xFF);

    // Make sure TA interrupt is disabled: write ICR without bit7 to clear enable
    // TODO: mask_interrupt?
    write_reg(CIA_B_ICR, 0x01);     // clear-enable TA (bit0)
    write_reg(CIA_B_CRA, (1 << 0)); // START=1, RUNMODE=0 (periodic/continuous)

    uint16_t last_ta_tick = get_ta_tick();
    uint8_t last_tod_tick = read_reg(CIA_B_TODMID);

    while (seen_ticks < tod_ticks) {
        uint16_t now_ta = get_ta_tick();
        tick_accum += (last_ta_tick - now_ta);
        last_ta_tick = now_ta;

        uint8_t tod = read_reg(CIA_B_TODMID); // & 0x0F;

        if (tod != last_tod_tick) {
            seen_ticks++;
            last_tod_tick = tod;
        }
    }

    uint64_t palfreq = (tick_accum * 50) / (uint64_t)tod_ticks;
    uint64_t ntscfreq = (tick_accum * 60) / (uint64_t)tod_ticks;

    const uint64_t lo = 650000, hi = 780000;
    const uint64_t pal_nom = 709379, ntsc_nom = 715909;

    bool in50 = (palfreq >= lo && palfreq <= hi);
    bool in60 = (ntscfreq >= lo && ntscfreq <= hi);

    if (in50 && in60) {
        uint64_t d50 =
            (palfreq > pal_nom) ? (palfreq - pal_nom) : (pal_nom - palfreq);
        uint64_t d60 =
            (ntscfreq > ntsc_nom) ? (ntscfreq - ntsc_nom) : (ntsc_nom - ntscfreq);
        hz = (d50 <= d60) ? palfreq : ntscfreq;
    } else if (in50) {
        hz = palfreq;
    } else if (in60) {
        hz = ntscfreq;
    } else {
        // Fallback if TOD was paused or we sampled during reset
        hz = pal_nom;
    }

    return (uint32_t)hz;
}

lk_bigtime_t current_time_hires(void) {
    uint16_t now = get_ta_tick();
    uint16_t delta = (uint16_t)(ta_last - now);

    ta_last = now;
    ta_ticks += delta;

    return (lk_bigtime_t)((ta_ticks * 1000000ull) / eclock_hz);
}

lk_time_t current_time(void) {
    return (lk_time_t)(current_time_hires() / 1000);
}

void platform_stop_timer(void) {
    write_reg(CIA_B_CRB, 0x00);
    mask_interrupt(INTERRUPT_TIMERB_B);
}

static inline uint16_t ms_to_ticks(lk_time_t ms) {
    uint64_t ticks = (uint64_t)ms * eclock_hz + 500; // round
    ticks /= 1000;
    if (ticks == 0) {
        ticks = 1;
    }
    if (ticks > 65535) {
        ticks = 65535; // single-shot ceiling
    }
    return (uint16_t)ticks;
}

static enum handler_return cia_timer_irq(void *arg) {
    cia_base[CIA_B_ICR]; // Clear ICR. TODO: Remove when clear_interrupt() has CIA support

    if (t_callback) {
        return t_callback(callback_arg, current_time());
    }

    return INT_NO_RESCHEDULE;
}

void cia_timer_init(void) {
    eclock_hz = calculate_eclock();

    mask_interrupt(INTERRUPT_TIMERA_B);

    // Continuous, free-running mode for Timer A
    write_reg(CIA_B_TALO, 0xFF);
    write_reg(CIA_B_TAHI, 0xFF);
    write_reg(CIA_B_CRA, 0x01); // Start Timer A, RUNMODE = continuous

    ta_last = get_ta_tick();
    ta_ticks = 0;

    platform_stop_timer();

    register_int_handler(INTERRUPT_TIMERB_B, cia_timer_irq, NULL);
}

// TODO: Can some of oneshot and periodic's stuff be centralised and called by
// each? Lots of similarities
status_t platform_set_oneshot_timer(platform_timer_callback callback, void *arg,
                                    lk_time_t interval) {
    arch_interrupt_saved_state_t state = spin_lock_irqsave(&lock);

    t_callback = callback;
    callback_arg = arg;

    uint16_t ticks = ms_to_ticks(interval);
    platform_stop_timer();

    write_reg(CIA_B_TBLO, (uint8_t)(ticks & 0xFF));
    write_reg(CIA_B_TBHI, (uint8_t)(ticks >> 8));

    // Enable Timer B CIA interrupt
    write_reg(CIA_B_ICR, 0x80 | 0x02);

    unmask_interrupt(INTERRUPT_TIMERB_B);

    // Start Timer B in one-shot mode. START | RUNMODE | LOAD
    write_reg(CIA_B_CRB, (0 << 5) | (1 << 4) | (1 << 3) | (1 << 0));

    spin_unlock_irqrestore(&lock, state);

    return NO_ERROR;
}

int platform_set_periodic_timer(platform_timer_callback callback, void *arg,
                                lk_time_t interval) {
    arch_interrupt_saved_state_t state = spin_lock_irqsave(&lock);

    t_callback = callback;
    callback_arg = arg;

    uint16_t ticks = ms_to_ticks(interval);

    write_reg(CIA_B_TBLO, (uint8_t)(ticks & 0xFF));
    write_reg(CIA_B_TBHI, (uint8_t)(ticks >> 8));

    spin_unlock_irqrestore(&lock, state);

    return NO_ERROR;
}
