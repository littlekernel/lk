/*
 * Copyright (c) 2009 Corey Tabaka
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include "platform_p.h"
#include <arch/x86.h>
#include <assert.h>
#include <inttypes.h>
#include <kernel/spinlock.h>
#include <kernel/thread.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/reg.h>
#include <lk/trace.h>
#include <platform.h>
#include <platform/interrupts.h>
#include <platform/pc.h>
#include <platform/pc/hpet.h>
#include <platform/pc/timer.h>
#include <platform/timer.h>
#include <platform/vga_console.h>
#include <sys/types.h>

#define LOCAL_TRACE 0

// TODO: switch this logic to lib/fixed_point math

static platform_timer_callback t_callback;
static void *callback_arg;
static spin_lock_t lock = SPIN_LOCK_INITIAL_VALUE;

static uint64_t ticks_per_ms;

// next callback event time in 32.32 fixed point milliseconds
static uint64_t next_trigger_time;

// if periodic, the delta to set to the next event. if oneshot, 0
static uint64_t next_trigger_delta;

// time in 32.32 fixed point milliseconds
static volatile uint64_t timer_current_time;
// delta time per periodic tick in 32.32
static uint64_t timer_delta_time;

#define INTERNAL_FREQ              1193182ULL
#define INTERNAL_FREQ_3X           3579546ULL
#define INTERNAL_FREQ_TICKS_PER_MS (INTERNAL_FREQ / 1000u)

/* Maximum amount of time that can be program on the timer to schedule the next
 *  interrupt, in milliseconds */
#define MAX_TIMER_INTERVAL 55

lk_time_t pit_current_time(void) {
    arch_interrupt_saved_state_t state = spin_lock_irqsave(&lock);

    lk_time_t time = (lk_time_t)(timer_current_time >> 32);

    spin_unlock_irqrestore(&lock, state);

    return time;
}

lk_bigtime_t pit_current_time_hires(void) {
    arch_interrupt_saved_state_t state = spin_lock_irqsave(&lock);

    lk_bigtime_t time = (lk_bigtime_t)((timer_current_time >> 22) * 1000) >> 10;

    spin_unlock_irqrestore(&lock, state);

    return time;
}

static enum handler_return pit_timer_tick(void *arg) {
    if (next_trigger_time != 0 || next_trigger_delta) {
        LTRACEF("ntt %#" PRIx64 ", ntd %#" PRIx64 "\n", next_trigger_time, next_trigger_delta);
    }

    spin_lock(&lock);
    timer_current_time += timer_delta_time;
    spin_unlock(&lock);

    lk_time_t time = current_time();

    if (t_callback && next_trigger_time != 0 && timer_current_time >= next_trigger_time) {
        if (next_trigger_delta != 0) {
            uint64_t delta = timer_current_time - next_trigger_time;
            next_trigger_time = timer_current_time + next_trigger_delta - delta;
        } else {
            next_trigger_time = 0;
        }

        return t_callback(callback_arg, time);
    } else {
        return INT_NO_RESCHEDULE;
    }
}

static void set_pit_frequency(uint32_t frequency) {
    uint32_t count, remainder;

    LTRACEF("frequency %u\n", frequency);

    /* figure out the correct divisor for the desired frequency */
    if (frequency <= 18) {
        count = 0xffff;
    } else if (frequency >= INTERNAL_FREQ) {
        count = 1;
    } else {
        count = INTERNAL_FREQ_3X / frequency;
        remainder = INTERNAL_FREQ_3X % frequency;

        if (remainder >= INTERNAL_FREQ_3X / 2) {
            count += 1;
        }

        count /= 3;
        remainder = count % 3;

        if (remainder >= 1) {
            count += 1;
        }
    }

    uint16_t divisor = count & 0xffff;

    /*
     * funky math that i don't feel like explaining. essentially 32.32 fixed
     * point representation of the configured timer delta.
     */
    timer_delta_time = (3685982306ULL * count) >> 10;

    LTRACEF("dt %#x.%08x\n", (uint32_t)(timer_delta_time >> 32),
            (uint32_t)(timer_delta_time & 0xffffffff));
    LTRACEF("divisor %" PRIu16 "\n", divisor);

    /*
     * setup the Programmable Interval Timer
     * timer 0, mode 2, binary counter, LSB followed by MSB
     */
    outp(I8253_CONTROL_REG, 0x34);
    outp(I8253_DATA_REG, divisor & 0xff); // LSB
    outp(I8253_DATA_REG, divisor >> 8);   // MSB
}

void pit_init(void) {
    // start the PIT at 1Khz in free-running mode to keep a time base
    timer_current_time = 0;
    ticks_per_ms = INTERNAL_FREQ / 1000;
    set_pit_frequency(1000); // ~1ms granularity
    register_int_handler(INT_PIT, &pit_timer_tick, NULL);
    unmask_interrupt(INT_PIT);
}

