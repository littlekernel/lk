/*
 * Copyright (c) 2025 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <sys/types.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/init.h>
#include <lk/reg.h>
#include <lk/trace.h>
#include <kernel/thread.h>
#include <kernel/vm.h>
#include <platform.h>
#include <platform/timer.h>
#include <platform/pc.h>
#include <platform/pc/timer.h>
#include <arch/x86.h>
#include <arch/x86/feature.h>
#include <arch/x86/apic.h>
#include <arch/x86/pv.h>
#include <inttypes.h>
#include <lib/fixed_point.h>

#include "platform_p.h"

#define LOCAL_TRACE 0

// Deals with all of the various clock sources and event timers on the PC platform.
// TODO:
//   HPET
//   cpuid leaves that describe clock rates

static enum clock_source {
    CLOCK_SOURCE_INITIAL,
    CLOCK_SOURCE_PIT,
    CLOCK_SOURCE_TSC,
    CLOCK_SOURCE_HPET,
} clock_source = CLOCK_SOURCE_INITIAL;

static struct fp_32_64 tsc_to_timebase;
static struct fp_32_64 tsc_to_timebase_hires;
static struct fp_32_64 timebase_to_tsc;
static bool use_lapic_timer = false;

static const char *clock_source_name(void) {
    switch (clock_source) {
        case CLOCK_SOURCE_INITIAL:
            return "initial";
        case CLOCK_SOURCE_PIT:
            return "PIT";
        case CLOCK_SOURCE_TSC:
            return "TSC";
        case CLOCK_SOURCE_HPET:
            return "HPET";
        default:
            return "unknown";
    }
}

lk_time_t current_time(void) {
    switch (clock_source) {
        case CLOCK_SOURCE_PIT:
            return pit_current_time();
        case CLOCK_SOURCE_TSC:
            return u32_mul_u64_fp32_64(__builtin_ia32_rdtsc(), tsc_to_timebase);
        default:
            return 0;
    }
}

lk_bigtime_t current_time_hires(void) {
    switch (clock_source) {
        case CLOCK_SOURCE_PIT:
            return pit_current_time_hires();
        case CLOCK_SOURCE_TSC:
            return u64_mul_u64_fp32_64(__builtin_ia32_rdtsc(), tsc_to_timebase_hires);
        default:
            return 0;
    }
}

// Convert lk_time_t to TSC ticks
uint64_t time_to_tsc_ticks(lk_time_t time) {
    return u64_mul_u32_fp32_64(time, timebase_to_tsc);
}

void platform_init_timer(void) {
    // Initialize the PIT, it's always present in PC hardware
    pit_init();
    clock_source = CLOCK_SOURCE_PIT;

#if !X86_LEGACY
    // XXX update note about what invariant TSC means
    bool use_invariant_tsc = x86_feature_test(X86_FEATURE_INVAR_TSC);
    LTRACEF("invariant TSC %d\n", use_invariant_tsc);

    // Test for hypervisor PV clock, which also effectively says if TSC is invariant across
    // all cpus.
    if (pvclock_init() == NO_ERROR) {
        bool pv_clock_stable = pv_clock_is_stable();

        use_invariant_tsc |= pv_clock_stable;

        printf("pv_clock: Clocksource is %sstable\n", (pv_clock_stable ? "" : "not "));
    }

    // XXX test for HPET and use it over PIT if present

    if (use_invariant_tsc) {
        // We're going to try to use the TSC as a time base, obtain the TSC frequency.
        uint64_t tsc_hz = 0;

        tsc_hz = pvclock_get_tsc_freq();
        if (tsc_hz == 0) {
            // TODO: some x86 cores describe the TSC and lapic clocks in cpuid

            // Calibrate the TSC against the PIT, which should always be present
            tsc_hz = pit_calibrate_tsc();
            if (tsc_hz == 0) {
                dprintf(CRITICAL, "PC: failed to calibrate TSC frequency\n");
                goto out;
            }
        }

        dprintf(INFO, "PC: TSC frequency %" PRIu64 "Hz\n", tsc_hz);

        // Compute the ratio of TSC to timebase
        fp_32_64_div_32_32(&tsc_to_timebase, 1000, tsc_hz);
        dprintf(INFO, "PC: TSC to timebase ratio %u.%08u...\n",
                tsc_to_timebase.l0, tsc_to_timebase.l32);

        fp_32_64_div_32_32(&tsc_to_timebase_hires, 1000*1000, tsc_hz);
        dprintf(INFO, "PC: TSC to hires timebase ratio %u.%08u...\n",
                tsc_to_timebase_hires.l0, tsc_to_timebase_hires.l32);

        fp_32_64_div_32_32(&timebase_to_tsc, tsc_hz, 1000);
        dprintf(INFO, "PC: timebase to TSC ratio %u.%08u...\n",
                timebase_to_tsc.l0, timebase_to_tsc.l32);

        clock_source = CLOCK_SOURCE_TSC;
    }
out:

    // Set up the local apic for event timer interrupts
    if (lapic_timer_init(use_invariant_tsc) == NO_ERROR) {
        dprintf(INFO, "PC: using LAPIC timer for event timer\n");
        use_lapic_timer = true;
    }

    // If we're not using the PIT for time base and using the LAPIC timer for events, stop the PIT.
    if (use_lapic_timer && clock_source != CLOCK_SOURCE_PIT) {
        pit_stop_timer();
    }

#endif // !X86_LEGACY

    dprintf(INFO, "PC: using %s clock source\n", clock_source_name());
}

status_t platform_set_periodic_timer(platform_timer_callback callback, void *arg, lk_time_t interval) {
    if (use_lapic_timer) {
        PANIC_UNIMPLEMENTED;
    }
    return pit_set_periodic_timer(callback, arg, interval);
}

status_t platform_set_oneshot_timer(platform_timer_callback callback,
                                    void *arg, lk_time_t interval) {
    if (use_lapic_timer) {
        return lapic_set_oneshot_timer(callback, arg, interval);
    }
    return pit_set_oneshot_timer(callback, arg, interval);
}

void platform_stop_timer(void) {
    if (use_lapic_timer) {
        lapic_cancel_timer();
    } else {
        pit_cancel_timer();
    }
}
