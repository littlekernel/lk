/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <arch/ops.h>
#include <arch/x86.h>
#include <arch/x86/feature.h>
#include <arch/x86/mtrr.h>
#include <inttypes.h>
#include <lk/err.h>
#include <lk/pow2.h>
#include <lk/trace.h>

// Basic routines to manage MTRRs per cpu.

// TODO: allow run time synchronization of MTRR changes across cpus by
// doing cross-cpu IPIs to coordinate changes and cache flushes.
// For now we just set them up on the boot cpu and copy the setup to
// the secondary cpus at start time, and then leave them alone after that.

#define LOCAL_TRACE                 0
#define X86_MTRR_MAX_VARIABLE_MTRRS 256
#define X86_MTRR_FIXED_MSR_COUNT    11

static const uint32_t x86_mtrr_fixed_msr_ids[X86_MTRR_FIXED_MSR_COUNT] = {
    X86_MSR_IA32_MTRR_FIX64K_00000,
    X86_MSR_IA32_MTRR_FIX16K_80000,
    X86_MSR_IA32_MTRR_FIX16K_A0000,
    X86_MSR_IA32_MTRR_FIX4K_C0000,
    X86_MSR_IA32_MTRR_FIX4K_C0000 + 1,
    X86_MSR_IA32_MTRR_FIX4K_C0000 + 2,
    X86_MSR_IA32_MTRR_FIX4K_C0000 + 3,
    X86_MSR_IA32_MTRR_FIX4K_C0000 + 4,
    X86_MSR_IA32_MTRR_FIX4K_C0000 + 5,
    X86_MSR_IA32_MTRR_FIX4K_C0000 + 6,
    X86_MSR_IA32_MTRR_FIX4K_F8000,
};

typedef struct x86_mtrr_saved_state {
    bool valid;
    uint64_t mtrrcap;
    uint64_t def_type;
    uint32_t variable_count;
    uint64_t variable_base[X86_MTRR_MAX_VARIABLE_MTRRS];
    uint64_t variable_mask[X86_MTRR_MAX_VARIABLE_MTRRS];
    uint64_t fixed[X86_MTRR_FIXED_MSR_COUNT];
} x86_mtrr_saved_state_t;

static x86_mtrr_saved_state_t x86_mtrr_saved_state;

static inline void flush_cache(void) {
    __asm__ volatile("wbinvd" ::: "memory");
}

static void get_mtrr_addresses(int index, uint32_t *base_msr, uint32_t *mask_msr) {
    *base_msr = 0x200 + (index * 2);
    *mask_msr = 0x201 + (index * 2);
}

static uint32_t x86_mtrr_clamp_variable_count(uint32_t count) {
    if (count > X86_MTRR_MAX_VARIABLE_MTRRS) {
        return X86_MTRR_MAX_VARIABLE_MTRRS;
    }
    return count;
}

static void x86_mtrr_save_state(void) {
    if (!x86_mtrr_supported()) {
        return;
    }

    x86_mtrr_saved_state.mtrrcap = read_msr(X86_MSR_IA32_MTRRCAP);
    x86_mtrr_saved_state.def_type = read_msr(X86_MSR_IA32_MTRR_DEF_TYPE);
    x86_mtrr_saved_state.variable_count =
        x86_mtrr_clamp_variable_count((uint32_t)(x86_mtrr_saved_state.mtrrcap & 0xFF));

    for (uint32_t i = 0; i < x86_mtrr_saved_state.variable_count; ++i) {
        uint32_t base_msr, mask_msr;
        get_mtrr_addresses((int)i, &base_msr, &mask_msr);
        x86_mtrr_saved_state.variable_base[i] = read_msr(base_msr);
        x86_mtrr_saved_state.variable_mask[i] = read_msr(mask_msr);
    }

    if (x86_mtrr_saved_state.mtrrcap & (1ULL << 8)) {
        for (uint32_t i = 0; i < X86_MTRR_FIXED_MSR_COUNT; ++i) {
            x86_mtrr_saved_state.fixed[i] = read_msr(x86_mtrr_fixed_msr_ids[i]);
        }
    }

    x86_mtrr_saved_state.valid = true;
}

