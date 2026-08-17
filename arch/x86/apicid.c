/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <arch/x86/apicid.h>

#include <arch/arch_ops.h>
#include <arch/x86.h>
#include <arch/x86/feature.h>
#include <arch/x86/mp.h>
#include <kernel/mp.h>
#include <lk/bits.h>
#include <lk/debug.h>
#include <lk/trace.h>
#include <string.h>

#define LOCAL_TRACE 0

static struct x86_apic_id_layout layout = { .source = "none" };

#if !WITH_SMP
static struct x86_cpu_ids boot_cpu_ids;
#endif

const struct x86_apic_id_layout *x86_get_apic_id_layout(void) {
    return &layout;
}

static uint8_t ceil_log2(uint32_t v) {
    if (v <= 1) {
        return 0;
    }
    return 32 - __builtin_clz(v - 1);
}

// The extended topology leaves (Intel 0xb / 0x1f, AMD 0x80000026) all share a format:
// each subleaf is one level, eax[4:0] is how many bits of APIC id everything at and
// below that level takes up, ecx[15:8] the level type. Subleaf 0 must be the SMT level
// (type 1) for the leaf to be valid at all.
enum topology_level_type {
    LEVEL_INVALID = 0,
    LEVEL_SMT = 1,
    LEVEL_CORE = 2,
    // 3 module, 4 tile, 5 die, 6 die group: all folded into "core" here
};

static bool layout_from_extended_topology(enum x86_cpuid_leaf_num leaf_num, const char *name) {
    const struct x86_cpuid_leaf *sub0 = x86_get_cpuid_leaf(leaf_num);
    if (!sub0 || sub0->b == 0 || BITS_SHIFT(sub0->c, 15, 8) != LEVEL_SMT) {
        return false;
    }

    layout.smt_bits = BITS_SHIFT(sub0->a, 4, 0);
    layout.core_bits = layout.smt_bits;

    // The highest level that exists is the boundary with the package id, whatever it's
    // called.
    for (uint32_t i = 1;; i++) {
        const struct x86_cpuid_leaf *sub = x86_get_cpuid_leaf_subleaf(leaf_num, i);
        if (!sub || BITS_SHIFT(sub->c, 15, 8) == LEVEL_INVALID) {
            break;
        }
        layout.core_bits = BITS_SHIFT(sub->a, 4, 0);
    }
    layout.source = name;
    return true;
}

// Pre-0xb Intel: leaf 1 says how many logical processors a package can address and leaf 4
// how many cores, so smt width is the ratio.
static bool layout_from_intel_legacy(void) {
    const struct x86_cpuid_leaf *leaf1 = x86_get_cpuid_leaf(X86_CPUID_MODEL_FEATURES);
    if (!leaf1) {
        return false;
    }
    const uint32_t max_logical = BITS_SHIFT(leaf1->b, 23, 16);
    if (max_logical == 0) {
        return false;
    }

    uint32_t max_cores = 1;
    const struct x86_cpuid_leaf *leaf4 = x86_get_cpuid_leaf(X86_CPUID_CACHE_V2);
    if (leaf4 && BITS(leaf4->a, 4, 0) != 0) {
        max_cores = BITS_SHIFT(leaf4->a, 31, 26) + 1;
    }

    layout.core_bits = ceil_log2(max_logical);
    layout.smt_bits = ceil_log2(max_logical / max_cores);
    layout.source = "cpuid 0x1/0x4";
    return true;
}

// Pre-0xb AMD: 0x8000001e gives threads per core, 0x80000008 the width of the core id
// field (or failing that the core count).
static bool layout_from_amd_legacy(void) {
    const struct x86_cpuid_leaf *leaf8 = x86_get_cpuid_leaf(X86_CPUID_ADDR_WIDTH);
    if (!leaf8) {
        return false;
    }

    uint32_t core_bits = BITS_SHIFT(leaf8->c, 15, 12);
    if (core_bits == 0) {
        core_bits = ceil_log2(BITS_SHIFT(leaf8->c, 7, 0) + 1);
    }

    uint32_t threads_per_core = 1;
    const struct x86_cpuid_leaf *leaf1e = x86_get_cpuid_leaf(X86_CPUID_AMD_TOPOLOGY);
    if (leaf1e) {
        threads_per_core = BITS_SHIFT(leaf1e->b, 15, 8) + 1;
    }

    layout.core_bits = core_bits;
    layout.smt_bits = ceil_log2(threads_per_core);
    layout.source = "cpuid 0x80000008/0x8000001e";
    return true;
}

