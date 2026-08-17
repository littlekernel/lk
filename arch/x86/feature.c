/*
 * Copyright (c) 2019 Travis Geiselbrecht
 * Copyright 2016 The Fuchsia Authors
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <arch/x86/feature.h>

#include <arch/x86.h>
#include <arch/x86/apicid.h>
#include <arch/x86/uarch.h>
#include <assert.h>
#include <lk/bits.h>
#include <lk/debug.h>
#include <lk/trace.h>
#include <stdlib.h>
#include <string.h>

#define LOCAL_TRACE 0

enum x86_cpu_vendor __x86_cpu_vendor = X86_CPU_VENDOR_INTEL;
enum x86_cpu_level __x86_cpu_level = X86_CPU_LEVEL_386; // start off assuming 386
enum x86_hypervisor __x86_hypervisor = X86_HYPERVISOR_NONE;
struct x86_model_info __x86_model;

static bool has_cpuid = false;

// vendor and brand strings, NUL terminated
static char vendor_string[13] = "unknown";
static char brand_string[49];

/* a saved cache of three banks of cpuids loaded a boot */
struct x86_cpuid_leaf saved_cpuids[__X86_MAX_SUPPORTED_CPUID + 1];
struct x86_cpuid_leaf saved_cpuids_hyp[__X86_MAX_SUPPORTED_CPUID_HYP - X86_CPUID_HYP_BASE + 1];
struct x86_cpuid_leaf saved_cpuids_ext[__X86_MAX_SUPPORTED_CPUID_EXT - X86_CPUID_EXT_BASE + 1];
uint32_t max_cpuid_leaf = 0;
uint32_t max_cpuid_leaf_hyp = 0;
uint32_t max_cpuid_leaf_ext = 0;

// Subleaves 1..N of the leaves that enumerate something through their subleaves. Kept as
// one flat table because it's small and only walked at boot; subleaf 0 stays in the
// indexed arrays above.
#define MAX_SAVED_SUBLEAVES 64
struct saved_subleaf {
    uint32_t leaf;
    uint32_t subleaf;
    struct x86_cpuid_leaf data;
};
static struct saved_subleaf saved_subleaves[MAX_SAVED_SUBLEAVES];
static uint32_t num_saved_subleaves = 0;

// How each subleaf enumerating leaf says where its subleaves stop.
enum subleaf_termination {
    SUBLEAF_MAX_IN_EAX,    // subleaf 0's eax holds the highest valid subleaf
    SUBLEAF_UNTIL_NO_CACHE, // walk until eax[4:0] (cache type) reads 0
    SUBLEAF_UNTIL_NO_LEVEL, // walk until ecx[15:8] (level type) reads 0
    SUBLEAF_XSAVE,         // subleaf 1 always, then one per state component bit
};

static const struct {
    uint32_t leaf;
    enum subleaf_termination termination;
    uint32_t cap; // never cache more subleaves than this, regardless of what the cpu says
} subleaf_leaves[] = {
    { X86_CPUID_CACHE_V2, SUBLEAF_UNTIL_NO_CACHE, 8 },
    { X86_CPUID_EXTENDED_FEATURE_FLAGS, SUBLEAF_MAX_IN_EAX, 4 },
    { X86_CPUID_TOPOLOGY, SUBLEAF_UNTIL_NO_LEVEL, 8 },
    { X86_CPUID_XSAVE, SUBLEAF_XSAVE, 24 },
    { X86_CPUID_PT, SUBLEAF_MAX_IN_EAX, 2 },
    { X86_CPUID_TOPOLOGY_V2, SUBLEAF_UNTIL_NO_LEVEL, 8 },
    { X86_CPUID_AVX10, SUBLEAF_MAX_IN_EAX, 2 },
    { X86_CPUID_AMD_CACHE_TOPOLOGY, SUBLEAF_UNTIL_NO_CACHE, 8 },
    { X86_CPUID_AMD_EXTENDED_TOPOLOGY, SUBLEAF_UNTIL_NO_LEVEL, 8 },
};

