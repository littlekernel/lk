/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

// Consistency checks on the x86 cpu identification code: that the boot time cpuid cache
// really is what the cpu says, that the microarch and apic id decoders agree with
// themselves, and that the clock answers are at least sane. None of this knows what the
// right answer for a given machine is; it checks the pieces against each other.

#include <arch/ops.h>
#include <arch/x86.h>
#include <arch/x86/apicid.h>
#include <arch/x86/clocks.h>
#include <arch/x86/feature.h>
#include <arch/x86/mp.h>
#include <arch/x86/uarch.h>
#include <kernel/mp.h>
#include <kernel/thread.h>
#include <lib/unittest.h>
#include <lk/compiler.h>

// Run the body of a test on cpu 0, so live cpuid results are the boot cpu's and comparable
// to the boot time cache (a hybrid part's cores don't all answer the same).
static int pin_to_cpu(int cpu) {
    thread_t *t = get_current_thread();
    const int old = thread_pinned_cpu(t);
    thread_set_pinned_cpu(t, cpu);
    thread_yield();
    return old;
}

static void unpin(int old) {
    thread_set_pinned_cpu(get_current_thread(), old);
}

static bool leaf_equal(const struct x86_cpuid_leaf *a, const struct x86_cpuid_leaf *b) {
    return a->a == b->a && a->b == b->b && a->c == b->c && a->d == b->d;
}

// Every subleaf 0 in all three ranges, and every cached higher subleaf, matches a live cpuid
// on the boot cpu.
static bool test_cpuid_cache_matches_live(void) {
    BEGIN_TEST;

    const int old = pin_to_cpu(0);
    ASSERT_EQ(0u, arch_curr_cpu_num(), "could not pin to cpu 0");

    static const uint32_t bases[] = { X86_CPUID_BASE, X86_CPUID_HYP_BASE, X86_CPUID_EXT_BASE };
    const uint32_t maxes[] = { max_cpuid_leaf, max_cpuid_leaf_hyp, max_cpuid_leaf_ext };

    for (size_t r = 0; r < countof(bases); r++) {
        for (uint32_t leaf = bases[r]; leaf <= maxes[r] && maxes[r] != 0; leaf++) {
            const struct x86_cpuid_leaf *cached = x86_get_cpuid_leaf(leaf);
            ASSERT_NONNULL(cached, "leaf in range not cached");

            struct x86_cpuid_leaf live;
            ASSERT_TRUE(x86_get_cpuid_subleaf(leaf, 0, &live), "live read refused");
            if (!leaf_equal(cached, &live)) {
                unittest_printf("leaf %#x: cached %08x %08x %08x %08x live %08x %08x %08x %08x\n",
                                leaf, cached->a, cached->b, cached->c, cached->d, live.a, live.b,
                                live.c, live.d);
            }
            EXPECT_TRUE(leaf_equal(cached, &live), "cached leaf differs from live cpuid");

            // walk whatever higher subleaves got cached for this leaf
            for (uint32_t sub = 1; sub < 64; sub++) {
                const struct x86_cpuid_leaf *csub = x86_get_cpuid_leaf_subleaf(leaf, sub);
                if (!csub) {
                    break;
                }
                ASSERT_TRUE(x86_get_cpuid_subleaf(leaf, sub, &live), "live subleaf read refused");
                EXPECT_TRUE(leaf_equal(csub, &live), "cached subleaf differs from live cpuid");
            }
        }
    }

    // subleaf 0 through the subleaf accessor is the same object as through the leaf one
    EXPECT_EQ(x86_get_cpuid_leaf(X86_CPUID_MODEL_FEATURES),
              x86_get_cpuid_leaf_subleaf(X86_CPUID_MODEL_FEATURES, 0), "");

    // nothing outside the detected ranges is served
    EXPECT_NULL(x86_get_cpuid_leaf((enum x86_cpuid_leaf_num)(max_cpuid_leaf + 1)), "");
    if (max_cpuid_leaf_ext != 0) {
        EXPECT_NULL(x86_get_cpuid_leaf((enum x86_cpuid_leaf_num)(max_cpuid_leaf_ext + 1)), "");
    } else {
        EXPECT_NULL(x86_get_cpuid_leaf(X86_CPUID_EXT_BASE), "");
    }
    EXPECT_NULL(x86_get_cpuid_leaf_subleaf(X86_CPUID_MODEL_FEATURES, 1), "");

    unpin(old);
    END_TEST;
}

