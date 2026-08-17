/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <arch/x86/uarch.h>

#include <arch/x86/feature.h>
#include <lk/compiler.h>
#include <lk/debug.h>
#include <lk/trace.h>
#include <stddef.h>

#define LOCAL_TRACE 0

#define MHZ(x) ((x) * 1000 * 1000)

// One row per microarch. Indexed by enum x86_uarch, checked at init.
static const struct x86_uarch_info uarch_table[] = {
    [X86_UARCH_UNKNOWN] = { X86_UARCH_UNKNOWN, "unknown", 0, X86_TSC_FREQ_MEASURE, 0 },

    // Intel big cores. Everything from Sandy Bridge on has a constant TSC and everything
    // from Skylake on enumerates the ratio in cpuid 0x15; only the Skylake generation
    // leaves the crystal itself out (SDM vol 3, "Determining the Processor Base
    // Frequency"), so it's the only one that needs the crystal column.
    [X86_UARCH_INTEL_P5] = { X86_UARCH_INTEL_P5, "P5", 0, X86_TSC_FREQ_MEASURE,
                             X86_UARCH_FLAG_TSC_NOT_CONSTANT },
    [X86_UARCH_INTEL_P6] = { X86_UARCH_INTEL_P6, "P6", 0, X86_TSC_FREQ_MEASURE,
                             X86_UARCH_FLAG_TSC_NOT_CONSTANT },
    [X86_UARCH_INTEL_PENTIUM_M] = { X86_UARCH_INTEL_PENTIUM_M, "Pentium M", 0,
                                    X86_TSC_FREQ_MEASURE, X86_UARCH_FLAG_TSC_NOT_CONSTANT },
    [X86_UARCH_INTEL_NETBURST] = { X86_UARCH_INTEL_NETBURST, "NetBurst", 0,
                                   X86_TSC_FREQ_MEASURE, 0 },
    [X86_UARCH_INTEL_CORE] = { X86_UARCH_INTEL_CORE, "Core", 0, X86_TSC_FREQ_MEASURE, 0 },
    [X86_UARCH_INTEL_CORE2] = { X86_UARCH_INTEL_CORE2, "Core 2", 0, X86_TSC_FREQ_MEASURE, 0 },
    [X86_UARCH_INTEL_NEHALEM] = { X86_UARCH_INTEL_NEHALEM, "Nehalem", 0, X86_TSC_FREQ_MEASURE,
                                  0 },
    [X86_UARCH_INTEL_WESTMERE] = { X86_UARCH_INTEL_WESTMERE, "Westmere", 0,
                                   X86_TSC_FREQ_MEASURE, 0 },
    [X86_UARCH_INTEL_SANDY_BRIDGE] = { X86_UARCH_INTEL_SANDY_BRIDGE, "Sandy Bridge", 0,
                                       X86_TSC_FREQ_MEASURE, 0 },
    [X86_UARCH_INTEL_IVY_BRIDGE] = { X86_UARCH_INTEL_IVY_BRIDGE, "Ivy Bridge", 0,
                                     X86_TSC_FREQ_MEASURE, 0 },
    [X86_UARCH_INTEL_HASWELL] = { X86_UARCH_INTEL_HASWELL, "Haswell", 0, X86_TSC_FREQ_MEASURE,
                                  0 },
    [X86_UARCH_INTEL_BROADWELL] = { X86_UARCH_INTEL_BROADWELL, "Broadwell", 0,
                                    X86_TSC_FREQ_MEASURE, 0 },
    [X86_UARCH_INTEL_SKYLAKE] = { X86_UARCH_INTEL_SKYLAKE, "Skylake", MHZ(24),
                                  X86_TSC_FREQ_CPUID_15, 0 },
    [X86_UARCH_INTEL_SKYLAKE_SERVER] = { X86_UARCH_INTEL_SKYLAKE_SERVER, "Skylake-SP", MHZ(25),
                                         X86_TSC_FREQ_CPUID_15, 0 },
    [X86_UARCH_INTEL_CANNON_LAKE] = { X86_UARCH_INTEL_CANNON_LAKE, "Cannon Lake", MHZ(24),
                                      X86_TSC_FREQ_CPUID_15, 0 },
    [X86_UARCH_INTEL_ICE_LAKE] = { X86_UARCH_INTEL_ICE_LAKE, "Ice Lake", 0,
                                   X86_TSC_FREQ_CPUID_15, 0 },
    [X86_UARCH_INTEL_TIGER_LAKE] = { X86_UARCH_INTEL_TIGER_LAKE, "Tiger Lake", 0,
                                     X86_TSC_FREQ_CPUID_15, 0 },
    [X86_UARCH_INTEL_ROCKET_LAKE] = { X86_UARCH_INTEL_ROCKET_LAKE, "Rocket Lake", 0,
                                      X86_TSC_FREQ_CPUID_15, 0 },
    [X86_UARCH_INTEL_SAPPHIRE_RAPIDS] = { X86_UARCH_INTEL_SAPPHIRE_RAPIDS, "Sapphire Rapids", 0,
                                          X86_TSC_FREQ_CPUID_15, 0 },
    [X86_UARCH_INTEL_GRANITE_RAPIDS] = { X86_UARCH_INTEL_GRANITE_RAPIDS, "Granite Rapids", 0,
                                         X86_TSC_FREQ_CPUID_15, 0 },
    [X86_UARCH_INTEL_ALDER_LAKE] = { X86_UARCH_INTEL_ALDER_LAKE, "Alder Lake", 0,
                                     X86_TSC_FREQ_CPUID_15, 0 },
    [X86_UARCH_INTEL_RAPTOR_LAKE] = { X86_UARCH_INTEL_RAPTOR_LAKE, "Raptor Lake", 0,
                                      X86_TSC_FREQ_CPUID_15, 0 },
    [X86_UARCH_INTEL_METEOR_LAKE] = { X86_UARCH_INTEL_METEOR_LAKE, "Meteor Lake", 0,
                                      X86_TSC_FREQ_CPUID_15, 0 },
    [X86_UARCH_INTEL_ARROW_LAKE] = { X86_UARCH_INTEL_ARROW_LAKE, "Arrow Lake", 0,
                                     X86_TSC_FREQ_CPUID_15, 0 },
    [X86_UARCH_INTEL_LUNAR_LAKE] = { X86_UARCH_INTEL_LUNAR_LAKE, "Lunar Lake", 0,
                                     X86_TSC_FREQ_CPUID_15, 0 },
    [X86_UARCH_INTEL_PANTHER_LAKE] = { X86_UARCH_INTEL_PANTHER_LAKE, "Panther Lake", 0,
                                       X86_TSC_FREQ_CPUID_15, 0 },

    // Intel small cores. Goldmont is the Atom equivalent of Skylake: 0x15 present, ecx
    // empty (19.2MHz on Apollo Lake, 25MHz on Denverton per the SDM).
    [X86_UARCH_INTEL_BONNELL] = { X86_UARCH_INTEL_BONNELL, "Bonnell", 0, X86_TSC_FREQ_MEASURE,
                                  0 },
    [X86_UARCH_INTEL_SALTWELL] = { X86_UARCH_INTEL_SALTWELL, "Saltwell", 0,
                                   X86_TSC_FREQ_MEASURE, 0 },
    [X86_UARCH_INTEL_SILVERMONT] = { X86_UARCH_INTEL_SILVERMONT, "Silvermont", 0,
                                     X86_TSC_FREQ_MEASURE, 0 },
    [X86_UARCH_INTEL_AIRMONT] = { X86_UARCH_INTEL_AIRMONT, "Airmont", 0, X86_TSC_FREQ_MEASURE,
                                  0 },
    [X86_UARCH_INTEL_GOLDMONT] = { X86_UARCH_INTEL_GOLDMONT, "Goldmont", 19200000,
                                   X86_TSC_FREQ_CPUID_15, 0 },
    [X86_UARCH_INTEL_GOLDMONT_D] = { X86_UARCH_INTEL_GOLDMONT_D, "Goldmont (Denverton)",
                                     MHZ(25), X86_TSC_FREQ_CPUID_15, 0 },
    [X86_UARCH_INTEL_GOLDMONT_PLUS] = { X86_UARCH_INTEL_GOLDMONT_PLUS, "Goldmont Plus", 0,
                                        X86_TSC_FREQ_CPUID_15, 0 },
    [X86_UARCH_INTEL_TREMONT] = { X86_UARCH_INTEL_TREMONT, "Tremont", 0, X86_TSC_FREQ_CPUID_15,
                                  0 },
    [X86_UARCH_INTEL_GRACEMONT] = { X86_UARCH_INTEL_GRACEMONT, "Gracemont", 0,
                                    X86_TSC_FREQ_CPUID_15, 0 },
    [X86_UARCH_INTEL_CRESTMONT] = { X86_UARCH_INTEL_CRESTMONT, "Crestmont", 0,
                                    X86_TSC_FREQ_CPUID_15, 0 },
    [X86_UARCH_INTEL_KNIGHTS] = { X86_UARCH_INTEL_KNIGHTS, "Knights Landing/Mill", 0,
                                  X86_TSC_FREQ_MEASURE, 0 },

    // AMD. Zen runs the TSC at the P0 P-state frequency, which is readable from an MSR.
    [X86_UARCH_AMD_K5] = { X86_UARCH_AMD_K5, "K5", 0, X86_TSC_FREQ_MEASURE,
                           X86_UARCH_FLAG_TSC_NOT_CONSTANT },
    [X86_UARCH_AMD_K6] = { X86_UARCH_AMD_K6, "K6", 0, X86_TSC_FREQ_MEASURE,
                           X86_UARCH_FLAG_TSC_NOT_CONSTANT },
    [X86_UARCH_AMD_K7] = { X86_UARCH_AMD_K7, "K7", 0, X86_TSC_FREQ_MEASURE,
                           X86_UARCH_FLAG_TSC_NOT_CONSTANT },
    [X86_UARCH_AMD_K8] = { X86_UARCH_AMD_K8, "K8", 0, X86_TSC_FREQ_MEASURE,
                           X86_UARCH_FLAG_TSC_NOT_CONSTANT },
    [X86_UARCH_AMD_K10] = { X86_UARCH_AMD_K10, "K10", 0, X86_TSC_FREQ_MEASURE, 0 },
    [X86_UARCH_AMD_BOBCAT] = { X86_UARCH_AMD_BOBCAT, "Bobcat", 0, X86_TSC_FREQ_MEASURE, 0 },
    [X86_UARCH_AMD_BULLDOZER] = { X86_UARCH_AMD_BULLDOZER, "Bulldozer", 0, X86_TSC_FREQ_MEASURE,
                                  0 },
    [X86_UARCH_AMD_JAGUAR] = { X86_UARCH_AMD_JAGUAR, "Jaguar", 0, X86_TSC_FREQ_MEASURE, 0 },
    [X86_UARCH_AMD_ZEN1] = { X86_UARCH_AMD_ZEN1, "Zen", 0, X86_TSC_FREQ_AMD_PSTATE, 0 },
    [X86_UARCH_AMD_ZEN2] = { X86_UARCH_AMD_ZEN2, "Zen 2", 0, X86_TSC_FREQ_AMD_PSTATE, 0 },
    [X86_UARCH_AMD_ZEN3] = { X86_UARCH_AMD_ZEN3, "Zen 3", 0, X86_TSC_FREQ_AMD_PSTATE, 0 },
    [X86_UARCH_AMD_ZEN4] = { X86_UARCH_AMD_ZEN4, "Zen 4", 0, X86_TSC_FREQ_AMD_PSTATE, 0 },
    [X86_UARCH_AMD_ZEN5] = { X86_UARCH_AMD_ZEN5, "Zen 5", 0, X86_TSC_FREQ_AMD_PSTATE, 0 },

    // Vendor defaults: assume the newest known conventions, everything is verified
    // against real clocks before it's trusted anyway.
    [X86_UARCH_INTEL_DEFAULT] = { X86_UARCH_INTEL_DEFAULT, "Intel (unknown model)", 0,
                                  X86_TSC_FREQ_CPUID_15, 0 },
    [X86_UARCH_AMD_DEFAULT] = { X86_UARCH_AMD_DEFAULT, "AMD (unknown model)", 0,
                                X86_TSC_FREQ_MEASURE, 0 },
};

