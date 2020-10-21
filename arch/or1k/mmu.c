/*
 * Copyright (c) 2015 Stefan Kristiansson
 * Based on arch/arm/arm/mmu.c
 * Copyright (c) 2008-2014 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/trace.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <assert.h>
#include <string.h>
#include <arch/mmu.h>
#include <arch/or1k.h>
#include <arch/or1k/mmu.h>
#include <kernel/vm.h>

#define LOCAL_TRACE 0

#if WITH_KERNEL_VM

uint32_t or1k_kernel_translation_table[256] __ALIGNED(8192) __SECTION(".bss.prebss.translation_table");

/* Pessimistic tlb invalidation, which rather invalidate too much.
 * TODO: make it more precise. */
void or1k_invalidate_tlb(vaddr_t vaddr, uint count) {
    uint32_t dmmucfgr = mfspr(OR1K_SPR_SYS_DMMUCFGR_ADDR);
    uint32_t immucfgr = mfspr(OR1K_SPR_SYS_IMMUCFGR_ADDR);
    uint32_t num_dtlb_ways = OR1K_SPR_SYS_DMMUCFGR_NTW_GET(dmmucfgr) + 1;
    uint32_t num_dtlb_sets = 1 << OR1K_SPR_SYS_DMMUCFGR_NTS_GET(dmmucfgr);
    uint32_t num_itlb_ways = OR1K_SPR_SYS_IMMUCFGR_NTW_GET(immucfgr) + 1;
    uint32_t num_itlb_sets = 1 << OR1K_SPR_SYS_IMMUCFGR_NTS_GET(immucfgr);
    uint32_t offs;

    for (; count; count--) {
        offs = (vaddr >> PAGE_SIZE_SHIFT) & (num_dtlb_sets-1);
        switch (num_dtlb_ways) {
            case 4:
                mtspr_off(0, OR1K_SPR_DMMU_DTLBW_MR_ADDR(3, offs), 0);
                // fallthrough
            case 3:
                mtspr_off(0, OR1K_SPR_DMMU_DTLBW_MR_ADDR(2, offs), 0);
                // fallthrough
            case 2:
                mtspr_off(0, OR1K_SPR_DMMU_DTLBW_MR_ADDR(1, offs), 0);
                // fallthrough
            case 1:
                mtspr_off(0, OR1K_SPR_DMMU_DTLBW_MR_ADDR(0, offs), 0);
        }

        offs = (vaddr >> PAGE_SIZE_SHIFT) & (num_itlb_sets-1);
        switch (num_itlb_ways) {
            case 4:
                mtspr_off(0, OR1K_SPR_IMMU_ITLBW_MR_ADDR(3, offs), 0);
                // fallthrough
            case 3:
                mtspr_off(0, OR1K_SPR_IMMU_ITLBW_MR_ADDR(2, offs), 0);
                // fallthrough
            case 2:
                mtspr_off(0, OR1K_SPR_IMMU_ITLBW_MR_ADDR(1, offs), 0);
                // fallthrough
            case 1:
                mtspr_off(0, OR1K_SPR_IMMU_ITLBW_MR_ADDR(0, offs), 0);
        }
        vaddr += PAGE_SIZE;
    }
}

status_t arch_mmu_query(arch_aspace_t *aspace, vaddr_t vaddr, paddr_t *paddr, uint *flags) {
    uint index = vaddr / SECTION_SIZE;
    uint32_t pte = or1k_kernel_translation_table[index];
    uint32_t vmask = SECTION_SIZE-1;

    if (!(pte & OR1K_MMU_PG_PRESENT))
        return ERR_NOT_FOUND;

    /* not a l1 entry */
    if (!(pte & OR1K_MMU_PG_L)) {
        uint32_t *l2_table = paddr_to_kvaddr(pte & ~OR1K_MMU_PG_FLAGS_MASK);
        index = (vaddr % SECTION_SIZE) / PAGE_SIZE;
        pte = l2_table[index];
        vmask = PAGE_SIZE-1;
    }

    if (paddr)
        *paddr = (pte & ~OR1K_MMU_PG_FLAGS_MASK) | (vaddr & vmask);

    if (flags) {
        *flags = 0;
        if (pte & OR1K_MMU_PG_U)
            *flags |= ARCH_MMU_FLAG_PERM_USER;
        if (!(pte & OR1K_MMU_PG_X))
            *flags |= ARCH_MMU_FLAG_PERM_NO_EXECUTE;
        if (!(pte & OR1K_MMU_PG_W))
            *flags |= ARCH_MMU_FLAG_PERM_RO;
        if (pte & OR1K_MMU_PG_CI)
            *flags |= ARCH_MMU_FLAG_UNCACHED;
    }

    return NO_ERROR;
}

int arch_mmu_unmap(arch_aspace_t *aspace, vaddr_t vaddr, uint count) {
    LTRACEF("vaddr = 0x%lx, count = %d\n", vaddr, count);

    if (!IS_PAGE_ALIGNED(vaddr))
        return ERR_INVALID_ARGS;

    uint unmapped = 0;
    while (count) {
        uint index = vaddr / SECTION_SIZE;
        uint32_t pte = or1k_kernel_translation_table[index];
        if (!(pte & OR1K_MMU_PG_PRESENT)) {
            vaddr += PAGE_SIZE;
            count--;
            continue;
        }
        /* Unmapping of l2 tables is not implemented (yet) */
        if (!(pte & OR1K_MMU_PG_L) || !IS_ALIGNED(vaddr, SECTION_SIZE) || count < SECTION_SIZE / PAGE_SIZE)
            PANIC_UNIMPLEMENTED;

        or1k_kernel_translation_table[index] = 0;
        or1k_invalidate_tlb(vaddr, SECTION_SIZE / PAGE_SIZE);
        vaddr += SECTION_SIZE;
        count -= SECTION_SIZE / PAGE_SIZE;
        unmapped += SECTION_SIZE / PAGE_SIZE;
    }

    return unmapped;
}