status_t pit_set_periodic_timer(platform_timer_callback callback, void *arg, lk_time_t interval) {
    LTRACEF("pit_set_periodic_timer: interval %u\n", interval);

    arch_interrupt_saved_state_t state = spin_lock_irqsave(&lock);

    t_callback = callback;
    callback_arg = arg;

    next_trigger_delta = (uint64_t)interval << 32;
    next_trigger_time = timer_current_time + next_trigger_delta;

    unmask_interrupt(INT_PIT);
    spin_unlock_irqrestore(&lock, state);

    return NO_ERROR;
}

status_t pit_set_oneshot_timer(platform_timer_callback callback, void *arg, lk_time_t interval) {
    LTRACEF("pit_set_oneshot_timer: interval %u\n", interval);

    arch_interrupt_saved_state_t state = spin_lock_irqsave(&lock);

    t_callback = callback;
    callback_arg = arg;

    next_trigger_delta = 0;
    next_trigger_time = timer_current_time + ((uint64_t)interval << 32);

    unmask_interrupt(INT_PIT);
    spin_unlock_irqrestore(&lock, state);

    return NO_ERROR;
}

void pit_cancel_timer(void) {
    LTRACE;

    arch_interrupt_saved_state_t state = spin_lock_irqsave(&lock);

    next_trigger_time = 0;

    spin_unlock_irqrestore(&lock, state);
}

void pit_stop_timer(void) {
    LTRACE;

    arch_interrupt_saved_state_t state = spin_lock_irqsave(&lock);

    next_trigger_time = 0;
    next_trigger_delta = 0;

    // stop the PIT
    outp(I8253_CONTROL_REG, 0x34);
    outp(I8253_DATA_REG, 0); // LSB
    outp(I8253_DATA_REG, 0); // MSB
    mask_interrupt(INT_PIT);

    spin_unlock_irqrestore(&lock, state);
}

// Measure what the PIT's counter is actually clocked at, in Hz.
//
// Everything else here takes INTERNAL_FREQ on faith and reports a TSC frequency derived
// from it, so a PIT ticking at the wrong rate comes back as a wrong TSC frequency with
// nothing to distinguish it from a correct one. This times a known countdown against the
// HPET instead, whose frequency is read from its capability register rather than measured,
// and so answers the question directly: it returns the PIT's rate, not a ratio between two
// guesses.
//
// Runs with interrupts disabled and takes as long as the countdown does -- ~55ms if the
// counter is ticking at its nominal rate, proportionally longer if it isn't -- so this is
// for the caltest command, not for anything on the boot path.
uint64_t pit_measure_freq(void) {
    DEBUG_ASSERT(arch_ints_disabled());

    const uint64_t hpet_hz = hpet_get_freq();
    if (hpet_hz == 0) {
        return 0;
    }

    // Full 16 bit countdown for the longest window we can get from one shot, which puts
    // the poll granularity somewhere around 10ppm of the result.
    const uint16_t count = 0xffff;

    // Mode 0, binary, lsb then msb. The countdown starts on the msb write.
    outp(I8253_CONTROL_REG, 0x30);
    outp(I8253_DATA_REG, count & 0xff);
    outp(I8253_DATA_REG, count >> 8);

    const uint64_t hpet_start = hpet_read_counter();

    // A machine whose PIT never reaches terminal count would otherwise wedge us here with
    // interrupts off, which is worth guarding against given the whole point of this routine
    // is to be pointed at a PIT that is behaving oddly.
    const uint64_t timeout_ticks = hpet_hz * 2;

    uint64_t elapsed;
    for (;;) {
        // Read back the status of ch0 and look for bit 7 (output) high, bit 6 (null
        // count) low.
        outp(I8253_CONTROL_REG, 0xe2);
        const uint8_t status = inp(I8253_DATA_REG);

        elapsed = hpet_counter_delta(hpet_start, hpet_read_counter());

        if ((status & 0xc0) == 0x80) {
            break;
        }

        if (elapsed > timeout_ticks) {
            dprintf(INFO, "PIT: counter never reached terminal count, giving up\n");
            set_pit_frequency(1000);
            return 0;
        }
    }

    if (elapsed == 0) {
        return 0;
    }

    // count ticks took elapsed/hpet_hz seconds. count is 16 bits and hpet_hz is tens of
    // MHz, so the product is nowhere near overflowing.
    const uint64_t freq = ((uint64_t)count * hpet_hz) / elapsed;

    // nominal/measured, in thousandths. That ratio is exactly the factor by which every
    // PIT derived measurement is scaled, so it is the number worth printing: 1.000x means
    // the counter is ticking where it should.
    const uint64_t ratio_milli = (INTERNAL_FREQ * 1000) / freq;
    dprintf(INFO,
            "PIT: input clock measured %" PRIu64 "Hz over %" PRIu64 " hpet ticks (nominal %" PRIu64
            "Hz, ratio %" PRIu64 ".%03" PRIu64 "x)\n",
            freq, elapsed, (uint64_t)INTERNAL_FREQ, ratio_milli / 1000, ratio_milli % 1000);

    // put the PIT back to 1ms countdown
    set_pit_frequency(1000);

    return freq;
}

