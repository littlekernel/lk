/*
 * Copyright (c) 2025 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include "arch/x86/pv.h"

#include <lk/err.h>
#include <lk/trace.h>
#include <assert.h>
#include <stdint.h>
#include <inttypes.h>
#include <arch/x86/feature.h>
#include <kernel/vm.h>

#define LOCAL_TRACE 0

#if !X86_LEGACY

// Deals with paravirtualized clock sources and event timers on the PC platform,
// specifically KVM.

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
#define VCPU_TIME_INFO_FLAG_STABLE 0x1

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
    if (!vcpu_time_info) {
        return 0;
    }

    uint32_t tsc_mul = 0;
    int8_t tsc_shift = 0;
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
    bool is_stable = (vcpu_time_info->flags & VCPU_TIME_INFO_FLAG_STABLE) ||
                     x86_feature_test(X86_FEATURE_KVM_CLOCKSOURCE_STABLE);
    return is_stable;
}

#endif // !X86_LEGACY