static bool test_identification(void) {
    BEGIN_TEST;

    const struct x86_model_info *model = x86_get_model();

    EXPECT_GE(x86_get_cpu_level(), X86_CPU_LEVEL_386, "");
    EXPECT_LE(x86_get_cpu_level(), X86_CPU_LEVEL_PENTIUM_PRO, "");

    // family maps onto the level the way feature.c decodes it
    if (model->family <= 5) {
        EXPECT_EQ((int)model->family, (int)x86_get_cpu_level(), "");
    } else {
        EXPECT_EQ((int)X86_CPU_LEVEL_PENTIUM_PRO, (int)x86_get_cpu_level(), "");
    }

    // display fields extend, never lose, the base ones
    EXPECT_GE(model->display_family, model->family, "");
    EXPECT_EQ(model->display_model & 0xf, model->model, "");

    // the vendor enum and string agree
    const char *vs = x86_get_cpu_vendor_string();
    if (x86_get_cpu_vendor() == X86_CPU_VENDOR_INTEL && max_cpuid_leaf > 1) {
        EXPECT_EQ(0, strcmp(vs, "GenuineIntel"), "");
    } else if (x86_get_cpu_vendor() == X86_CPU_VENDOR_AMD) {
        EXPECT_EQ(0, strcmp(vs, "AuthenticAMD"), "");
    }
    EXPECT_EQ(12u, strlen(vs) == 7 ? 12u : strlen(vs), "vendor string is 12 chars or 'unknown'");

    // hypervisor enum follows the cpuid bit
    if (x86_feature_test(X86_FEATURE_HYPERVISOR)) {
        EXPECT_TRUE(x86_is_virtualized(), "");
        EXPECT_NE((int)X86_HYPERVISOR_NONE, (int)x86_get_hypervisor(), "");
        EXPECT_NONNULL(x86_get_cpuid_leaf(X86_CPUID_HYP_VENDOR), "hypervisor vendor leaf cached");
    } else {
        EXPECT_FALSE(x86_is_virtualized(), "");
        EXPECT_EQ((int)X86_HYPERVISOR_NONE, (int)x86_get_hypervisor(), "");
        EXPECT_NULL(x86_get_cpuid_leaf(X86_CPUID_HYP_VENDOR), "");
    }

    // KVM feature bits are only ever true under KVM/TCG
    if (x86_get_hypervisor() != X86_HYPERVISOR_KVM && x86_get_hypervisor() != X86_HYPERVISOR_TCG) {
        EXPECT_FALSE(x86_feature_test(X86_FEATURE_KVM_CLOCKSOURCE), "");
        EXPECT_FALSE(x86_feature_test(X86_FEATURE_KVM_CLOCKSOURCE2), "");
    }

    END_TEST;
}

