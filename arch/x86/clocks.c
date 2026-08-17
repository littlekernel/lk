/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <arch/x86/clocks.h>

#include <arch/x86.h>
#include <arch/x86/feature.h>
#include <arch/x86/uarch.h>
#include <lk/bits.h>
#include <lk/trace.h>

#define LOCAL_TRACE 0

#define MHZ(x) ((uint64_t)(x) * 1000 * 1000)

// AMD P-state definition MSRs, one per P-state starting at P0. Zen runs the TSC at the P0
// core frequency.
#define AMD_MSR_PSTATE_DEF0 0xc0010064
#define AMD_PSTATE_DEF_ENABLE (1ULL << 63)

uint64_t x86_cpu_crystal_hz(void) {
    const struct x86_cpuid_leaf *leaf = x86_get_cpuid_leaf(X86_CPUID_TSC);
    if (leaf && leaf->c != 0) {
        return leaf->c;
    }

    // The table is about the physical part, which under a hypervisor may be nothing like
    // what cpuid claims and whose crystal is not what feeds an emulated timer anyway.
    if (!x86_is_virtualized()) {
        return x86_get_uarch_info()->crystal_hz;
    }

    return 0;
}

// Intel: SDM vol 3 "Determining the Processor Base Frequency". Cpuid 0x15 gives the ratio
// TSC / crystal as ebx / eax; if either is zero the ratio isn't enumerated.
static uint64_t intel_tsc_hz_from_cpuid_15(void) {
    const struct x86_cpuid_leaf *leaf = x86_get_cpuid_leaf(X86_CPUID_TSC);
    if (!leaf || leaf->a == 0 || leaf->b == 0) {
        return 0;
    }

    const uint64_t crystal = x86_cpu_crystal_hz();
    if (crystal == 0) {
        return 0;
    }

    return crystal * leaf->b / leaf->a;
}

// AMD Zen: the P0 P-state MSR describes the frequency the TSC counts at. Two encodings
// have been used (family 17h/19h PPRs):
//   Zen 1-3: CpuFid[7:0], CpuDfsId[13:8]; CoreCOF = CpuFid / CpuDfsId * 200MHz
//   Zen 4-5: CpuFid[11:0]; CoreCOF = CpuFid * 5MHz
// The MSR is not architectural and hypervisors don't necessarily emulate it, so bare metal
// only. There is no fault safe MSR read yet, so this also leans on the vendor/family match
// in the microarch table to know the MSR exists.
static uint64_t amd_tsc_hz_from_pstate(bool fid_did_encoding) {
    if (x86_is_virtualized()) {
        return 0;
    }

    const uint64_t pstate = read_msr(AMD_MSR_PSTATE_DEF0);
    LTRACEF("P0 pstate def %#llx\n", pstate);
    if (!(pstate & AMD_PSTATE_DEF_ENABLE)) {
        return 0;
    }

    if (fid_did_encoding) {
        const uint64_t fid = BITS_SHIFT(pstate, 7, 0);
        const uint64_t did = BITS_SHIFT(pstate, 13, 8);
        if (did == 0) {
            return 0;
        }
        return fid * MHZ(200) / did;
    } else {
        const uint64_t fid = BITS_SHIFT(pstate, 11, 0);
        return fid * MHZ(5);
    }
}

uint64_t x86_cpu_tsc_hz(void) {
    const struct x86_uarch_info *uarch = x86_get_uarch_info();

    if (uarch->flags & X86_UARCH_FLAG_TSC_NOT_CONSTANT) {
        return 0;
    }

    switch (uarch->tsc_freq_source) {
        case X86_TSC_FREQ_CPUID_15:
            return intel_tsc_hz_from_cpuid_15();
        case X86_TSC_FREQ_AMD_PSTATE_FID_DID:
            return amd_tsc_hz_from_pstate(true);
        case X86_TSC_FREQ_AMD_PSTATE_FID:
            return amd_tsc_hz_from_pstate(false);
        case X86_TSC_FREQ_MEASURE:
        default:
            return 0;
    }
}

const char *x86_cpu_tsc_freq_source_name(void) {
    switch (x86_get_uarch_info()->tsc_freq_source) {
        case X86_TSC_FREQ_CPUID_15:
            return "cpuid 0x15";
        case X86_TSC_FREQ_AMD_PSTATE_FID_DID:
        case X86_TSC_FREQ_AMD_PSTATE_FID:
            return "AMD P0 P-state MSR";
        case X86_TSC_FREQ_MEASURE:
        default:
            return "none";
    }
}

uint64_t x86_cpu_lapic_timer_hz(void) {
    if (x86_is_virtualized()) {
        return 0;
    }

    // SDM vol 3, APIC timer: on parts that enumerate cpuid 0x15 the timer counts at the
    // core crystal clock. Older parts ran it off the bus clock, which cpuid never
    // enumerates, so those measure.
    const struct x86_cpuid_leaf *leaf = x86_get_cpuid_leaf(X86_CPUID_TSC);
    if (!leaf || leaf->a == 0 || leaf->b == 0) {
        return 0;
    }
    return x86_cpu_crystal_hz();
}

uint32_t x86_cpu_base_mhz(void) {
    const struct x86_cpuid_leaf *leaf = x86_get_cpuid_leaf(X86_CPUID_FREQ);
    if (!leaf) {
        return 0;
    }
    return BITS_SHIFT(leaf->a, 15, 0);
}
