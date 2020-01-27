/*
 * Copyright (c) 2020 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#if RISCV_MMU

#include "arch/riscv/mmu.h"

#include <assert.h>
#include <string.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/trace.h>
#include <arch/ops.h>
#include <arch/mmu.h>
#include <arch/riscv.h>
#include <arch/riscv/csr.h>
#include <kernel/vm.h>

#define LOCAL_TRACE 0

#include <kernel/vm.h>

#if __riscv_xlen == 32
#error "32 bit mmu not supported yet"
#endif

riscv_pte_t kernel_pgtable[512] __ALIGNED(PAGE_SIZE);
paddr_t kernel_pgtable_phys; // filled in by start.S

// initial memory mappings. VM uses to construct mappings after the fact
struct mmu_initial_mapping mmu_initial_mappings[] = {
    // all of memory, mapped in start.S
    {
        .phys = 0,
        .virt = KERNEL_ASPACE_BASE,
#if RISCV_MMU == 48
        .size = 512UL * GB,
#elif RISCV_MMU == 39
        .size = 64UL * GB,
#else
#error implement
#endif
        .flags = 0,
        .name = "memory"
    },

    // null entry to terminate the list
    { 0 }
};

static inline void riscv_set_satp(uint asid, paddr_t pt) {
    ulong satp;

#if RISCV_MMU == 48
    satp = RISCV_SATP_MODE_SV48;
#elif RISCV_MMU == 39
    satp = RISCV_SATP_MODE_SV39;
#endif

    // make sure the asid is in range
    DEBUG_ASSERT((asid & RISCV_SATP_ASID_MASK) == 0);
    satp |= (ulong)asid << RISCV_SATP_ASID_SHIFT;

    // make sure the page table is aligned
    DEBUG_ASSERT(IS_PAGE_ALIGNED(pt));
    satp |= pt;

    riscv_csr_write(RISCV_CSR_SATP, satp);

    // TODO: TLB flush here or use asid properly
    // sfence.vma zero, zero
}

// given a va address and the level, compute the index in the current PT
static inline uint vaddr_to_index(vaddr_t va, uint level) {
    // levels count down from PT_LEVELS - 1
    DEBUG_ASSERT(level < RISCV_MMU_PT_LEVELS);

    // canonicalize the address
    va &= RISCV_MMU_CANONICAL_MASK;

    uint index = ((va >> PAGE_SIZE_SHIFT) >> (level * RISCV_MMU_PT_SHIFT)) & (RISCV_MMU_PT_ENTRIES - 1);
    LTRACEF_LEVEL(3, "canonical va %#lx, level %u = index %#x\n", va, level, index);

    return index;
}

static uintptr_t page_size_per_level(uint level) {
    // levels count down from PT_LEVELS - 1
    DEBUG_ASSERT(level < RISCV_MMU_PT_LEVELS);

    return 1UL << (PAGE_SIZE_SHIFT + level * RISCV_MMU_PT_SHIFT);
}

static uintptr_t page_mask_per_level(uint level) {
    return page_size_per_level(level) - 1;
}

static volatile riscv_pte_t *alloc_ptable(paddr_t *pa) {
    // grab a page from the pmm
    vm_page_t *p = pmm_alloc_page();
    if (!p) {
        return NULL;
    }

    // get the physical and virtual mappings of the page
    *pa = vm_page_to_paddr(p);
    riscv_pte_t *pte = paddr_to_kvaddr(*pa);

    // zero it out
    memset(pte, 0, PAGE_SIZE);

    smp_wmb();

    LTRACEF_LEVEL(3, "returning pa %#lx, va %p\n", *pa, pte);
    return pte;
}

static riscv_pte_t mmu_flags_to_pte(uint flags) {
    riscv_pte_t pte = 0;

    pte |= (flags & ARCH_MMU_FLAG_PERM_USER) ? RISCV_PTE_U : 0;
    pte |= (flags & ARCH_MMU_FLAG_PERM_RO) ? RISCV_PTE_R : (RISCV_PTE_R | RISCV_PTE_W);
    pte |= (flags & ARCH_MMU_FLAG_PERM_NO_EXECUTE) ? 0 : RISCV_PTE_X;

    return pte;
}

static uint pte_flags_to_mmu_flags(riscv_pte_t pte) {
    uint f = 0;
    if ((pte & (RISCV_PTE_R | RISCV_PTE_W)) == RISCV_PTE_R) {
        f |= ARCH_MMU_FLAG_PERM_RO;
    }
    f |= (pte & RISCV_PTE_X) ? 0 : ARCH_MMU_FLAG_PERM_NO_EXECUTE;
    f |= (pte & RISCV_PTE_U) ? ARCH_MMU_FLAG_PERM_USER : 0;
    return f;
}

// public api

// initialize per address space
status_t arch_mmu_init_aspace(arch_aspace_t *aspace, vaddr_t base, size_t size, uint flags) {
    LTRACEF("aspace %p, base %#lx, size %#zx, flags %#x\n", aspace, base, size, flags);

    DEBUG_ASSERT(aspace);

    // validate that the base + size is sane and doesn't wrap
    DEBUG_ASSERT(size > PAGE_SIZE);
    DEBUG_ASSERT(base + size - 1 > base);

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

    return NO_ERROR;
}
status_t arch_mmu_destroy_aspace(arch_aspace_t *aspace) {
    LTRACEF("aspace %p\n", aspace);

    PANIC_UNIMPLEMENTED;
}

// routines to map/unmap/query mappings per address space
int arch_mmu_map(arch_aspace_t *aspace, vaddr_t vaddr, paddr_t paddr, uint count, const uint flags) {
    LTRACEF("vaddr %#lx paddr %#lx count %u flags %#x\n", vaddr, paddr, count, flags);

    DEBUG_ASSERT(aspace);

restart:
    if (count == 0)
        return NO_ERROR;

    // bootstrap the top level walk
    uint level = RISCV_MMU_PT_LEVELS - 1;
    uint index = vaddr_to_index(vaddr, level);
    volatile riscv_pte_t *ptep = aspace->pt_virt + index;

    for (;;) {
        LTRACEF_LEVEL(2, "level %u, index %u, pte %p (%#lx) va %#lx pa %#lx\n",
                      level, index, ptep, *ptep, vaddr, paddr);

        // look at our page table entry
        riscv_pte_t pte = *ptep;
        if (level > 0 && !(pte & RISCV_PTE_V)) {
            // invalid entry, will have to add a page table
            paddr_t ptp;
            volatile riscv_pte_t *ptv = alloc_ptable(&ptp);
            if (!ptv) {
                return ERR_NO_MEMORY;
            }

            LTRACEF_LEVEL(2, "new ptable table %p, pa %#lx\n", ptv, ptp);

            // link it in. RMW == 0 is a page table link
            pte = RISCV_PTE_PPN_TO_PTE(ptp) | RISCV_PTE_V;
            *ptep = pte;

            // go one level deeper
            level--;
            index = vaddr_to_index(vaddr, level);
            ptep = ptv + index;
        } else if ((pte & RISCV_PTE_V) && !(pte & RISCV_PTE_PERM_MASK)) {
            // next level page table pointer (RWX = 0)
            paddr_t ptp = RISCV_PTE_PPN(pte);
            volatile riscv_pte_t *ptv = paddr_to_kvaddr(ptp);

            LTRACEF_LEVEL(2, "next level page table at %p, pa %#lx\n", ptv, ptp);

            // go one level deeper
            level--;
            index = vaddr_to_index(vaddr, level);
            ptep = ptv + index;
        } else if (pte & RISCV_PTE_V) {
            // terminal entry already exists
            if (level > 0) {
                PANIC_UNIMPLEMENTED_MSG("terminal large page entry");
            } else {
                PANIC_UNIMPLEMENTED_MSG("terminal page entry");
            }
        } else {
            DEBUG_ASSERT(level == 0 && !(pte & RISCV_PTE_V));

            // hit a open terminal page table entry, lets add ours
            pte = RISCV_PTE_PPN_TO_PTE(paddr);
            pte |= mmu_flags_to_pte(flags);
            pte |= RISCV_PTE_A | RISCV_PTE_D | RISCV_PTE_V;
            pte |= (aspace->flags & ARCH_ASPACE_FLAG_KERNEL) ? RISCV_PTE_G : 0;

            LTRACEF_LEVEL(2, "added new terminal entry: pte %#lx\n", pte);

            *ptep = pte;

            // simple algorithm: restart walk from top, one page at a time
            // TODO: more efficiently deal with runs and large pages
            count--;
            paddr += PAGE_SIZE;
            vaddr += PAGE_SIZE;
            goto restart;
        }

        // make sure we didn't decrement level one too many
        DEBUG_ASSERT(level < RISCV_MMU_PT_LEVELS);
    }
    // unreachable
}

int arch_mmu_unmap(arch_aspace_t *aspace, vaddr_t vaddr, uint count) {
    LTRACEF("vaddr %#lx count %u\n", vaddr, count);

    PANIC_UNIMPLEMENTED;
}

status_t arch_mmu_query(arch_aspace_t *aspace, const vaddr_t vaddr, paddr_t *paddr, uint *flags) {
    LTRACEF("aspace %p, vaddr %#lx\n", aspace, vaddr);

    DEBUG_ASSERT(aspace);

    // trim the vaddr to the aspace
    if (vaddr < aspace->base || vaddr > aspace->base + aspace->size - 1) {
        return ERR_OUT_OF_RANGE;
    }

    uint level = RISCV_MMU_PT_LEVELS - 1;
    uint index = vaddr_to_index(vaddr, level);
    volatile riscv_pte_t *ptep = aspace->pt_virt + index;

    // walk down through the levels, looking for a terminal entry that matches our address
    for (;;) {
        LTRACEF_LEVEL(2, "level %u, index %u, pte %p (%#lx)\n", level, index, ptep, *ptep);

        // look at our page table entry
        riscv_pte_t pte = *ptep;
        if ((pte & RISCV_PTE_V) == 0) {
            // invalid entry, terminate search
            return ERR_NOT_FOUND;
        } else if ((pte & RISCV_PTE_PERM_MASK) == 0) {
            // next level page table pointer (RWX = 0)
            paddr_t ptp = RISCV_PTE_PPN(pte);
            volatile riscv_pte_t *ptv = paddr_to_kvaddr(ptp);

            LTRACEF_LEVEL(2, "next level page table at %p, pa %#lx\n", ptv, ptp);

            // go one level deeper
            level--;
            index = vaddr_to_index(vaddr, level);
            ptep = ptv + index;
        } else {
            // terminal entry
            LTRACEF_LEVEL(3, "terminal entry\n");

            if (paddr) {
                // extract the ppn
                paddr_t pa = RISCV_PTE_PPN(pte);
                uintptr_t page_mask = page_mask_per_level(level);

                // add the va offset into the physical address
                *paddr = pa | (vaddr & page_mask);
                LTRACEF_LEVEL(3, "raw pa %#lx, page_mask %#lx, final pa %#lx\n", pa, page_mask, *paddr);
            }

            if (flags) {
                // compute the flags
                *flags = pte_flags_to_mmu_flags(pte);
                LTRACEF_LEVEL(3, "computed flags %#x\n", *flags);
            }

            return NO_ERROR;
        }

        // make sure we didn't decrement level one too many
        DEBUG_ASSERT(level < RISCV_MMU_PT_LEVELS);
    }
    // unreachable
}


// load a new user address space context.
// aspace argument NULL should load kernel-only context
void arch_mmu_context_switch(arch_aspace_t *aspace) {
    LTRACEF("aspace %p\n", aspace);

    PANIC_UNIMPLEMENTED;
}

#endif