static bool test_uarch_table(void) {
    BEGIN_TEST;

    // every row is at its own index and has a name
    for (int u = X86_UARCH_UNKNOWN; u <= X86_UARCH_AMD_DEFAULT; u++) {
        const struct x86_uarch_info *info = x86_uarch_info_for((enum x86_uarch)u);
        EXPECT_EQ(u, (int)info->uarch, "uarch table row at wrong index");
        EXPECT_NONNULL(info->name, "");
    }

    // the running cpu's entry is the one the lookup gives for its model
    const struct x86_model_info *model = x86_get_model();
    EXPECT_EQ((int)x86_uarch_lookup(x86_get_cpu_vendor(), model->display_family,
                                    model->display_model),
              (int)x86_get_uarch(), "");
    EXPECT_EQ(x86_uarch_info_for(x86_get_uarch()), x86_get_uarch_info(), "");

    // a few known parts
    EXPECT_EQ((int)X86_UARCH_INTEL_SKYLAKE, (int)x86_uarch_lookup(X86_CPU_VENDOR_INTEL, 6, 0x5e), "");
    EXPECT_EQ((int)X86_UARCH_INTEL_SKYLAKE, (int)x86_uarch_lookup(X86_CPU_VENDOR_INTEL, 6, 0x9e), "");
    EXPECT_EQ((int)X86_UARCH_INTEL_ALDER_LAKE, (int)x86_uarch_lookup(X86_CPU_VENDOR_INTEL, 6, 0x97), "");
    EXPECT_EQ((int)X86_UARCH_INTEL_NETBURST, (int)x86_uarch_lookup(X86_CPU_VENDOR_INTEL, 0xf, 2), "");
    EXPECT_EQ((int)X86_UARCH_INTEL_P5, (int)x86_uarch_lookup(X86_CPU_VENDOR_INTEL, 5, 2), "");
    EXPECT_EQ((int)X86_UARCH_AMD_ZEN2, (int)x86_uarch_lookup(X86_CPU_VENDOR_AMD, 0x17, 0x71), "");
    EXPECT_EQ((int)X86_UARCH_AMD_ZEN3, (int)x86_uarch_lookup(X86_CPU_VENDOR_AMD, 0x19, 0x21), "");
    EXPECT_EQ((int)X86_UARCH_AMD_ZEN4, (int)x86_uarch_lookup(X86_CPU_VENDOR_AMD, 0x19, 0x61), "");
    EXPECT_EQ((int)X86_UARCH_AMD_ZEN5, (int)x86_uarch_lookup(X86_CPU_VENDOR_AMD, 0x1a, 0x44), "");

    // and the fallbacks
    EXPECT_EQ((int)X86_UARCH_INTEL_DEFAULT, (int)x86_uarch_lookup(X86_CPU_VENDOR_INTEL, 6, 0xfe), "");
    EXPECT_EQ((int)X86_UARCH_AMD_DEFAULT, (int)x86_uarch_lookup(X86_CPU_VENDOR_AMD, 0x1b, 0), "");
    EXPECT_EQ((int)X86_UARCH_UNKNOWN, (int)x86_uarch_lookup(X86_CPU_VENDOR_CYRIX, 6, 0), "");

    // Skylake is the reason the crystal column exists
    EXPECT_EQ(24000000u, x86_uarch_info_for(X86_UARCH_INTEL_SKYLAKE)->crystal_hz, "");
    EXPECT_EQ(0u, x86_uarch_info_for(X86_UARCH_INTEL_DEFAULT)->crystal_hz, "");

    END_TEST;
}

