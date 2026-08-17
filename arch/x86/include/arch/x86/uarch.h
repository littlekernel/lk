/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lk/compiler.h>
#include <stdbool.h>
#include <stdint.h>

__BEGIN_CDECLS

// Microarchitecture identification.
//
// A few facts about an x86 cpu can't be discovered from cpuid at all and are only known
// per microarchitecture: the frequency of the crystal the TSC and local apic timer are
// derived from on parts that don't enumerate it, which register describes the TSC
// frequency, that sort of thing. This maps the vendor/family/model to a small enum and
// a table of plain data. It is deliberately data only: nothing here decides behaviour,
// the code that consumes a property does, so a new microarch is a new row and nothing
// else.

enum x86_uarch {
    X86_UARCH_UNKNOWN,

    // Intel, big cores
    X86_UARCH_INTEL_P5,
    X86_UARCH_INTEL_P6,
    X86_UARCH_INTEL_PENTIUM_M,
    X86_UARCH_INTEL_NETBURST,
    X86_UARCH_INTEL_CORE,
    X86_UARCH_INTEL_CORE2,
    X86_UARCH_INTEL_NEHALEM,
    X86_UARCH_INTEL_WESTMERE,
    X86_UARCH_INTEL_SANDY_BRIDGE,
    X86_UARCH_INTEL_IVY_BRIDGE,
    X86_UARCH_INTEL_HASWELL,
    X86_UARCH_INTEL_BROADWELL,
    X86_UARCH_INTEL_SKYLAKE,
    X86_UARCH_INTEL_SKYLAKE_SERVER,
    X86_UARCH_INTEL_CANNON_LAKE,
    X86_UARCH_INTEL_ICE_LAKE,
    X86_UARCH_INTEL_TIGER_LAKE,
    X86_UARCH_INTEL_ROCKET_LAKE,
    X86_UARCH_INTEL_SAPPHIRE_RAPIDS,
    X86_UARCH_INTEL_GRANITE_RAPIDS,
    X86_UARCH_INTEL_ALDER_LAKE,
    X86_UARCH_INTEL_RAPTOR_LAKE,
    X86_UARCH_INTEL_METEOR_LAKE,
    X86_UARCH_INTEL_ARROW_LAKE,
    X86_UARCH_INTEL_LUNAR_LAKE,
    X86_UARCH_INTEL_PANTHER_LAKE,

    // Intel, small cores
    X86_UARCH_INTEL_BONNELL,
    X86_UARCH_INTEL_SALTWELL,
    X86_UARCH_INTEL_SILVERMONT,
    X86_UARCH_INTEL_AIRMONT,
    X86_UARCH_INTEL_GOLDMONT,
    X86_UARCH_INTEL_GOLDMONT_D, // Denverton
    X86_UARCH_INTEL_GOLDMONT_PLUS,
    X86_UARCH_INTEL_TREMONT,
    X86_UARCH_INTEL_GRACEMONT,
    X86_UARCH_INTEL_CRESTMONT,
    X86_UARCH_INTEL_KNIGHTS,

    // AMD
    X86_UARCH_AMD_K5,
    X86_UARCH_AMD_K6,
    X86_UARCH_AMD_K7,
    X86_UARCH_AMD_K8,
    X86_UARCH_AMD_K10,
    X86_UARCH_AMD_BOBCAT,
    X86_UARCH_AMD_BULLDOZER,
    X86_UARCH_AMD_JAGUAR,
    X86_UARCH_AMD_ZEN1,
    X86_UARCH_AMD_ZEN2,
    X86_UARCH_AMD_ZEN3,
    X86_UARCH_AMD_ZEN4,
    X86_UARCH_AMD_ZEN5,

    // Fallbacks used when the vendor is known but the model isn't in the table
    X86_UARCH_INTEL_DEFAULT,
    X86_UARCH_AMD_DEFAULT,
};

// Where the TSC's nominal frequency can be read from without measuring it.
enum x86_tsc_freq_source {
    X86_TSC_FREQ_MEASURE,    // nowhere, calibrate against another clock
    X86_TSC_FREQ_CPUID_15,   // crystal * cpuid 0x15 ratio (Intel)
    X86_TSC_FREQ_AMD_PSTATE, // decode the P0 P-state MSR (AMD Zen)
};

// The TSC counts at the current core clock rather than a fixed rate. Never a timebase.
#define X86_UARCH_FLAG_TSC_NOT_CONSTANT (1u << 0)

struct x86_uarch_info {
    enum x86_uarch uarch;
    const char *name;

    // Frequency of the core crystal clock the TSC and local apic timer are derived from,
    // for parts that leave cpuid 0x15 ecx zero. 0 if unknown. Only ever consulted on
    // bare metal, a hypervisor's model number says nothing about the host's clock.
    uint32_t crystal_hz;

    enum x86_tsc_freq_source tsc_freq_source;
    uint32_t flags;
};

// Only valid after x86_feature_early_init(). Always returns a table entry, falling back
// to the vendor default and then X86_UARCH_UNKNOWN.
const struct x86_uarch_info *x86_get_uarch_info(void);

static inline enum x86_uarch x86_get_uarch(void) {
    return x86_get_uarch_info()->uarch;
}

// Called by feature.c once vendor and model are known.
void x86_uarch_early_init(void);

__END_CDECLS