int arch_mmu_map(arch_aspace_t *aspace, vaddr_t vaddr, paddr_t paddr, uint count, uint flags) {
    uint l1_index;
    uint32_t pte;
    uint32_t arch_flags = 0;

    LTRACEF("vaddr = 0x%lx, paddr = 0x%lx, count = %d, flags = 0x%x\n", vaddr, paddr, count, flags);

    if (!IS_PAGE_ALIGNED(vaddr) || !IS_PAGE_ALIGNED(paddr))
        return ERR_INVALID_ARGS;

    if (flags & ARCH_MMU_FLAG_PERM_USER)
        arch_flags |= OR1K_MMU_PG_U;
    if (!(flags & ARCH_MMU_FLAG_PERM_NO_EXECUTE))
        arch_flags |= OR1K_MMU_PG_X;
    if (flags & ARCH_MMU_FLAG_CACHE_MASK)
        arch_flags |= OR1K_MMU_PG_CI;
    if (!(flags & ARCH_MMU_FLAG_PERM_RO))
        arch_flags |= OR1K_MMU_PG_W;

    uint mapped = 0;
    while (count) {
        l1_index = vaddr / SECTION_SIZE;
        if (IS_ALIGNED(vaddr, SECTION_SIZE) && IS_ALIGNED(paddr, SECTION_SIZE) && count >= SECTION_SIZE / PAGE_SIZE) {
            or1k_kernel_translation_table[l1_index] = (paddr & ~(SECTION_SIZE-1)) | arch_flags | OR1K_MMU_PG_PRESENT | OR1K_MMU_PG_L;
            count -= SECTION_SIZE / PAGE_SIZE;
            mapped += SECTION_SIZE / PAGE_SIZE;
            vaddr += SECTION_SIZE;
            paddr += SECTION_SIZE;
            continue;
        }

        uint32_t *l2_table;

        pte = or1k_kernel_translation_table[l1_index];

        /* FIXME: l1 already mapped as a section */
        if (pte & OR1K_MMU_PG_PRESENT && pte & OR1K_MMU_PG_L)
            PANIC_UNIMPLEMENTED;

        if (pte & OR1K_MMU_PG_PRESENT) {
            l2_table = paddr_to_kvaddr(pte & ~OR1K_MMU_PG_FLAGS_MASK);
            LTRACEF("l2_table at %p\n", l2_table);
        } else {
            l2_table = pmm_alloc_kpage();
            if (!l2_table) {
                TRACEF("failed to allocate pagetable\n");
                return mapped;
            }

            memset(l2_table, 0, PAGE_SIZE);
            paddr_t l2_pa = vaddr_to_paddr(l2_table);
            LTRACEF("allocated pagetable at %p, pa 0x%lx\n", l2_table, l2_pa);
            or1k_kernel_translation_table[l1_index] = l2_pa | arch_flags | OR1K_MMU_PG_PRESENT;
        }

        uint l2_index = (vaddr % SECTION_SIZE) / PAGE_SIZE;

        LTRACEF("l2_index = 0x%x, vaddr = 0x%lx, paddr = 0x%lx\n", l2_index, vaddr, paddr);
        l2_table[l2_index] = paddr | arch_flags | OR1K_MMU_PG_PRESENT | OR1K_MMU_PG_L;

        count--;
        mapped++;
        vaddr += PAGE_SIZE;
        paddr += PAGE_SIZE;
    }

    return mapped;
}

// initialize per address space
status_t arch_mmu_init_aspace(arch_aspace_t *aspace, vaddr_t base, size_t size, uint flags) {
    LTRACEF("aspace %p, base %#lx, size %#zx, flags %#x\n", aspace, base, size, flags);

    DEBUG_ASSERT(aspace);

    // validate that the base + size is sane and doesn't wrap
    DEBUG_ASSERT(size > PAGE_SIZE);
    DEBUG_ASSERT(base + size - 1 > base);

#if 0
    aspace->flags = flags;
    if (flags & ARCH_ASPACE_FLAG_KERNEL) {
        // at the moment we can only deal with address spaces as globally defined
        DEBUG_ASSERT(base == KERNEL_ASPACE_BASE);
        DEBUG_ASSERT(size == KERNEL_ASPACE_SIZE);

        aspace->base = base;
        aspace->size = size;
        aspace->pt_virt = kernel_pgtable;
        aspace->pt_phys = kernel_pgtable_phys;
    } else {
        PANIC_UNIMPLEMENTED;
    }

    LTRACEF("pt phys %#lx, pt virt %p\n", aspace->pt_phys, aspace->pt_virt);
#endif

    return NO_ERROR;
}

status_t arch_mmu_destroy_aspace(arch_aspace_t *aspace) {
    LTRACEF("aspace %p\n", aspace);

    PANIC_UNIMPLEMENTED;
}

// load a new user address space context.
// aspace argument NULL should load kernel-only context
void arch_mmu_context_switch(arch_aspace_t *aspace) {
    LTRACEF("aspace %p\n", aspace);

    PANIC_UNIMPLEMENTED;
}

#endif /* WITH_KERNEL_VM */