static bool test_apic_id_decode(void) {
    BEGIN_TEST;

    const struct x86_apic_id_layout *layout = x86_get_apic_id_layout();
    EXPECT_LE(layout->smt_bits, layout->core_bits, "smt field is inside the core field");
    EXPECT_LT(layout->core_bits, 32, "");
    EXPECT_NONNULL(layout->source, "");

    if (!x86_feature_test(X86_FEATURE_HTT)) {
        EXPECT_EQ(0, layout->smt_bits, "no HTT means no smt field");
        EXPECT_EQ(0, layout->core_bits, "no HTT means no core field");
    }

    // every active cpu's record decodes back to itself
    for (uint cpu = 0; cpu < SMP_MAX_CPUS; cpu++) {
        if (!mp_is_cpu_active(cpu)) {
            continue;
        }
        const struct x86_cpu_ids *ids = x86_get_cpu_ids_for_cpu(cpu);
        ASSERT_TRUE(ids->valid, "active cpu has no ids");
        EXPECT_EQ(x86_apic_id_smt(layout, ids->apic_id), ids->smt_id, "");
        EXPECT_EQ(x86_apic_id_core(layout, ids->apic_id), ids->core_id, "");
        EXPECT_EQ(x86_apic_id_package(layout, ids->apic_id), ids->package_id, "");
        EXPECT_LT(ids->smt_id, 1u << layout->smt_bits, "");
        if (layout->llc_share_known) {
            EXPECT_EQ(ids->apic_id >> layout->llc_share_shift, ids->llc_share_id, "");
        }
#if WITH_SMP
        // the id cpuid reports is the one the local apic reports (the xapic register only
        // holds 8 bits of it)
        const uint32_t lapic_id = x86_get_percpu_for_cpu(cpu)->apic_id;
        if (lapic_id < 255) {
            EXPECT_EQ(lapic_id, ids->apic_id, "cpuid and local apic disagree on apic id");
        }
#endif
    }

    // no two active cpus claim the same apic id
    for (uint a = 0; a < SMP_MAX_CPUS; a++) {
        if (!mp_is_cpu_active(a)) {
            continue;
        }
        for (uint b = a + 1; b < SMP_MAX_CPUS; b++) {
            if (!mp_is_cpu_active(b)) {
                continue;
            }
            EXPECT_NE(x86_get_cpu_ids_for_cpu(a)->apic_id, x86_get_cpu_ids_for_cpu(b)->apic_id,
                      "duplicate apic id");
        }
    }

    // reading our own id live gives our own record
    const int old = pin_to_cpu(0);
    EXPECT_EQ(x86_get_cpu_ids_for_cpu(0)->apic_id, x86_read_local_apic_id(), "");
    EXPECT_EQ(x86_get_cpu_ids_for_cpu(0), x86_get_cpu_ids(), "");
    unpin(old);

    // smt can only be active if there's an smt field
    if (layout->smt_bits == 0) {
        EXPECT_FALSE(x86_smt_active(), "");
    }

    END_TEST;
}

static bool test_clocks(void) {
    BEGIN_TEST;

    const uint64_t crystal = x86_cpu_crystal_hz();
    const uint64_t tsc = x86_cpu_tsc_hz();
    const uint64_t lapic = x86_cpu_lapic_timer_hz();

    // 0 or plausible
    if (crystal != 0) {
        EXPECT_GE(crystal, 1000000u, "crystal below 1MHz");
        EXPECT_LE(crystal, 1000000000u, "crystal above 1GHz");
    }
    if (tsc != 0) {
        EXPECT_GE(tsc, 100u * 1000 * 1000, "TSC below 100MHz");
        EXPECT_LE(tsc, 10000ull * 1000 * 1000, "TSC above 10GHz");
        EXPECT_FALSE(x86_get_uarch_info()->flags & X86_UARCH_FLAG_TSC_NOT_CONSTANT,
                     "a non-constant TSC has no nominal frequency");
    }
    if (lapic != 0) {
        EXPECT_EQ(crystal, lapic, "lapic timer runs at the crystal when known");
    }

    // nothing about the host's clocks is claimed under a hypervisor except what it says
    // through cpuid 0x15 itself
    if (x86_is_virtualized()) {
        EXPECT_EQ(0u, lapic, "");
        const struct x86_cpuid_leaf *l = x86_get_cpuid_leaf(X86_CPUID_TSC);
        if (!l || l->c == 0) {
            EXPECT_EQ(0u, crystal, "");
        }
    }

    // the Intel path is a straight ratio of what the leaf says
    const struct x86_cpuid_leaf *l = x86_get_cpuid_leaf(X86_CPUID_TSC);
    if (x86_get_uarch_info()->tsc_freq_source == X86_TSC_FREQ_CPUID_15 && l && l->a && l->b &&
        crystal) {
        EXPECT_EQ(crystal * l->b / l->a, tsc, "");
    }

    END_TEST;
}

BEGIN_TEST_CASE(x86_feature_tests)
RUN_TEST(test_cpuid_cache_matches_live);
RUN_TEST(test_identification);
RUN_TEST(test_uarch_table);
RUN_TEST(test_apic_id_decode);
RUN_TEST(test_clocks);
END_TEST_CASE(x86_feature_tests)