uint64_t pit_calibrate_tsc(void) {
    DEBUG_ASSERT(arch_ints_disabled());

    uint64_t tsc_ticks[5] = { 0 };
    uint32_t countdown_ms[5] = { 0 };

    uint64_t tsc_freq = 0;
    for (uint i = 0; i < countof(tsc_ticks); i++) {
        // calibrate the tsc frequency using the PIT
        countdown_ms[i] = 2 * (i + 1);

        uint16_t pic_ticks = INTERNAL_FREQ_TICKS_PER_MS * countdown_ms[i];
        outp(I8253_CONTROL_REG, 0x30);
        outp(I8253_DATA_REG, pic_ticks & 0xff); // LSB
        outp(I8253_DATA_REG, pic_ticks >> 8);   // MSB

        // read the tsc
        uint64_t tsc_start = __builtin_ia32_rdtsc();

        // wait for countdown_ms
        uint8_t status = 0;
        do {
            // Send a read-back command that latches the status of ch0
            outp(I8253_CONTROL_REG, 0xe2);
            status = inp(I8253_DATA_REG);
            // Wait for bit 7 (output) to go high and for bit 6 (null count) to go low
        } while ((status & 0xc0) != 0x80);

        uint64_t tsc_end = __builtin_ia32_rdtsc();
        tsc_ticks[i] = tsc_end - tsc_start;
    }

    // Each sample carries a roughly fixed error: the window starts before the tsc is
    // sampled (the countdown is already running once the last port write lands), and it
    // ends after the countdown expires (the readback poll only notices on its next pass).
    // Both are close to constant per sample, so they shrink as a fraction of the window as
    // the window grows -- which makes the longest window the most accurate sample and the
    // raw-tick comparison this used to do, which always resolved to the *shortest* window,
    // the least accurate one.
    uint best_index = 0;
    for (uint i = 0; i < countof(tsc_ticks); i++) {
        const uint64_t implied = (tsc_ticks[i] * 1000) / countdown_ms[i];

        // Print every sample. The trend across them says what any error is: a frequency
        // that holds steady as the window grows means the PIT is ticking at the wrong rate,
        // one that drifts means a fixed per-sample cost is being amortized away.
        dprintf(SPEW, "PIT: sample %u: %ums window, %" PRIu64 " tsc ticks, implies %" PRIu64 "Hz\n",
                i, countdown_ms[i], tsc_ticks[i], implied);

        if (countdown_ms[i] > countdown_ms[best_index]) {
            best_index = i;
        }
    }

    tsc_freq = (tsc_ticks[best_index] * 1000) / countdown_ms[best_index];
    dprintf(INFO, "PIT: calibrated TSC frequency: %" PRIu64 "Hz\n", tsc_freq);

    // put the PIT back to 1ms countdown
    set_pit_frequency(1000);

    return tsc_freq;
}

uint32_t pit_calibrate_lapic(uint32_t (*lapic_read_tick)(void)) {
    DEBUG_ASSERT(arch_ints_disabled());

    uint64_t lapic_ticks[5] = { 0 };
    uint32_t countdown_ms[5] = { 0 };

    for (uint i = 0; i < countof(lapic_ticks); i++) {
        // calibrate the tsc frequency using the PIT
        countdown_ms[i] = 2 * (i + 1);

        uint16_t pic_ticks = INTERNAL_FREQ_TICKS_PER_MS * countdown_ms[i];
        outp(I8253_CONTROL_REG, 0x30);
        outp(I8253_DATA_REG, pic_ticks & 0xff); // LSB
        outp(I8253_DATA_REG, pic_ticks >> 8);   // MSB

        // read the tsc
        uint32_t tick_start = lapic_read_tick();

        // wait for countdown_ms
        uint8_t status = 0;
        do {
            // Send a read-back command that latches the status of ch0
            outp(I8253_CONTROL_REG, 0xe2);
            status = inp(I8253_DATA_REG);
            // Wait for bit 7 (output) to go high and for bit 6 (null count) to go low
        } while ((status & 0xc0) != 0x80);

        uint32_t tick_end = lapic_read_tick();
        lapic_ticks[i] = tick_start - tick_end;
    }

    // Same selection as pit_calibrate_tsc(): the longest window amortizes the fixed
    // per-sample error best, so take it rather than comparing raw tick counts.
    uint best_index = 0;
    for (uint i = 0; i < countof(lapic_ticks); i++) {
        const uint32_t implied = (lapic_ticks[i] * 1000) / countdown_ms[i];

        dprintf(SPEW,
                "PIT: sample %u: %ums window, %" PRIu64 " lapic ticks, implies %" PRIu32 "Hz\n", i,
                countdown_ms[i], lapic_ticks[i], implied);

        if (countdown_ms[i] > countdown_ms[best_index]) {
            best_index = i;
        }
    }

    uint32_t freq = (lapic_ticks[best_index] * 1000) / countdown_ms[best_index];
    dprintf(INFO, "PIT: calibrated local apic frequency: %" PRIu32 "Hz\n", freq);

    // put the PIT back to 1ms countdown
    set_pit_frequency(1000);

    return freq;
}