static enum x86_cpu_vendor match_cpu_vendor_string(const char *str) {
    // from table at https://www.sandpile.org/x86/cpuid.htm#level_0000_0000h
    if (!strcmp(str, "GenuineIntel")) {
        return X86_CPU_VENDOR_INTEL;
    }
    if (!strcmp(str, "UMC UMC UMC ")) {
        return X86_CPU_VENDOR_UMC;
    }
    if (!strcmp(str, "AuthenticAMD")) {
        return X86_CPU_VENDOR_AMD;
    }
    if (!strcmp(str, "CyrixInstead")) {
        return X86_CPU_VENDOR_CYRIX;
    }
    if (!strcmp(str, "NexGenDriven")) {
        return X86_CPU_VENDOR_NEXGEN;
    }
    if (!strcmp(str, "CentaurHauls")) {
        return X86_CPU_VENDOR_CENTAUR;
    }
    if (!strcmp(str, "RiseRiseRise")) {
        return X86_CPU_VENDOR_RISE;
    }
    if (!strcmp(str, "SiS SiS SiS ")) {
        return X86_CPU_VENDOR_SIS;
    }
    if (!strcmp(str, "GenuineTMx86")) {
        return X86_CPU_VENDOR_TRANSMETA;
    }
    if (!strcmp(str, "Geode by NSC")) {
        return X86_CPU_VENDOR_NSC;
    }
    if (!strcmp(str, "HygonGenuine")) {
        return X86_CPU_VENDOR_HYGON;
    }
    if (!strcmp(str, "  Shanghai  ")) {
        return X86_CPU_VENDOR_ZHAOXIN;
    }
    if (!strcmp(str, "VIA VIA VIA ")) {
        return X86_CPU_VENDOR_VIA;
    }
    return X86_CPU_VENDOR_UNKNOWN;
}

static enum x86_hypervisor match_hypervisor_string(const char *str) {
    static const struct {
        const char *sig;
        enum x86_hypervisor hyp;
    } table[] = {
        { "KVMKVMKVM\0\0\0", X86_HYPERVISOR_KVM },
        { "TCGTCGTCGTCG", X86_HYPERVISOR_TCG },
        { "Microsoft Hv", X86_HYPERVISOR_HYPERV },
        { "VMwareVMware", X86_HYPERVISOR_VMWARE },
        { "XenVMMXenVMM", X86_HYPERVISOR_XEN },
        { "VBoxVBoxVBox", X86_HYPERVISOR_VIRTUALBOX },
        { "bhyve bhyve ", X86_HYPERVISOR_BHYVE },
        { "ACRNACRNACRN", X86_HYPERVISOR_ACRN },
        { " lrpepyh vr", X86_HYPERVISOR_PARALLELS }, // sic, that is the real signature
    };
    for (size_t i = 0; i < countof(table); i++) {
        if (!memcmp(str, table[i].sig, 12)) {
            return table[i].hyp;
        }
    }
    return X86_HYPERVISOR_UNKNOWN;
}

const char *x86_hypervisor_name(enum x86_hypervisor hyp) {
    switch (hyp) {
        case X86_HYPERVISOR_NONE:
            return "none";
        case X86_HYPERVISOR_KVM:
            return "KVM";
        case X86_HYPERVISOR_TCG:
            return "TCG";
        case X86_HYPERVISOR_HYPERV:
            return "Hyper-V";
        case X86_HYPERVISOR_VMWARE:
            return "VMware";
        case X86_HYPERVISOR_XEN:
            return "Xen";
        case X86_HYPERVISOR_VIRTUALBOX:
            return "VirtualBox";
        case X86_HYPERVISOR_BHYVE:
            return "bhyve";
        case X86_HYPERVISOR_ACRN:
            return "ACRN";
        case X86_HYPERVISOR_PARALLELS:
            return "Parallels";
        default:
            return "unknown";
    }
}

const char *x86_get_cpu_vendor_string(void) {
    return vendor_string;
}

const char *x86_get_cpu_brand_string(void) {
    return brand_string;
}

#if X86_LEGACY
// Probe for an x87 on a cpu that can't tell us through cpuid. Classic technique: after
// FNINIT a real coprocessor leaves a zero status word and a 0x37f control word, whereas
// with no coprocessor the stores never happen and the memory keeps its preload value.
// The instructions can only reach the coprocessor with CR0.EM and CR0.TS clear, so those
// are cleared for the duration and CR0 is put back afterwards.
//
// This is untestable in QEMU (there is no 386 model and every model it has reports the
// FPU via cpuid), so it is only ever exercised on real 386/486SX class hardware.
static bool x86_probe_x87(void) {
    ulong cr0 = x86_get_cr0();
    x86_set_cr0(cr0 & ~(X86_CR0_EM | X86_CR0_TS));

    volatile uint16_t status = 0x5a5a;
    volatile uint16_t control = 0x5a5a;
    __asm__ __volatile__("fninit\n\t"
                         "fnstsw %0\n\t"
                         "fnstcw %1\n\t"
                         : "=m"(status), "=m"(control)
                         :
                         : "memory");

    x86_set_cr0(cr0);

    return (status == 0) && ((control & 0x103f) == 0x003f);
}
#endif