static void x86_mtrr_restore_state(void) {
    if (!x86_mtrr_supported() || !x86_mtrr_saved_state.valid) {
        return;
    }

    flush_cache();

    const uint64_t def_type_disabled =
        x86_mtrr_saved_state.def_type & ~(MTRR_DEF_TYPE_ENABLE | MTRR_DEF_TYPE_FIX_ENABLE);
    write_msr(X86_MSR_IA32_MTRR_DEF_TYPE, def_type_disabled);

    for (uint32_t i = 0; i < x86_mtrr_saved_state.variable_count; ++i) {
        uint32_t base_msr, mask_msr;
        get_mtrr_addresses((int)i, &base_msr, &mask_msr);
        write_msr(mask_msr, 0);
    }

    if (x86_mtrr_saved_state.mtrrcap & (1ULL << 8)) {
        for (uint32_t i = 0; i < X86_MTRR_FIXED_MSR_COUNT; ++i) {
            write_msr(x86_mtrr_fixed_msr_ids[i], x86_mtrr_saved_state.fixed[i]);
        }
    }

    for (uint32_t i = 0; i < x86_mtrr_saved_state.variable_count; ++i) {
        uint32_t base_msr, mask_msr;
        get_mtrr_addresses((int)i, &base_msr, &mask_msr);
        write_msr(base_msr, x86_mtrr_saved_state.variable_base[i]);
        write_msr(mask_msr, x86_mtrr_saved_state.variable_mask[i]);
    }

    write_msr(X86_MSR_IA32_MTRR_DEF_TYPE, x86_mtrr_saved_state.def_type);

    flush_cache();

    uintptr_t cr3;
    __asm__ volatile("mov %%cr3, %0\n\tmov %0, %%cr3" : "=r"(cr3)::"memory");
}

static const char *x86_mtrr_type_name(uint8_t type) {
    switch (type) {
        case MTRR_TYPE_UC:
            return "UC";
        case MTRR_TYPE_WC:
            return "WC";
        case MTRR_TYPE_WT:
            return "WT";
        case MTRR_TYPE_WP:
            return "WP";
        case MTRR_TYPE_WB:
            return "WB";
        default:
            return "UNK";
    }
}

bool x86_mtrr_supported(void) {
    return x86_feature_test(X86_FEATURE_MTRR);
}

int x86_mtrr_count(void) {
    if (!x86_mtrr_supported()) {
        return 0;
    }

    uint64_t mtrrcap = read_msr(X86_MSR_IA32_MTRRCAP);
    return (int)(mtrrcap & 0xFF);
}