// The last level cache's sharing width comes from the deterministic cache leaves: each
// subleaf is one cache, eax[25:14]+1 is the max number of logical cpus sharing it. Take the
// last one enumerated.
static void llc_share_from_cache_leaf(enum x86_cpuid_leaf_num leaf_num) {
    for (uint32_t i = 0;; i++) {
        const struct x86_cpuid_leaf *sub = x86_get_cpuid_leaf_subleaf(leaf_num, i);
        if (!sub || BITS(sub->a, 4, 0) == 0) {
            break;
        }
        layout.llc_share_shift = ceil_log2(BITS_SHIFT(sub->a, 25, 14) + 1);
        layout.llc_share_known = true;
    }
}

void x86_apic_id_layout_init(void) {
    // Without HTT the package holds exactly one logical cpu, and none of the topology
    // leaves are meaningful. Counterintuitively, HTT is set on plenty of parts with no
    // SMT at all: on Intel it means "multiple logical cpus per package" and on AMD
    // "more than one thread per core or core per compute unit".
    if (!x86_feature_test(X86_FEATURE_HTT)) {
        layout.source = "no HTT";
    } else if (layout_from_extended_topology(X86_CPUID_TOPOLOGY_V2, "cpuid 0x1f")) {
    } else if (layout_from_extended_topology(X86_CPUID_AMD_EXTENDED_TOPOLOGY,
                                             "cpuid 0x80000026")) {
    } else if (layout_from_extended_topology(X86_CPUID_TOPOLOGY, "cpuid 0xb")) {
    } else if (x86_get_cpu_vendor() == X86_CPU_VENDOR_AMD ||
               x86_get_cpu_vendor() == X86_CPU_VENDOR_HYGON) {
        layout_from_amd_legacy();
    } else {
        layout_from_intel_legacy();
    }

    if (x86_get_cpu_vendor() == X86_CPU_VENDOR_AMD || x86_get_cpu_vendor() == X86_CPU_VENDOR_HYGON) {
        llc_share_from_cache_leaf(X86_CPUID_AMD_CACHE_TOPOLOGY);
    } else {
        llc_share_from_cache_leaf(X86_CPUID_CACHE_V2);
    }

    LTRACEF("smt_bits %u core_bits %u llc_share_shift %u (%s) from %s\n", layout.smt_bits,
            layout.core_bits, layout.llc_share_shift, layout.llc_share_known ? "known" : "unknown",
            layout.source);
}

uint32_t x86_read_local_apic_id(void) {
    // The extended topology leaves return the executing cpu's full x2apic id in edx of
    // any subleaf. Fall back to the 8 bit initial id in leaf 1.
    struct x86_cpuid_leaf leaf;
    if (x86_get_cpuid_subleaf(X86_CPUID_TOPOLOGY_V2, 0, &leaf) && leaf.b != 0) {
        return leaf.d;
    }
    if (x86_get_cpuid_subleaf(X86_CPUID_TOPOLOGY, 0, &leaf) && leaf.b != 0) {
        return leaf.d;
    }
    if (x86_get_cpuid_subleaf(X86_CPUID_MODEL_FEATURES, 0, &leaf)) {
        return BITS_SHIFT(leaf.b, 31, 24);
    }
    return 0;
}

const char *x86_core_type_name(enum x86_core_type type) {
    switch (type) {
        case X86_CORE_TYPE_PERFORMANCE:
            return "P-core";
        case X86_CORE_TYPE_EFFICIENT:
            return "E-core";
        default:
            return "core";
    }
}