static void x86_cpu_detect(void) {
    if (X86_LEGACY) {
        // inspired by http://www.rcollins.org/ddj/Sep96/Sep96.html
        // try to detect a 486
        // set the EFLAGS.AC bit, see if it sets
        uint32_t flags = x86_save_flags();
        x86_restore_flags(flags | X86_FLAGS_AC);
        if (x86_save_flags() & X86_FLAGS_AC) {
            __x86_cpu_level = X86_CPU_LEVEL_486;

            // test EFLAGS.ID flag
            x86_restore_flags(flags | X86_FLAGS_ID);
            if (x86_save_flags() & X86_FLAGS_ID) {
                has_cpuid = true;
            }
        }
        x86_restore_flags(flags);
    } else {
        // at least a pentium and has cpuid
        __x86_cpu_level = X86_CPU_LEVEL_PENTIUM;
        has_cpuid = true;
    }

    if (has_cpuid) {
        uint32_t a, b, c, d;

        // read the max basic cpuid leaf
        cpuid(X86_CPUID_BASE, &a, &b, &c, &d);
        max_cpuid_leaf = MIN(a, __X86_MAX_SUPPORTED_CPUID);

        LTRACEF("cpuid leaf 0: %#x %#x %#x %#x\n", a, b, c, d);

        // read the vendor string
        memcpy(vendor_string + 0, &b, 4);
        memcpy(vendor_string + 4, &d, 4);
        memcpy(vendor_string + 8, &c, 4);
        vendor_string[12] = 0;
        __x86_cpu_vendor = match_cpu_vendor_string(vendor_string);

        LTRACEF("vendor string '%s' from cpuid\n", vendor_string);

        // read max extended cpuid leaf
        cpuid(X86_CPUID_EXT_BASE, &a, &b, &c, &d);
        if (a >= X86_CPUID_EXT_BASE) {
            max_cpuid_leaf_ext = MIN(a, __X86_MAX_SUPPORTED_CPUID_EXT);
        }
    } else {
        __x86_cpu_vendor = X86_CPU_VENDOR_INTEL; // intrinsically Intel without cpuid
    }
}

// Runs after the basic leaves are cached. Decides which hypervisor we're under and how
// much of the 0x40000000 range to believe.
static void x86_hypervisor_detect(void) {
    max_cpuid_leaf_hyp = 0;

    if (!x86_feature_test(X86_FEATURE_HYPERVISOR)) {
        __x86_hypervisor = X86_HYPERVISOR_NONE;
        return;
    }

    uint32_t a, b, c, d;
    cpuid(X86_CPUID_HYP_VENDOR, &a, &b, &c, &d);

    char sig[12];
    memcpy(sig + 0, &b, 4);
    memcpy(sig + 4, &c, 4);
    memcpy(sig + 8, &d, 4);
    __x86_hypervisor = match_hypervisor_string(sig);

    LTRACEF("hypervisor signature '%.12s' max leaf %#x -> %s\n", sig, a,
            x86_hypervisor_name(__x86_hypervisor));

    // The vendor leaf is always worth keeping. The leaves above it are laid out
    // differently by every hypervisor and the X86_FEATURE_KVM_* bits describe KVM's, so
    // only KVM (and TCG, which reuses the KVM layout) gets anything past it.
    switch (__x86_hypervisor) {
        case X86_HYPERVISOR_KVM:
        case X86_HYPERVISOR_TCG:
            max_cpuid_leaf_hyp = MIN(a, __X86_MAX_SUPPORTED_CPUID_HYP);
            if (max_cpuid_leaf_hyp < X86_CPUID_HYP_VENDOR) {
                max_cpuid_leaf_hyp = X86_CPUID_HYP_VENDOR;
            }
            break;
        default:
            max_cpuid_leaf_hyp = X86_CPUID_HYP_VENDOR;
            break;
    }
}