status_t x86_mtrr_set(int mtrr_index, uint64_t phys_base, uint64_t size, uint8_t type) {
    DEBUG_ASSERT(arch_curr_cpu_num() == 0); /* only allow on boot CPU to avoid cpu synchronization issues for now */

    if (!x86_mtrr_supported()) {
        TRACEF("ERROR: MTRR not supported on this CPU\n");
        return ERR_NOT_SUPPORTED;
    }

    if (mtrr_index < 0 || mtrr_index >= x86_mtrr_count()) {
        TRACEF("ERROR: invalid MTRR index %d (max %d)\n", mtrr_index, x86_mtrr_count() - 1);
        return ERR_OUT_OF_RANGE;
    }

    /* Check that size is a power of 2 */
    if (!ispow2(size)) {
        TRACEF("ERROR: size %#" PRIx64 " is not a power of 2\n", size);
        return ERR_INVALID_ARGS;
    }

    /* Validate parameters */
    if (!IS_ALIGNED(phys_base, size)) {
        TRACEF("ERROR: base %#" PRIx64 " not aligned to size %#" PRIx64 "\n",
               phys_base, size);
        return ERR_INVALID_ARGS;
    }

    uint32_t base_msr, mask_msr;
    get_mtrr_addresses(mtrr_index, &base_msr, &mask_msr);

    LTRACEF("Setting MTRR%d: base=%#" PRIx64 " size=%#" PRIx64 " type=%d\n",
            mtrr_index, phys_base, size, type);

    /* Intel SDM Vol. 3 Section 11.11.8: MTRR Initialization Sequence
     * This is CRITICAL for correct operation on real hardware
     */

    /* Step 1: Flush all caches */
    flush_cache();

    /* Step 2: Disable MTRRs temporarily */
    uint64_t def_type = read_msr(X86_MSR_IA32_MTRR_DEF_TYPE);
    uint64_t def_type_disabled = def_type & ~(MTRR_DEF_TYPE_ENABLE | MTRR_DEF_TYPE_FIX_ENABLE);

    write_msr(X86_MSR_IA32_MTRR_DEF_TYPE, def_type_disabled);

    /* Step 3: Clear the PHYSMASK valid bit first */
    write_msr(mask_msr, 0);

    /* Step 4: Program PHYSBASE with base address and type */
    uint64_t base_value = (phys_base & 0xFFFFFFF000ULL) | type;
    write_msr(base_msr, base_value);

    /* Step 5: Program PHYSMASK with the size mask and enable */
    /* The mask is (size - 1) inverted, shifted to align with address bits.
     * For a 64MB region, size = 0x4000000, mask bits should be 0x3FFFFFF
     * But PHYSMASK expects the negation of the region size
     */
    uint64_t mask_bits = (~(size - 1)) & 0xFFFFFFF000ULL;
    uint64_t mask_value = mask_bits | MTRR_PHYSMASK_VALID;
    write_msr(mask_msr, mask_value);

    /* Step 6: Re-enable MTRRs */
    write_msr(X86_MSR_IA32_MTRR_DEF_TYPE, def_type);

    /* Step 7: Flush all caches again after MTRR update */
    flush_cache();

    /* Step 8: Flush TLB to invalidate all entries
     * Use a dummy invlpg to flush all TLBs on modern CPUs.
     */
    // TODO: use common TLB shootdown code when implemented
    uintptr_t cr3;
    __asm__ volatile("mov %%cr3, %0\n\tmov %0, %%cr3" : "=r"(cr3)::"memory");

    dprintf(INFO, "MTRR: MTRR%d configured: PHYSBASE=%#" PRIx64 " PHYSMASK=%#" PRIx64 "\n",
            mtrr_index, base_value, mask_value);

    x86_mtrr_save_state();

    return NO_ERROR;
}

status_t x86_mtrr_set_framebuffer(uint64_t fb_phys_addr, uint64_t fb_size) {
    if (!x86_mtrr_supported()) {
        TRACEF("MTRR not supported, framebuffer performance may be degraded\n");
        return ERR_NOT_SUPPORTED;
    }

    int num_mtrrs = x86_mtrr_count();
    if (num_mtrrs <= 0) {
        TRACEF("ERROR: No variable MTRRs available\n");
        return ERR_NO_RESOURCES;
    }

    dprintf(INFO, "MTRR: Framebuffer base=%#" PRIx64 " size=%#" PRIx64 " (%" PRIu64 " MB)\n",
            fb_phys_addr, fb_size, fb_size / (1024 * 1024));

    for (int i = 0; i < num_mtrrs; i++) {
        uint32_t base_msr, mask_msr;
        get_mtrr_addresses(i, &base_msr, &mask_msr);

        uint64_t mask_val = read_msr(mask_msr);
        if (!(mask_val & MTRR_PHYSMASK_VALID)) {
            dprintf(INFO, "MTRR: Using MTRR%d for framebuffer\n", i);
            return x86_mtrr_set(i, fb_phys_addr, fb_size, MTRR_TYPE_WC);
        }
    }

    TRACEF("All variable MTRRs are in use\n");
    return ERR_NO_RESOURCES;
}