// vendor + display family + inclusive display model range -> uarch. First match wins.
struct uarch_match {
    enum x86_cpu_vendor vendor;
    uint16_t family;
    uint16_t model_lo;
    uint16_t model_hi;
    enum x86_uarch uarch;
};

#define INTEL6(lo, hi, u) { X86_CPU_VENDOR_INTEL, 0x6, (lo), (hi), (u) }
#define AMD(fam, lo, hi, u) { X86_CPU_VENDOR_AMD, (fam), (lo), (hi), (u) }

// Intel display family/model numbers are from the SDM vol 4 chapter 2 (the "CPUID
// Signature Values of DisplayFamily_DisplayModel" table that opens the MSR listings) and
// the Intel Architecture Instruction Set Extensions and Future Features Programming
// Reference for parts newer than the SDM revision at hand; AMD from the per-family BKDGs
// and PPRs, whose titles carry the family/model range each one covers. The public
// signature table at https://www.sandpile.org/x86/cpuid.htm collects all of the above
// (plus the VIA/Zhaoxin lineage) in one place and was used to cross-check the whole table.
static const struct uarch_match uarch_matches[] = {
    { X86_CPU_VENDOR_INTEL, 0x5, 0x00, 0xff, X86_UARCH_INTEL_P5 },
    INTEL6(0x00, 0x08, X86_UARCH_INTEL_P6),         // Pentium Pro / II / III
    INTEL6(0x09, 0x09, X86_UARCH_INTEL_PENTIUM_M),  // Banias
    INTEL6(0x0a, 0x0b, X86_UARCH_INTEL_P6),         // Pentium III Xeon / Tualatin
    INTEL6(0x0d, 0x0d, X86_UARCH_INTEL_PENTIUM_M),  // Dothan
    INTEL6(0x0e, 0x0e, X86_UARCH_INTEL_CORE),       // Yonah
    INTEL6(0x0f, 0x0f, X86_UARCH_INTEL_CORE2),      // Merom
    INTEL6(0x16, 0x17, X86_UARCH_INTEL_CORE2),      // Merom-L, Penryn
    INTEL6(0x1a, 0x1a, X86_UARCH_INTEL_NEHALEM),
    INTEL6(0x1c, 0x1c, X86_UARCH_INTEL_BONNELL),
    INTEL6(0x1d, 0x1d, X86_UARCH_INTEL_CORE2),      // Dunnington
    INTEL6(0x1e, 0x1f, X86_UARCH_INTEL_NEHALEM),
    INTEL6(0x25, 0x25, X86_UARCH_INTEL_WESTMERE),
    INTEL6(0x26, 0x26, X86_UARCH_INTEL_BONNELL),
    INTEL6(0x27, 0x27, X86_UARCH_INTEL_SALTWELL),
    INTEL6(0x2a, 0x2a, X86_UARCH_INTEL_SANDY_BRIDGE),
    INTEL6(0x2c, 0x2c, X86_UARCH_INTEL_WESTMERE),
    INTEL6(0x2d, 0x2d, X86_UARCH_INTEL_SANDY_BRIDGE),
    INTEL6(0x2e, 0x2e, X86_UARCH_INTEL_NEHALEM),
    INTEL6(0x2f, 0x2f, X86_UARCH_INTEL_WESTMERE),
    INTEL6(0x35, 0x36, X86_UARCH_INTEL_SALTWELL),
    INTEL6(0x37, 0x37, X86_UARCH_INTEL_SILVERMONT),
    INTEL6(0x3a, 0x3a, X86_UARCH_INTEL_IVY_BRIDGE),
    INTEL6(0x3c, 0x3c, X86_UARCH_INTEL_HASWELL),
    INTEL6(0x3d, 0x3d, X86_UARCH_INTEL_BROADWELL),
    INTEL6(0x3e, 0x3e, X86_UARCH_INTEL_IVY_BRIDGE),
    INTEL6(0x3f, 0x3f, X86_UARCH_INTEL_HASWELL),
    INTEL6(0x45, 0x46, X86_UARCH_INTEL_HASWELL),
    INTEL6(0x47, 0x47, X86_UARCH_INTEL_BROADWELL),
    INTEL6(0x4a, 0x4a, X86_UARCH_INTEL_SILVERMONT),
    INTEL6(0x4c, 0x4c, X86_UARCH_INTEL_AIRMONT),
    INTEL6(0x4d, 0x4d, X86_UARCH_INTEL_SILVERMONT),
    INTEL6(0x4e, 0x4e, X86_UARCH_INTEL_SKYLAKE),
    INTEL6(0x4f, 0x4f, X86_UARCH_INTEL_BROADWELL),
    INTEL6(0x55, 0x55, X86_UARCH_INTEL_SKYLAKE_SERVER), // also Cascade/Cooper Lake
    INTEL6(0x56, 0x56, X86_UARCH_INTEL_BROADWELL),
    INTEL6(0x57, 0x57, X86_UARCH_INTEL_KNIGHTS),
    INTEL6(0x5a, 0x5a, X86_UARCH_INTEL_SILVERMONT),
    INTEL6(0x5c, 0x5c, X86_UARCH_INTEL_GOLDMONT),
    INTEL6(0x5d, 0x5d, X86_UARCH_INTEL_SILVERMONT),
    INTEL6(0x5e, 0x5e, X86_UARCH_INTEL_SKYLAKE),
    INTEL6(0x5f, 0x5f, X86_UARCH_INTEL_GOLDMONT_D),
    INTEL6(0x66, 0x66, X86_UARCH_INTEL_CANNON_LAKE),
    INTEL6(0x6a, 0x6a, X86_UARCH_INTEL_ICE_LAKE),
    INTEL6(0x6c, 0x6c, X86_UARCH_INTEL_ICE_LAKE),
    INTEL6(0x75, 0x75, X86_UARCH_INTEL_AIRMONT),
    INTEL6(0x7a, 0x7a, X86_UARCH_INTEL_GOLDMONT_PLUS),
    INTEL6(0x7d, 0x7e, X86_UARCH_INTEL_ICE_LAKE),
    INTEL6(0x85, 0x85, X86_UARCH_INTEL_KNIGHTS),
    INTEL6(0x86, 0x86, X86_UARCH_INTEL_TREMONT),
    INTEL6(0x8a, 0x8a, X86_UARCH_INTEL_TREMONT),
    INTEL6(0x8c, 0x8d, X86_UARCH_INTEL_TIGER_LAKE),
    INTEL6(0x8e, 0x8e, X86_UARCH_INTEL_SKYLAKE), // Kaby/Coffee/Whiskey/Amber/Comet Lake U/Y
    INTEL6(0x8f, 0x8f, X86_UARCH_INTEL_SAPPHIRE_RAPIDS),
    INTEL6(0x96, 0x96, X86_UARCH_INTEL_TREMONT),
    INTEL6(0x97, 0x97, X86_UARCH_INTEL_ALDER_LAKE),
    INTEL6(0x9a, 0x9a, X86_UARCH_INTEL_ALDER_LAKE),
    INTEL6(0x9c, 0x9c, X86_UARCH_INTEL_TREMONT),
    INTEL6(0x9e, 0x9e, X86_UARCH_INTEL_SKYLAKE), // Kaby/Coffee Lake H/S
    INTEL6(0xa5, 0xa6, X86_UARCH_INTEL_SKYLAKE), // Comet Lake
    INTEL6(0xa7, 0xa7, X86_UARCH_INTEL_ROCKET_LAKE),
    INTEL6(0xaa, 0xaa, X86_UARCH_INTEL_METEOR_LAKE),
    INTEL6(0xac, 0xac, X86_UARCH_INTEL_METEOR_LAKE),
    INTEL6(0xad, 0xae, X86_UARCH_INTEL_GRANITE_RAPIDS),
    INTEL6(0xaf, 0xaf, X86_UARCH_INTEL_CRESTMONT), // Sierra Forest
    INTEL6(0xb5, 0xb5, X86_UARCH_INTEL_ARROW_LAKE),
    INTEL6(0xb6, 0xb6, X86_UARCH_INTEL_CRESTMONT), // Grand Ridge
    INTEL6(0xb7, 0xb7, X86_UARCH_INTEL_RAPTOR_LAKE),
    INTEL6(0xba, 0xba, X86_UARCH_INTEL_RAPTOR_LAKE),
    INTEL6(0xbd, 0xbd, X86_UARCH_INTEL_LUNAR_LAKE),
    INTEL6(0xbe, 0xbe, X86_UARCH_INTEL_GRACEMONT), // Alder Lake N
    INTEL6(0xbf, 0xbf, X86_UARCH_INTEL_RAPTOR_LAKE),
    INTEL6(0xc5, 0xc6, X86_UARCH_INTEL_ARROW_LAKE),
    INTEL6(0xcc, 0xcc, X86_UARCH_INTEL_PANTHER_LAKE),
    INTEL6(0xd5, 0xd5, X86_UARCH_INTEL_PANTHER_LAKE), // Wildcat Lake
    INTEL6(0xcf, 0xcf, X86_UARCH_INTEL_SAPPHIRE_RAPIDS), // Emerald Rapids
    { X86_CPU_VENDOR_INTEL, 0xf, 0x00, 0xff, X86_UARCH_INTEL_NETBURST },

    AMD(0x5, 0x00, 0x03, X86_UARCH_AMD_K5),
    AMD(0x5, 0x04, 0xff, X86_UARCH_AMD_K6),
    AMD(0x6, 0x00, 0xff, X86_UARCH_AMD_K7),
    AMD(0xf, 0x00, 0xff, X86_UARCH_AMD_K8),
    AMD(0x10, 0x00, 0xff, X86_UARCH_AMD_K10),
    AMD(0x11, 0x00, 0xff, X86_UARCH_AMD_K10),   // Turion X2 Ultra
    AMD(0x12, 0x00, 0xff, X86_UARCH_AMD_K10),   // Llano
    AMD(0x14, 0x00, 0xff, X86_UARCH_AMD_BOBCAT),
    AMD(0x15, 0x00, 0xff, X86_UARCH_AMD_BULLDOZER),
    AMD(0x16, 0x00, 0xff, X86_UARCH_AMD_JAGUAR),
    AMD(0x17, 0x00, 0x2f, X86_UARCH_AMD_ZEN1),
    AMD(0x17, 0x30, 0xff, X86_UARCH_AMD_ZEN2),
    AMD(0x19, 0x00, 0x0f, X86_UARCH_AMD_ZEN3),
    AMD(0x19, 0x10, 0x1f, X86_UARCH_AMD_ZEN4),  // Genoa
    AMD(0x19, 0x20, 0x5f, X86_UARCH_AMD_ZEN3),
    AMD(0x19, 0x60, 0x7f, X86_UARCH_AMD_ZEN4),  // Raphael, Phoenix
    AMD(0x19, 0xa0, 0xaf, X86_UARCH_AMD_ZEN4),  // Bergamo
    AMD(0x1a, 0x00, 0xff, X86_UARCH_AMD_ZEN5),
    { X86_CPU_VENDOR_HYGON, 0x18, 0x00, 0xff, X86_UARCH_AMD_ZEN1 }, // Dhyana
};