static void x86_model_detect(void) {
    const struct x86_cpuid_leaf *leaf = x86_get_cpuid_leaf(X86_CPUID_MODEL_FEATURES);
    if (!leaf) {
        return;
    }
    const uint32_t a = leaf->a;

    __x86_model.processor_type = BITS_SHIFT(a, 13, 12);
    __x86_model.family = BITS_SHIFT(a, 11, 8);
    __x86_model.model = BITS_SHIFT(a, 7, 4);
    __x86_model.stepping = BITS_SHIFT(a, 3, 0);
    __x86_model.display_family = __x86_model.family;
    __x86_model.display_model = __x86_model.model;

    uint32_t ext_family = BITS_SHIFT(a, 27, 20);
    uint32_t ext_model = BITS_SHIFT(a, 19, 16);

    switch (__x86_model.family) {
        case 3:
            __x86_cpu_level = X86_CPU_LEVEL_386;
            break;
        case 4:
            __x86_cpu_level = X86_CPU_LEVEL_486;
            break;
        case 5:
            __x86_cpu_level = X86_CPU_LEVEL_PENTIUM;
            break;
        case 6:
            __x86_cpu_level = X86_CPU_LEVEL_PENTIUM_PRO;
            if (x86_get_cpu_vendor() == X86_CPU_VENDOR_INTEL) {
                // Intel extends family 6 with the extended model field; AMD only does so
                // at family 0xf and above.
                __x86_model.display_model |= ext_model << 4;
            }
            break;
        case 0xf:
            __x86_cpu_level = X86_CPU_LEVEL_PENTIUM_PRO;
            __x86_model.display_family += ext_family; // family 0xf stuff is extended by bits 27:20
            __x86_model.display_model |= ext_model << 4; // extended model field extends the regular model
            break;
        default:
            // unhandled decode, assume ppro+ level
            __x86_cpu_level = X86_CPU_LEVEL_PENTIUM_PRO;
            break;
    }
}

static void x86_save_subleaf(uint32_t leaf, uint32_t subleaf) {
    if (num_saved_subleaves >= MAX_SAVED_SUBLEAVES) {
        return;
    }
    struct saved_subleaf *s = &saved_subleaves[num_saved_subleaves++];
    s->leaf = leaf;
    s->subleaf = subleaf;
    cpuid_c(leaf, subleaf, &s->data.a, &s->data.b, &s->data.c, &s->data.d);
}

// Walk the subleaves of one leaf according to its termination rule, saving each.
static void x86_cache_subleaves(uint32_t leaf, enum subleaf_termination termination,
                                uint32_t cap) {
    const struct x86_cpuid_leaf *sub0 = x86_get_cpuid_leaf(leaf);
    if (!sub0) {
        return;
    }

    switch (termination) {
        case SUBLEAF_MAX_IN_EAX: {
            const uint32_t max = MIN(sub0->a, cap);
            for (uint32_t i = 1; i <= max; i++) {
                x86_save_subleaf(leaf, i);
            }
            break;
        }
        case SUBLEAF_UNTIL_NO_CACHE:
        case SUBLEAF_UNTIL_NO_LEVEL: {
            // Subleaf 0 already tells us if there is anything at all.
            const bool sub0_valid = (termination == SUBLEAF_UNTIL_NO_CACHE)
                                        ? (BITS(sub0->a, 4, 0) != 0)
                                        : (BITS_SHIFT(sub0->c, 15, 8) != 0);
            if (!sub0_valid) {
                break;
            }
            for (uint32_t i = 1; i <= cap; i++) {
                struct x86_cpuid_leaf l;
                cpuid_c(leaf, i, &l.a, &l.b, &l.c, &l.d);
                const bool valid = (termination == SUBLEAF_UNTIL_NO_CACHE)
                                       ? (BITS(l.a, 4, 0) != 0)
                                       : (BITS_SHIFT(l.c, 15, 8) != 0);
                if (!valid) {
                    break;
                }
                x86_save_subleaf(leaf, i);
            }
            break;
        }
        case SUBLEAF_XSAVE: {
            // Subleaf 1 describes the xsave instruction variants and the supervisor state
            // mask; subleaves 2..63 exist for each state component the cpu supports, as
            // announced by the user (subleaf 0 edx:eax) and supervisor (subleaf 1 edx:ecx)
            // masks.
            x86_save_subleaf(leaf, 1);
            const struct saved_subleaf *sub1 = &saved_subleaves[num_saved_subleaves - 1];
            uint64_t mask = ((uint64_t)sub0->d << 32) | sub0->a;
            mask |= ((uint64_t)sub1->data.d << 32) | sub1->data.c;
            uint32_t saved = 1;
            for (uint32_t i = 2; i < 64 && saved < cap; i++) {
                if (mask & (1ULL << i)) {
                    x86_save_subleaf(leaf, i);
                    saved++;
                }
            }
            break;
        }
    }
}