void x86_mtrr_dump(void) {
    if (!x86_mtrr_supported()) {
        dprintf(INFO, "MTRR: unsupported on this CPU\n");
        return;
    }

    const uint64_t mtrrcap = read_msr(X86_MSR_IA32_MTRRCAP);
    const uint64_t def_type = read_msr(X86_MSR_IA32_MTRR_DEF_TYPE);
    const int count = (int)(mtrrcap & 0xFF);

    dprintf(INFO,
            "MTRR: cap=%#" PRIx64 " var=%d fix=%u wc=%u smrr=%u\n",
            mtrrcap,
            count,
            (unsigned int)((mtrrcap >> 8) & 0x1),
            (unsigned int)((mtrrcap >> 10) & 0x1),
            (unsigned int)((mtrrcap >> 11) & 0x1));

    dprintf(INFO,
            "MTRR: def_type=%#" PRIx64 " enabled=%u fixed_enabled=%u default=%u(%s)\n",
            def_type,
            (unsigned int)((def_type & MTRR_DEF_TYPE_ENABLE) ? 1 : 0),
            (unsigned int)((def_type & MTRR_DEF_TYPE_FIX_ENABLE) ? 1 : 0),
            (unsigned int)(def_type & MTRR_DEF_TYPE_TYPE_MASK),
            x86_mtrr_type_name((uint8_t)(def_type & MTRR_DEF_TYPE_TYPE_MASK)));

    if (mtrrcap & (1ULL << 8)) {
        dprintf(INFO, "MTRR: fixed ranges\n");
        for (int i = 0; i < X86_MTRR_FIXED_MSR_COUNT; ++i) {
            const uint64_t fixed = read_msr(x86_mtrr_fixed_msr_ids[i]);
            dprintf(INFO,
                    "MTRR: FIX[%d] msr=%#x val=%#018" PRIx64 "\n",
                    i,
                    x86_mtrr_fixed_msr_ids[i],
                    fixed);
        }
    } else {
        dprintf(INFO, "MTRR: fixed ranges unsupported\n");
    }

    for (int i = 0; i < count; ++i) {
        uint32_t base_msr, mask_msr;
        get_mtrr_addresses(i, &base_msr, &mask_msr);

        const uint64_t base = read_msr(base_msr);
        const uint64_t mask = read_msr(mask_msr);
        const bool valid = (mask & MTRR_PHYSMASK_VALID) != 0;
        const uint8_t type = (uint8_t)(base & MTRR_PHYSBASE_TYPE_MASK);

        if (!valid) {
            dprintf(INFO,
                    "MTRR: [%d] disabled base=%#" PRIx64 " mask=%#" PRIx64 "\n",
                    i,
                    base,
                    mask);
            continue;
        }

        const uint64_t base_addr = base & 0xFFFFFFF000ULL;
        const uint64_t mask_addr = mask & 0xFFFFFFF000ULL;
        const uint64_t size = (~mask_addr & 0xFFFFFFF000ULL) + 0x1000ULL;

        dprintf(INFO,
                "MTRR: [%d] base=%#018" PRIx64 " mask=%#018" PRIx64
                " addr=%#" PRIx64 " size=%#" PRIx64 " type=%u(%s)\n",
                i,
                base,
                mask,
                base_addr,
                size,
                (unsigned int)type,
                x86_mtrr_type_name(type));
    }
}

// Very early initialization on the first cpu
void x86_mtrr_early_init(void) {
    x86_mtrr_save_state();
}

// Called on every cpu, including the first
void x86_mtrr_early_init_percpu(void) {
    if (!x86_mtrr_saved_state.valid) {
        return;
    }

    uint cpu = arch_curr_cpu_num();
    if (cpu == 0) {
        return;
    }

    x86_mtrr_restore_state();
    dprintf(INFO, "MTRR: cpu %u restored saved MTRR state\n", cpu);
}

void x86_mtrr_init(void) {
    if (LK_DEBUGLEVEL >= 2) {
        x86_mtrr_dump();
    }
}