static struct x86_cpu_ids *ids_for_cpu(uint cpu_num) {
#if WITH_SMP
    return &x86_get_percpu_for_cpu(cpu_num)->ids;
#else
    return &boot_cpu_ids;
#endif
}

const struct x86_cpu_ids *x86_get_cpu_ids_for_cpu(uint cpu_num) {
    return ids_for_cpu(cpu_num);
}

const struct x86_cpu_ids *x86_get_cpu_ids(void) {
    return ids_for_cpu(arch_curr_cpu_num());
}

// Runs on each cpu. Everything in here has to be a live cpuid, since it's this cpu's
// answer we want, not the boot cpu's.
void x86_cpu_ids_init_percpu(void) {
    struct x86_cpu_ids *ids = ids_for_cpu(arch_curr_cpu_num());
    memset(ids, 0, sizeof(*ids));

    ids->apic_id = x86_read_local_apic_id();
    ids->smt_id = x86_apic_id_smt(&layout, ids->apic_id);
    ids->core_id = x86_apic_id_core(&layout, ids->apic_id);
    ids->package_id = x86_apic_id_package(&layout, ids->apic_id);
    if (layout.llc_share_known) {
        ids->llc_share_id = ids->apic_id >> layout.llc_share_shift;
    }

    struct x86_cpuid_leaf leaf;
    if (x86_feature_test(X86_FEATURE_HYBRID) &&
        x86_get_cpuid_subleaf(X86_CPUID_HYBRID, 0, &leaf)) {
        // Intel: leaf 0x1a eax[31:24] is the core type of this cpu, eax[23:0] its native
        // model id. SDM vol 2, "Native Model ID Enumeration Leaf".
        switch (BITS_SHIFT(leaf.a, 31, 24)) {
            case 0x20:
                ids->core_type = X86_CORE_TYPE_EFFICIENT; // Atom
                break;
            case 0x40:
                ids->core_type = X86_CORE_TYPE_PERFORMANCE; // Core
                break;
            default:
                break;
        }
        ids->native_model_id = BITS_SHIFT(leaf.a, 23, 0);
    } else if (x86_get_cpuid_subleaf(X86_CPUID_AMD_EXTENDED_TOPOLOGY, 0, &leaf) &&
               BIT(leaf.a, 30)) {
        // AMD: on parts flagging heterogeneous cores (eax[30]) each subleaf's ebx carries
        // the core type of this cpu in [31:28] (0 performance, 1 efficiency) and its
        // native model id in [27:24]. Per the family 19h/1ah PPRs; not yet verified on
        // hardware.
        switch (BITS_SHIFT(leaf.b, 31, 28)) {
            case 0:
                ids->core_type = X86_CORE_TYPE_PERFORMANCE;
                break;
            case 1:
                ids->core_type = X86_CORE_TYPE_EFFICIENT;
                break;
            default:
                break;
        }
        ids->native_model_id = BITS_SHIFT(leaf.b, 27, 24);
    }

    ids->valid = true;

    LTRACEF("cpu %u apic %#x pkg %u core %u smt %u llc %u %s\n", arch_curr_cpu_num(),
            ids->apic_id, ids->package_id, ids->core_id, ids->smt_id, ids->llc_share_id,
            x86_core_type_name(ids->core_type));
}

bool x86_smt_active(void) {
    if (layout.smt_bits == 0) {
        return false;
    }

    // Look for two active cpus in the same core.
    for (uint i = 0; i < SMP_MAX_CPUS; i++) {
        if (!mp_is_cpu_active(i)) {
            continue;
        }
        const struct x86_cpu_ids *a = ids_for_cpu(i);
        if (!a->valid) {
            continue;
        }
        for (uint j = i + 1; j < SMP_MAX_CPUS; j++) {
            if (!mp_is_cpu_active(j)) {
                continue;
            }
            const struct x86_cpu_ids *b = ids_for_cpu(j);
            if (b->valid && a->package_id == b->package_id && a->core_id == b->core_id) {
                return true;
            }
        }
    }
    return false;
}
