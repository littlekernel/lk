/*
 * Copyright (c) 2025 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <arch/x86.h>
#include <arch/interrupts.h>
#include <arch/x86/apic.h>
#include <arch/x86/feature.h>
#include <arch/x86/pv.h>
#include <inttypes.h>
#include <lk/console_cmd.h>
#include <kernel/thread.h>
#include <kernel/vm.h>
#include <lib/fixed_point.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/init.h>
#include <lk/reg.h>
#include <lk/trace.h>
#include <platform.h>
#include <platform/pc.h>
#include <platform/pc/hpet.h>
#include <platform/pc/timer.h>
#include <platform/timer.h>
#include <sys/types.h>

#include "platform_p.h"

#define LOCAL_TRACE 0

// Deals with all of the various clock sources and event timers on the PC platform.
// TODO:
//   cpuid leaves that describe clock rates

static enum clock_source {
    CLOCK_SOURCE_INITIAL,
    CLOCK_SOURCE_PIT,
    CLOCK_SOURCE_TSC,
    CLOCK_SOURCE_HPET,
} clock_source = CLOCK_SOURCE_INITIAL;

// Bounds on a believable TSC frequency, used to sanity check whatever the calibration
// sources come back with. An invariant TSC only exists on parts that run at hundreds of
// MHz or better, and nothing ships with a TSC anywhere near 10GHz.
#define MIN_PLAUSIBLE_TSC_HZ (100ULL * 1000 * 1000)
#define MAX_PLAUSIBLE_TSC_HZ (10ULL * 1000 * 1000 * 1000)

static struct fp_32_64 tsc_to_timebase;
static struct fp_32_64 tsc_to_timebase_hires;
static struct fp_32_64 timebase_to_tsc;
static bool use_lapic_timer = false;

#if !X86_LEGACY
// The TSC frequency the timebase conversions above were built from, kept for the caltest
// command to compare fresh measurements against. Zero if the TSC is not the time base.
static uint64_t tsc_hz_in_use;
#endif

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
        case CLOCK_SOURCE_HPET:
            return hpet_current_time();
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
        case CLOCK_SOURCE_HPET:
            return hpet_current_time_hires();
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
    // An invariant TSC ticks at a fixed rate regardless of CPU frequency scaling,
    // throttling, or C-state changes, and stays in sync across all cores/packages.
    // That makes it usable as a cheap, stable time base; without it the TSC drifts
    // relative to wall-clock time as the CPU's performance state changes, so we have
    // to fall back to another clock source instead.
    bool use_invariant_tsc = x86_feature_test(X86_FEATURE_INVAR_TSC);
    LTRACEF("invariant TSC %d\n", use_invariant_tsc);

    // Test for hypervisor PV clock, which also effectively says if TSC is invariant across
    // all cpus.
    if (pvclock_init() == NO_ERROR) {
        bool pv_clock_stable = pv_clock_is_stable();

        use_invariant_tsc |= pv_clock_stable;

        printf("pv_clock: Clocksource is %sstable\n", (pv_clock_stable ? "" : "not "));
    }

    // Look for an HPET up front rather than only on the fallback path: its free-running
    // counter is a much better reference to calibrate the TSC against than the PIT, and
    // it can stand in as the clock source if the TSC turns out to be unusable.
    const bool have_hpet = (hpet_init() == NO_ERROR);

    if (use_invariant_tsc) {
        // We're going to try to use the TSC as a time base, obtain the TSC frequency.
        uint64_t tsc_hz = 0;

        // Best to worst: a hypervisor that just tells us, then a measurement against the
        // HPET's free-running counter, then a measurement against the PIT.
        // TODO: some x86 cores describe the TSC and lapic clocks in cpuid (leaf 0x15/0x16),
        // which is exact and should be preferred over measuring anything at all.
        tsc_hz = pvclock_get_tsc_freq();
        if (tsc_hz == 0 && have_hpet) {
            tsc_hz = hpet_calibrate_tsc();
        }
        if (tsc_hz == 0) {
            tsc_hz = pit_calibrate_tsc();
        }

        // Every source above can lie -- an emulated or mis-clocked PIT most of all -- and
        // a bad frequency here is invisible from inside the kernel, since it scales the
        // conversion in both directions and stays self-consistent while every timer in the
        // system runs at the wrong rate against the wall clock. Reject a result that can't
        // be true rather than quietly building the whole timebase on it.
        if (tsc_hz < MIN_PLAUSIBLE_TSC_HZ || tsc_hz > MAX_PLAUSIBLE_TSC_HZ) {
            dprintf(CRITICAL, "PC: implausible TSC frequency %" PRIu64 "Hz, not using the TSC\n",
                    tsc_hz);
            // Fall back to another clock source, and keep the LAPIC timer off TSC deadline
            // mode, which would otherwise convert its deadlines with a ratio we never set.
            use_invariant_tsc = false;
        } else {
            dprintf(INFO, "PC: TSC frequency %" PRIu64 "Hz\n", tsc_hz);

            // Compute the ratio of TSC to timebase
            fp_32_64_div_32_64(&tsc_to_timebase, 1000, tsc_hz);
            fp_32_64_div_32_64(&tsc_to_timebase_hires, 1000 * 1000, tsc_hz);
            fp_32_64_div_64_32(&timebase_to_tsc, tsc_hz, 1000);

            char ratio_buf[32];
            dprintf(SPEW, "PC: TSC to timebase ratio %s\n",
                    fp_32_64_snprintf(ratio_buf, sizeof(ratio_buf), &tsc_to_timebase, 9));
            dprintf(SPEW, "PC: TSC to hires timebase ratio %s\n",
                    fp_32_64_snprintf(ratio_buf, sizeof(ratio_buf), &tsc_to_timebase_hires, 9));
            dprintf(SPEW, "PC: timebase to TSC ratio %s\n",
                    fp_32_64_snprintf(ratio_buf, sizeof(ratio_buf), &timebase_to_tsc, 9));

            clock_source = CLOCK_SOURCE_TSC;
            tsc_hz_in_use = tsc_hz;
        }
    }

    if (clock_source != CLOCK_SOURCE_TSC && have_hpet) {
        // No usable TSC to use as a time base, prefer the HPET's free-running
        // counter over the interrupt-driven PIT.
        clock_source = CLOCK_SOURCE_HPET;
    }

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

status_t platform_set_periodic_timer(platform_timer_callback callback, void *arg,
                                     lk_time_t interval) {
    if (use_lapic_timer) {
        PANIC_UNIMPLEMENTED;
    }
    return pit_set_periodic_timer(callback, arg, interval);
}

status_t platform_set_oneshot_timer(platform_timer_callback callback, void *arg,
                                    lk_time_t interval) {
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

#if !X86_LEGACY

// Print a measured TSC frequency next to how far it lands from the one the system is
// actually running on, which is the number that matters when comparing sources.
static void caltest_print_result(const char *name, uint64_t hz) {
    if (hz == 0) {
        printf("%-8s unavailable\n", name);
        return;
    }

    if (tsc_hz_in_use == 0) {
        printf("%-8s %" PRIu64 "Hz\n", name, hz);
        return;
    }

    // Signed ppm difference from the frequency in use. The numerator peaks around 1e16
    // for any believable pair of frequencies, so this stays inside 64 bits.
    const int64_t ppm =
        ((int64_t)hz - (int64_t)tsc_hz_in_use) * 1000000 / (int64_t)tsc_hz_in_use;

    printf("%-8s %" PRIu64 "Hz (%+" PRId64 " ppm vs in use)\n", name, hz, ppm);
}

// Re-run every TSC calibration source available on this machine and print them side by
// side. Useful for deciding whether a given source can be trusted on a given box, since
// the sources are independent of each other and of whatever ran at boot.
static int cmd_caltest(int argc, const console_cmd_args *argv) {
    printf("clock source: %s\n", clock_source_name());
    if (tsc_hz_in_use != 0) {
        printf("TSC frequency in use: %" PRIu64 "Hz\n", tsc_hz_in_use);
    } else {
        printf("TSC frequency in use: none, the TSC is not the time base\n");
    }

    caltest_print_result("pvclock", pvclock_get_tsc_freq());

    caltest_print_result("HPET", hpet_is_available() ? hpet_calibrate_tsc() : 0);

    // Calibrating against the PIT reprograms it, which would wreck a timebase or an event
    // timer still running on it. Those are exactly the cases where platform_init_timer()
    // left the PIT running, so only touch it when it left the PIT stopped.
    if (!use_lapic_timer || clock_source == CLOCK_SOURCE_PIT) {
        printf("%-8s skipped, still in use for %s\n", "PIT",
               (clock_source == CLOCK_SOURCE_PIT) ? "timekeeping" : "event timers");
    } else {
        // pit_calibrate_tsc() requires interrupts off, and spends ~30ms measuring.
        const arch_interrupt_saved_state_t state = arch_interrupt_save();
        const uint64_t pit_hz = pit_calibrate_tsc();
        arch_interrupt_restore(state);

        // It leaves the PIT free running at 1KHz; put it back to stopped where we found it.
        pit_stop_timer();

        caltest_print_result("PIT", pit_hz);
    }

    return 0;
}

STATIC_COMMAND_START
STATIC_COMMAND("caltest", "re-run and compare the TSC calibration sources", &cmd_caltest)
STATIC_COMMAND_END(caltest);

#endif // !X86_LEGACY