/* early detection of cpu features on cpu 0, before the kernel is scheduling */
void x86_feature_early_init(void) {
    x86_cpu_detect();

    if (!has_cpuid) {
#if X86_LEGACY
        // Synthesize a leaf 1 from what the probes found so that everything downstream can
        // ask its questions through x86_feature_test() and never care that cpuid is missing.
        // Family from the 386/486 probe, model and stepping unknown, FPU bit from the x87
        // probe.
        saved_cpuids[X86_CPUID_MODEL_FEATURES].a = (uint32_t)__x86_cpu_level << 8;
        if (x86_probe_x87()) {
            saved_cpuids[X86_CPUID_MODEL_FEATURES].d |= (1u << 0); // X86_FEATURE_FPU
        }
        max_cpuid_leaf = X86_CPUID_MODEL_FEATURES;
        x86_model_detect();
        x86_uarch_early_init();
        x86_apic_id_layout_init();
#endif
        return;
    }

    // cache a copy of the cpuid bits
    for (uint32_t i = 0; i <= max_cpuid_leaf; i++) {
        cpuid_c(i, 0, &saved_cpuids[i].a, &saved_cpuids[i].b, &saved_cpuids[i].c,
                &saved_cpuids[i].d);
    }

    if (max_cpuid_leaf_ext > 0) {
        for (uint32_t i = X86_CPUID_EXT_BASE; i <= max_cpuid_leaf_ext; i++) {
            uint32_t index = i - X86_CPUID_EXT_BASE;
            cpuid_c(i, 0, &saved_cpuids_ext[index].a, &saved_cpuids_ext[index].b,
                    &saved_cpuids_ext[index].c, &saved_cpuids_ext[index].d);
        }
    }

    // needs leaf 1 in the cache
    x86_hypervisor_detect();
    if (max_cpuid_leaf_hyp > 0) {
        for (uint32_t i = X86_CPUID_HYP_BASE; i <= max_cpuid_leaf_hyp; i++) {
            uint32_t index = i - X86_CPUID_HYP_BASE;
            cpuid_c(i, 0, &saved_cpuids_hyp[index].a, &saved_cpuids_hyp[index].b,
                    &saved_cpuids_hyp[index].c, &saved_cpuids_hyp[index].d);
        }
    }

    // cache the higher subleaves of the leaves that use them
    for (size_t i = 0; i < countof(subleaf_leaves); i++) {
        x86_cache_subleaves(subleaf_leaves[i].leaf, subleaf_leaves[i].termination,
                            subleaf_leaves[i].cap);
    }

    // brand string, if present
    if (max_cpuid_leaf_ext >= X86_CPUID_BRAND + 2) {
        for (uint32_t i = 0; i < 3; i++) {
            const struct x86_cpuid_leaf *l = x86_get_cpuid_leaf(X86_CPUID_BRAND + i);
            memcpy(brand_string + i * 16 + 0, &l->a, 4);
            memcpy(brand_string + i * 16 + 4, &l->b, 4);
            memcpy(brand_string + i * 16 + 8, &l->c, 4);
            memcpy(brand_string + i * 16 + 12, &l->d, 4);
        }
        brand_string[48] = 0;
        // strip surrounding spaces, older parts right justify the string and some pad it
        const char *p = brand_string;
        while (*p == ' ') {
            p++;
        }
        if (p != brand_string) {
            memmove(brand_string, p, strlen(p) + 1);
        }
        for (size_t len = strlen(brand_string); len > 0 && brand_string[len - 1] == ' '; len--) {
            brand_string[len - 1] = 0;
        }
    }

    x86_model_detect();
    x86_uarch_early_init();
    x86_apic_id_layout_init();
}