static const struct x86_uarch_info *uarch_info = &uarch_table[X86_UARCH_UNKNOWN];

const struct x86_uarch_info *x86_get_uarch_info(void) {
    return uarch_info;
}

void x86_uarch_early_init(void) {
    static_assert(countof(uarch_table) == X86_UARCH_AMD_DEFAULT + 1, "uarch table incomplete");

    const enum x86_cpu_vendor vendor = x86_get_cpu_vendor();
    const struct x86_model_info *model = x86_get_model();

    enum x86_uarch uarch = X86_UARCH_UNKNOWN;
    for (size_t i = 0; i < countof(uarch_matches); i++) {
        const struct uarch_match *m = &uarch_matches[i];
        if (m->vendor == vendor && m->family == model->display_family &&
            model->display_model >= m->model_lo && model->display_model <= m->model_hi) {
            uarch = m->uarch;
            break;
        }
    }
    if (uarch == X86_UARCH_UNKNOWN) {
        if (vendor == X86_CPU_VENDOR_INTEL) {
            uarch = X86_UARCH_INTEL_DEFAULT;
        } else if (vendor == X86_CPU_VENDOR_AMD || vendor == X86_CPU_VENDOR_HYGON) {
            uarch = X86_UARCH_AMD_DEFAULT;
        }
    }

    uarch_info = &uarch_table[uarch];
    DEBUG_ASSERT(uarch_info->uarch == uarch);
    LTRACEF("vendor %d family %#x model %#x -> uarch %d '%s'\n", vendor, model->display_family,
            model->display_model, uarch, uarch_info->name);
}
