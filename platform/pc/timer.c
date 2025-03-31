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
#include "platform_p.h"
#include <arch/x86.h>
#include <arch/x86/feature.h>
#include <inttypes.h>
#include <lib/fixed_point.h>

#define LOCAL_TRACE 1

// Deals with all of the various clock sources and event timers on the PC platform.

static enum clock_source {
    CLOCK_SOURCE_INITIAL,
    CLOCK_SOURCE_PIT,
    CLOCK_SOURCE_TSC,
    CLOCK_SOURCE_HPET,
} clock_source = CLOCK_SOURCE_INITIAL;

struct fp_32_64 tsc_to_timebase;
struct fp_32_64 tsc_to_timebase_hires;

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

// From https://www.kernel.org/doc/html/v6.14/virt/kvm/x86/msr.html
struct pvclock_wall_clock {
    uint32_t   version;
    uint32_t   sec;
    uint32_t   nsec;
} __PACKED;
static_assert(sizeof(struct pvclock_wall_clock) == 12, "pvclock_wall_clock size mismatch");

struct pvclock_vcpu_time_info {
    uint32_t   version;
    uint32_t   pad0;
    uint64_t   tsc_timestamp;
    uint64_t   system_time;
    uint32_t   tsc_to_system_mul;
    int8_t     tsc_shift;
    uint8_t    flags;
    uint8_t    pad[2];
} __PACKED;
static_assert(sizeof(struct pvclock_vcpu_time_info) == 32, "pvclock_vcpu_time_info size mismatch");

static volatile struct pvclock_wall_clock *wall_clock;
static volatile struct pvclock_vcpu_time_info *vcpu_time_info;

status_t pvclock_init(void) {
    uint32_t clocksource_msr_base = 0;
    if (x86_feature_test(X86_FEATURE_KVM_CLOCKSOURCE)) {
        clocksource_msr_base = 0x11;
    }
    if (x86_feature_test(X86_FEATURE_KVM_CLOCKSOURCE2)) {
        clocksource_msr_base = 0x4b564d00;
    }
    if (!clocksource_msr_base) {
        return ERR_NOT_SUPPORTED;
    }
    dprintf(INFO, "pv_clock: clocksource detected, msr base %#x\n", clocksource_msr_base);

    // map a page of memory and point the KVM clocksource msrs at it
    void *clocksource_page;
    status_t err = vmm_alloc(vmm_get_kernel_aspace(), "lapic", PAGE_SIZE, &clocksource_page, 0, 0, 0);
    if (err != NO_ERROR) {
        printf("pv_clock: failed to allocate page for clocksource msrs\n");
        return err;
    }

    paddr_t paddr;
    arch_mmu_query(&vmm_get_kernel_aspace()->arch_aspace, (vaddr_t)clocksource_page, &paddr, NULL);
    LTRACEF("clocksource page %p, paddr %#" PRIxPTR "\n", clocksource_page, paddr);

    write_msr(clocksource_msr_base, paddr);
    write_msr(clocksource_msr_base + 1, paddr + sizeof(struct pvclock_wall_clock) + 1);

    wall_clock = (struct pvclock_wall_clock *)clocksource_page;
    vcpu_time_info = (struct pvclock_vcpu_time_info *)(wall_clock + 1);

    dprintf(SPEW, "pv_clock: wall clock version %u, sec %u, nsec %u\n",
            wall_clock->version, wall_clock->sec, wall_clock->nsec);

    dprintf(SPEW, "pv_clock: vcpu time info version %u, tsc timestamp %llu, system time %llu\n",
            vcpu_time_info->version, vcpu_time_info->tsc_timestamp, vcpu_time_info->system_time);
    dprintf(SPEW, "pv_clock: tsc to system mul %u, tsc shift %d, flags %u\n",
            vcpu_time_info->tsc_to_system_mul, vcpu_time_info->tsc_shift, vcpu_time_info->flags);

    return NO_ERROR;
}

uint64_t pvclock_get_tsc_freq(void) {
    uint32_t tsc_mul = 0;
    int8_t tsc_shift = 0;

    if (!vcpu_time_info) {
        return 0;
    }

    uint32_t pre_version = 0, post_version = 0;
    do {
        pre_version = vcpu_time_info->version;
        if (pre_version % 2 != 0) {
            asm("pause");
            continue;
        }
        tsc_mul = vcpu_time_info->tsc_to_system_mul;
        tsc_shift = vcpu_time_info->tsc_shift;
        post_version = vcpu_time_info->version;
    } while (pre_version != post_version);

    uint64_t tsc_khz = 1000000ULL << 32;
    tsc_khz = tsc_khz / tsc_mul;
    if (tsc_shift > 0) {
      tsc_khz >>= tsc_shift;
    } else {
      tsc_khz <<= -tsc_shift;
    }
    return tsc_khz * 1000;
}

bool pv_clock_is_stable(void) {
    if (!vcpu_time_info) {
        return false;
    }
    bool is_stable = (vcpu_time_info->flags & (1<<0)) ||
                     x86_feature_test(X86_FEATURE_KVM_CLOCKSOURCE_STABLE);
    return is_stable;
}

void pc_init_timer(unsigned int level) {
    // Initialize the PIT, it's always present in PC hardware
    pit_init();
    clock_source = CLOCK_SOURCE_PIT;

    lapic_init();

#if !X86_LEGACY
    // XXX update note about what invariant TSC means
    bool invariant_tsc = x86_feature_test(X86_FEATURE_INVAR_TSC);
    LTRACEF("invariant TSC %d\n", invariant_tsc);

    // Test for hypervisor PV clock, which also effectively says if TSC is invariant across
    // all cpus.
    if (pvclock_init() == NO_ERROR) {
        bool pv_clock_stable = pv_clock_is_stable();

        invariant_tsc |= pv_clock_stable;

        printf("pv_clock: Clocksource is %sstable\n", (pv_clock_stable ? "" : "not "));
    }

    // XXX test for HPET and use it over PIT if present

    if (invariant_tsc) {
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

        clock_source = CLOCK_SOURCE_TSC;
    }
out:
#endif // !X86_LEGACY

    dprintf(INFO, "PC: using %s clock source\n", clock_source_name());
}

LK_INIT_HOOK(pc_timer, pc_init_timer, LK_INIT_LEVEL_VM);

status_t platform_set_periodic_timer(platform_timer_callback callback, void *arg, lk_time_t interval) {
    return pit_set_periodic_timer(callback, arg, interval);
}

status_t platform_set_oneshot_timer(platform_timer_callback callback,
                                    void *arg, lk_time_t interval) {
    return pit_set_oneshot_timer(callback, arg, interval);
}

void platform_stop_timer(void) {
    pit_stop_timer();
}