static void x86_feature_dump_cpuid(void) {
    for (uint32_t i = X86_CPUID_BASE; i <= max_cpuid_leaf; i++) {
        const struct x86_cpuid_leaf *l = &saved_cpuids[i];
        printf("X86: cpuid leaf %#x: %08x %08x %08x %08x\n", i, l->a, l->b, l->c, l->d);
        for (uint32_t j = 0; j < num_saved_subleaves; j++) {
            const struct saved_subleaf *s = &saved_subleaves[j];
            if (s->leaf == i) {
                printf("X86: cpuid leaf %#x.%u: %08x %08x %08x %08x\n", i, s->subleaf, s->data.a,
                       s->data.b, s->data.c, s->data.d);
            }
        }
    }
    for (uint32_t i = X86_CPUID_HYP_BASE; i <= max_cpuid_leaf_hyp; i++) {
        const struct x86_cpuid_leaf *l = &saved_cpuids_hyp[i - X86_CPUID_HYP_BASE];
        printf("X86: cpuid leaf %#x: %08x %08x %08x %08x\n", i, l->a, l->b, l->c, l->d);
    }
    for (uint32_t i = X86_CPUID_EXT_BASE; i <= max_cpuid_leaf_ext; i++) {
        const struct x86_cpuid_leaf *l = &saved_cpuids_ext[i - X86_CPUID_EXT_BASE];
        printf("X86: cpuid leaf %#x: %08x %08x %08x %08x\n", i, l->a, l->b, l->c, l->d);
        for (uint32_t j = 0; j < num_saved_subleaves; j++) {
            const struct saved_subleaf *s = &saved_subleaves[j];
            if (s->leaf == i) {
                printf("X86: cpuid leaf %#x.%u: %08x %08x %08x %08x\n", i, s->subleaf, s->data.a,
                       s->data.b, s->data.c, s->data.d);
            }
        }
    }
}

/* later feature init hook, called after the kernel is able to schedule */
void x86_feature_init(void) {
    const struct x86_model_info *model = x86_get_model();

    dprintf(INFO, "X86: %s \"%s\"\n", vendor_string, brand_string);
    dprintf(INFO, "X86: family %#x model %#x stepping %#x (display family %#x model %#x) %s, "
                  "cpu level %d, hypervisor %s\n",
            model->family, model->model, model->stepping, model->display_family,
            model->display_model, x86_get_uarch_info()->name, x86_get_cpu_level(),
            x86_hypervisor_name(x86_get_hypervisor()));

    const struct x86_apic_id_layout *layout = x86_get_apic_id_layout();
    const struct x86_cpu_ids *ids = x86_get_cpu_ids();
    dprintf(INFO, "X86: apic id layout: %u smt bits, %u core bits (from %s)\n", layout->smt_bits,
            layout->core_bits, layout->source);
    dprintf(INFO, "X86: boot cpu apic id %#x: package %u core %u smt %u, %s\n", ids->apic_id,
            ids->package_id, ids->core_id, ids->smt_id, x86_core_type_name(ids->core_type));

    if (has_cpuid) {
        dprintf(SPEW, "X86: max cpuid leaf %#x ext %#x hyp %#x, %u subleaves cached\n",
                max_cpuid_leaf, max_cpuid_leaf_ext, max_cpuid_leaf_hyp, num_saved_subleaves);
        if (LK_DEBUGLEVEL > 1) {
            x86_feature_dump_cpuid();
        }
    } else {
        dprintf(INFO, "X86: no cpuid, x87 %spresent\n",
                x86_feature_test(X86_FEATURE_FPU) ? "" : "not ");
    }
}

const struct x86_cpuid_leaf *x86_get_cpuid_leaf_subleaf(enum x86_cpuid_leaf_num leaf,
                                                        uint32_t subleaf) {
    if (subleaf == 0) {
        return x86_get_cpuid_leaf(leaf);
    }

    for (uint32_t i = 0; i < num_saved_subleaves; i++) {
        if (saved_subleaves[i].leaf == (uint32_t)leaf && saved_subleaves[i].subleaf == subleaf) {
            return &saved_subleaves[i].data;
        }
    }
    return NULL;
}

bool x86_get_cpuid_subleaf(enum x86_cpuid_leaf_num num, uint32_t subleaf,
                           struct x86_cpuid_leaf *leaf) {
    if (!has_cpuid) {
        return false;
    }

    // make sure the leaf number is within the detected range of the three blocks we know about
    if (num < X86_CPUID_HYP_BASE) {
        if (num > max_cpuid_leaf) {
            return false;
        }
    } else if (num < X86_CPUID_EXT_BASE) {
        if (num > max_cpuid_leaf_hyp) {
            return false;
        }
    } else if (num > max_cpuid_leaf_ext) {
        return false;
    }

    cpuid_c((uint32_t)num, subleaf, &leaf->a, &leaf->b, &leaf->c, &leaf->d);
    return true;
}
