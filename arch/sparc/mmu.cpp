/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
// TODO: remove this temporary override once ARCH_HAS_MMU is wired up for sparc
#define ARCH_HAS_MMU 1
#if ARCH_HAS_MMU

#include <arch/aspace.h>
#include <arch/sparc.h>
#include <arch/sparc/mmu.h>

#include <arch/mmu.h>
#include <assert.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/trace.h>

#define LOCAL_TRACE 0

namespace {

// 256 entries is a reasonable maximum for the number of contexts on a SPARC v8 system.
// NOTE: some may be less
constexpr uint32_t CTX_TABLE_SIZE = 256;
// align the ctx_table to its natural size.
__ALIGNED(CTX_TABLE_SIZE * 4) srmmu_pte_t ctx_table[CTX_TABLE_SIZE];

bool is_ptd(srmmu_pte_t entry) {
    return (entry & SRMMU_ET_MASK) == SRMMU_ET_PTD;
}

bool is_pte(srmmu_pte_t entry) {
    return (entry & SRMMU_ET_MASK) == SRMMU_ET_PTE;
}

uint mmu_flags_from_pte(srmmu_pte_t pte) {
    uint f = 0;
    if (!(pte & SRMMU_PTE_C)) {
        f |= ARCH_MMU_FLAG_UNCACHED;
    }
    // TODO: finish
    return f;
}

} // namespace

void arch_mmu_early_init() {
    // save a copy of the ctx table that was set up by firmware.
    uint64_t ctx_table_phys = SRMMU_PTE_PADDR(sparc_srmmu_read_reg(SRMMU_REG_CTX_TABLE_PTR));
    for (size_t i = 0; i < countof(ctx_table); i++) {
        ctx_table[i] = sparc_read_physical_32(ctx_table_phys + i * sizeof(srmmu_pte_t));
    }

    // switch to it
    sparc_srmmu_write_reg(SRMMU_REG_CTX_TABLE_PTR, SRMMU_MAKE_CTX_TABLE_PTR((paddr_t)ctx_table));

    // TODO: TLB flush here?
}

void arch_mmu_init() {
    dprintf(INFO, "SRMMU control 0x%08x\n", sparc_srmmu_read_reg(SRMMU_REG_CONTROL));
    dprintf(INFO, "SRMMU context 0x%08x\n", sparc_srmmu_read_reg(SRMMU_REG_CONTEXT));
}

// public api

status_t arch_mmu_init_aspace(arch_aspace_t *aspace, vaddr_t base, size_t size, uint flags) {
    LTRACEF("aspace %p, base %#lx, size %#zx, flags %#x\n", aspace, base, size, flags);

    DEBUG_ASSERT(aspace);

    return ERR_NOT_IMPLEMENTED;
}

status_t arch_mmu_destroy_aspace(arch_aspace_t *aspace) {
    LTRACEF("aspace %p\n", aspace);

    DEBUG_ASSERT(aspace);
    DEBUG_ASSERT(aspace->magic == SPARC_ASPACE_MAGIC);

    return ERR_NOT_IMPLEMENTED;
}

int arch_mmu_map(arch_aspace_t *aspace, vaddr_t vaddr, paddr_t paddr, uint count, uint flags) {
    LTRACEF("vaddr %#lx paddr %#lx count %u flags %#x\n", vaddr, paddr, count, flags);

    DEBUG_ASSERT(aspace);
    DEBUG_ASSERT(aspace->magic == SPARC_ASPACE_MAGIC);

    return ERR_NOT_IMPLEMENTED;
}

int arch_mmu_unmap(arch_aspace_t *aspace, vaddr_t vaddr, uint count) {
    LTRACEF("vaddr %#lx count %u\n", vaddr, count);

    DEBUG_ASSERT(aspace);
    DEBUG_ASSERT(aspace->magic == SPARC_ASPACE_MAGIC);

    return ERR_NOT_IMPLEMENTED;
}

status_t arch_mmu_query(arch_aspace_t *aspace, vaddr_t vaddr, paddr_t *paddr, uint *flags) {
    LTRACEF("aspace %p, vaddr %#lx, paddr %p, flags %p\n", aspace, vaddr, paddr, flags);

    DEBUG_ASSERT(aspace);
    DEBUG_ASSERT(aspace->magic == SPARC_ASPACE_MAGIC);

    // Context Table -> Level 1 table
    srmmu_pte_t entry = ctx_table[aspace->context];
    if (!is_ptd(entry)) {
        return ERR_NOT_FOUND;
    }

    // Level 1 table walk
    srmmu_pte_t *l1_table = (srmmu_pte_t *)(uintptr_t)SRMMU_PTD_PADDR(entry);
    uint32_t idx1 = SRMMU_VA_INDEX1(vaddr);
    entry = l1_table[idx1];

    if (is_pte(entry)) {
        if (paddr) {
            *paddr = SRMMU_PTE_PADDR(entry) | (vaddr & (SRMMU_L1_MAP_SIZE - 1));
        }
        if (flags) {
            *flags = mmu_flags_from_pte(entry);
        }
        return NO_ERROR;
    }
    if (!is_ptd(entry)) {
        return ERR_NOT_FOUND;
    }

    // Level 2 table walk
    srmmu_pte_t *l2_table = (srmmu_pte_t *)(uintptr_t)SRMMU_PTD_PADDR(entry);
    uint32_t idx2 = SRMMU_VA_INDEX2(vaddr);
    entry = l2_table[idx2];

    if (is_pte(entry)) {
        if (paddr) {
            *paddr = SRMMU_PTE_PADDR(entry) | (vaddr & (SRMMU_L2_MAP_SIZE - 1));
        }
        if (flags) {
            *flags = mmu_flags_from_pte(entry);
        }
        return NO_ERROR;
    }
    if (!is_ptd(entry)) {
        return ERR_NOT_FOUND;
    }

    // Level 3 table walk
    srmmu_pte_t *l3_table = (srmmu_pte_t *)(uintptr_t)SRMMU_PTD_PADDR(entry);
    uint32_t idx3 = SRMMU_VA_INDEX3(vaddr);
    entry = l3_table[idx3];

    if (!is_pte(entry)) {
        return ERR_NOT_FOUND;
    }

    if (paddr) {
        *paddr = SRMMU_PTE_PADDR(entry) | (vaddr & (SRMMU_L3_MAP_SIZE - 1));
    }
    if (flags) {
        *flags = mmu_flags_from_pte(entry);
    }

    return NO_ERROR;
}

void arch_mmu_context_switch(arch_aspace_t *aspace) {
    LTRACEF("aspace %p\n", aspace);
}

bool arch_mmu_supports_nx_mappings(void) {
    return false;
}

bool arch_mmu_supports_ns_mappings(void) {
    return false;
}

bool arch_mmu_supports_user_aspaces(void) {
    return false;
}

#endif
