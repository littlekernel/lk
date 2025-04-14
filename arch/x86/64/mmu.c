/*
 * Copyright (c) 2009 Corey Tabaka
 * Copyright (c) 2015 Intel Corporation
 * Copyright (c) 2016 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <arch.h>
#include <arch/arch_ops.h>
#include <arch/mmu.h>
#include <arch/x86.h>
#include <arch/x86/feature.h>
#include <arch/x86/mmu.h>
#include <assert.h>
#include <kernel/vm.h>
#include <lk/compiler.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/trace.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#define LOCAL_TRACE 0
#define TRACE_CONTEXT_SWITCH 0

// TODO:
// - proper tlb flush (local and SMP)
// - synchronization of top level page tables for user space aspaces

/* Address width including virtual/physical address*/
static uint8_t vaddr_width = 0;
static uint8_t paddr_width = 0;
static bool supports_huge_pages;
static bool supports_invpcid;
static bool supports_pcid;

/* top level kernel page tables, initialized in start.S */
map_addr_t kernel_pml4[NO_OF_PT_ENTRIES] __ALIGNED(PAGE_SIZE);
map_addr_t kernel_pdp[NO_OF_PT_ENTRIES] __ALIGNED(PAGE_SIZE); /* temporary */
map_addr_t kernel_pte[NO_OF_PT_ENTRIES] __ALIGNED(PAGE_SIZE);

/* saed physical address of the kernel_pml4 table */
paddr_t kernel_pml4_phys;

/* top level pdp needed to map the -512GB..0 space */
map_addr_t kernel_pdp_high[NO_OF_PT_ENTRIES] __ALIGNED(PAGE_SIZE);

/* a big pile of page tables needed to map 64GB of memory into kernel space using 2MB pages */
map_addr_t kernel_linear_map_pdp[(64ULL*GB) / (2*MB)];

/**
 * @brief  check if the virtual address is aligned and canonical
 *
 */
static bool x86_mmu_check_vaddr(vaddr_t vaddr) {
    uint64_t addr = (uint64_t)vaddr;
    uint64_t max_vaddr_lohalf,
             min_vaddr_hihalf;

    /* Check to see if the address is PAGE aligned */
    if (!IS_ALIGNED(addr, PAGE_SIZE))
        return false;

    /* get max address in lower-half canonical addr space */
    /* e.g. if width is 48, then 0x00007FFF_FFFFFFFF */
    max_vaddr_lohalf = ((uint64_t)1ull << (vaddr_width - 1)) - 1;

    /* get min address in higher-half canonical addr space */
    /* e.g. if width is 48, then 0xFFFF8000_00000000*/
    min_vaddr_hihalf = ~ max_vaddr_lohalf;

    /* Check to see if the address in a canonical address */
    if ((addr > max_vaddr_lohalf) && (addr < min_vaddr_hihalf))
        return false;

    return true;
}

/**
 * @brief  check if the physical address is valid and aligned
 *
 */
static bool x86_mmu_check_paddr(const paddr_t paddr) {
    /* Check to see if the address is PAGE aligned */
    if (!IS_ALIGNED(paddr, PAGE_SIZE))
        return false;

    uint64_t max_paddr = ((uint64_t)1ull << paddr_width) - 1;

    return paddr <= max_paddr;
}

/* is the address within the aspace */
static bool is_valid_vaddr(const arch_aspace_t *aspace, vaddr_t vaddr) {
    return (vaddr >= aspace->base && vaddr <= aspace->base + aspace->size - 1);
}

static inline uint64_t get_pfn_from_pte(uint64_t pte) {
    uint64_t pfn = (pte & (X86_PG_FRAME & X86_PHY_ADDR_MASK));
    return pfn;
}

static inline uint64_t get_pfn_from_pde(uint64_t pde) {
    uint64_t pfn = (pde & X86_2MB_PAGE_FRAME);
    return pfn;
}

static inline uint64_t get_pml4_entry_from_pml4_table(const vaddr_t vaddr, const uint64_t * const pml4_table) {
    const uint32_t index = (((uint64_t)vaddr >> PML4_SHIFT) & ((1ul << ADDR_OFFSET) - 1));

    LTRACEF_LEVEL(2, "pml4_table %p, index %u, returning %#llx\n", pml4_table, index, pml4_table[index]);

    return pml4_table[index];
}

static inline uint64_t get_pdp_entry_from_pdp_table(const vaddr_t vaddr, const uint64_t pml4e) {
    const uint64_t *pdpe_table = paddr_to_kvaddr(get_pfn_from_pte(pml4e));

    uint32_t index = (((uint64_t)vaddr >> PDP_SHIFT) & ((1ul << ADDR_OFFSET) - 1));

    LTRACEF_LEVEL(2, "pdpe_table %p, index %u, returning %#llx\n", pdpe_table, index, pdpe_table[index]);
    return pdpe_table[index];
}

static inline uint64_t get_pd_entry_from_pd_table(const vaddr_t vaddr, const uint64_t pdpe) {
    const uint64_t *pde_table = paddr_to_kvaddr(get_pfn_from_pte(pdpe));

    uint32_t index = (((uint64_t)vaddr >> PD_SHIFT) & ((1ul << ADDR_OFFSET) - 1));

    LTRACEF_LEVEL(2, "pde_table %p, index %u, returning %#llx\n", pde_table, index, pde_table[index]);
    return pde_table[index];
}

static inline uint64_t get_pt_entry_from_pt_table(const vaddr_t vaddr, const uint64_t pde) {
    const uint64_t *pte_table = paddr_to_kvaddr(get_pfn_from_pte(pde));

    uint32_t index = (((uint64_t)vaddr >> PT_SHIFT) & ((1ul << ADDR_OFFSET) - 1));

    LTRACEF_LEVEL(2, "pte_table %p, index %u, returning %#llx\n", pte_table, index, pte_table[index]);
    return pte_table[index];
}

static inline bool is_pte_present(uint64_t pte) {
    return pte & X86_MMU_PG_P;
}

/**
 * @brief Returning the x86 arch flags from generic mmu flags
 */
static arch_flags_t get_x86_arch_flags(uint flags) {
    arch_flags_t arch_flags = 0;

    if (!(flags & ARCH_MMU_FLAG_PERM_RO))
        arch_flags |= X86_MMU_PG_RW;

    if (flags & ARCH_MMU_FLAG_PERM_USER)
        arch_flags |= X86_MMU_PG_U;

    if (flags & ARCH_MMU_FLAG_UNCACHED)
        arch_flags |= X86_MMU_CACHE_DISABLE;

    if (flags & ARCH_MMU_FLAG_PERM_NO_EXECUTE)
        arch_flags |= X86_MMU_PG_NX;

    return arch_flags;
}

/**
 * @brief Returning the generic mmu flags from x86 arch flags
 */
static uint get_arch_mmu_flags(arch_flags_t flags) {
    uint mmu_flags = 0;

    if (!(flags & X86_MMU_PG_RW))
        mmu_flags |= ARCH_MMU_FLAG_PERM_RO;

    if (flags & X86_MMU_PG_U)
        mmu_flags |= ARCH_MMU_FLAG_PERM_USER;

    if (flags & X86_MMU_CACHE_DISABLE)
        mmu_flags |= ARCH_MMU_FLAG_UNCACHED;

    if (flags & X86_MMU_PG_NX)
        mmu_flags |= ARCH_MMU_FLAG_PERM_NO_EXECUTE;

    return (uint)mmu_flags;
}

/**
 * @brief  Walk the page table structures
 *
 * In this scenario, we are considering the paging scheme to be a PAE mode with
 * 4KB pages.
 *
 */
static status_t x86_mmu_get_mapping(uint64_t * const pml4_table, const vaddr_t vaddr, uint32_t * const ret_level,
                             arch_flags_t * const mmu_flags, paddr_t * const paddr) {
    uint64_t pml4e, pdpe, pde, pte;

    DEBUG_ASSERT(pml4_table);
    if ((!ret_level) || (!paddr) || (!mmu_flags)) {
        return ERR_INVALID_ARGS;
    }

    *ret_level = PML4_L;
    *paddr = 0;
    *mmu_flags = 0;

    LTRACEF_LEVEL(2, "pml4_table %p\n", pml4_table);

    pml4e = get_pml4_entry_from_pml4_table(vaddr, pml4_table);
    if (!is_pte_present(pml4e)) {
        return ERR_NOT_FOUND;
    }
    LTRACEF_LEVEL(2, "pml4e 0x%llx\n", pml4e);

    pdpe = get_pdp_entry_from_pdp_table(vaddr, pml4e);
    if (!is_pte_present(pdpe)) {
        *ret_level = PDP_L;
        return ERR_NOT_FOUND;
    }
    LTRACEF_LEVEL(2, "pdpe 0x%llx\n", pdpe);

    pde = get_pd_entry_from_pd_table(vaddr, pdpe);
    if (!is_pte_present(pde)) {
        *ret_level = PD_L;
        return ERR_NOT_FOUND;
    }
    LTRACEF_LEVEL(2, "pde 0x%llx\n", pde);

    /* 2 MB pages */
    if (pde & X86_MMU_PG_PS) {
        /* Getting the Page frame & adding the 4KB page offset from the vaddr */
        *paddr = get_pfn_from_pde(pde) + ((uint64_t)vaddr & PAGE_OFFSET_MASK_2MB);
        *mmu_flags = get_arch_mmu_flags(pde & X86_FLAGS_MASK);
        LTRACEF("getting flags from 2MB pte %#llx, flags %#llx\n", pde, *mmu_flags);
        goto last;
    }

    /* 4 KB pages */
    pte = get_pt_entry_from_pt_table(vaddr, pde);
    if (!is_pte_present(pte)) {
        *ret_level = PT_L;
        return ERR_NOT_FOUND;
    }

    /* Getting the Page frame & adding the 4KB page offset from the vaddr */
    *paddr = get_pfn_from_pte(pte) + ((uint64_t)vaddr & PAGE_OFFSET_MASK_4KB);
    *mmu_flags = get_arch_mmu_flags(pte & X86_FLAGS_MASK);

    LTRACEF("getting flags from pte %#llx, flags %#llx\n", pte, *mmu_flags);

last:

    *ret_level = PF_L;
    return NO_ERROR;
}

static void update_pt_entry(vaddr_t vaddr, uint64_t pde, paddr_t paddr, arch_flags_t flags) {
    uint64_t *pt_table = paddr_to_kvaddr(get_pfn_from_pte(pde));
    uint32_t pt_index = (((uint64_t)vaddr >> PT_SHIFT) & ((1ul << ADDR_OFFSET) - 1));
    pt_table[pt_index] = paddr;
    pt_table[pt_index] |= flags | X86_MMU_PG_P;
    if (!(flags & X86_MMU_PG_U))
        pt_table[pt_index] |= X86_MMU_PG_G; /* setting global flag for kernel pages */
    LTRACEF_LEVEL(2, "writing entry %#llx in pt %p at index %u\n", pt_table[pt_index], pt_table, pt_index);
}

static void update_pd_entry(vaddr_t vaddr, uint64_t pdpe, map_addr_t paddr, arch_flags_t flags) {
    uint64_t *pd_table = paddr_to_kvaddr(get_pfn_from_pte(pdpe));
    uint32_t pd_index = (((uint64_t)vaddr >> PD_SHIFT) & ((1ul << ADDR_OFFSET) - 1));
    pd_table[pd_index] = paddr;
    pd_table[pd_index] |= X86_MMU_PG_P | X86_MMU_PG_RW;
    if (flags & X86_MMU_PG_U)
        pd_table[pd_index] |= X86_MMU_PG_U;
    LTRACEF_LEVEL(2, "writing entry %#llx in pd %p at index %u\n", pd_table[pd_index], pd_table, pd_index);
}

static void update_pdp_entry(vaddr_t vaddr, uint64_t pml4e, map_addr_t paddr, arch_flags_t flags) {
    uint64_t *pdp_table = paddr_to_kvaddr(get_pfn_from_pte(pml4e));
    uint32_t pdp_index = (((uint64_t)vaddr >> PDP_SHIFT) & ((1ul << ADDR_OFFSET) - 1));
    pdp_table[pdp_index] = paddr;
    pdp_table[pdp_index] |= X86_MMU_PG_P | X86_MMU_PG_RW;
    if (flags & X86_MMU_PG_U)
        pdp_table[pdp_index] |= X86_MMU_PG_U;
    LTRACEF_LEVEL(2, "writing entry %#llx in pdp %p at index %u\n", pdp_table[pdp_index], pdp_table, pdp_index);
}

static void update_pml4_entry(vaddr_t vaddr, uint64_t *pml4_table, map_addr_t paddr, arch_flags_t flags) {
    uint32_t pml4_index = (((uint64_t)vaddr >> PML4_SHIFT) & ((1ul << ADDR_OFFSET) - 1));
    pml4_table[pml4_index] = paddr;
    pml4_table[pml4_index] |= X86_MMU_PG_P | X86_MMU_PG_RW;
    if (flags & X86_MMU_PG_U)
        pml4_table[pml4_index] |= X86_MMU_PG_U;

    /* Explicitly do not set the G bit here for kernel pages. Intel cores
     * ignore the G bit on PML4 entries, but AMD cores treat it as MBZ.
     */
    LTRACEF_LEVEL(2, "writing entry %#llx in pml4 %p at index %u\n", pml4_table[pml4_index], pml4_table, pml4_index);
}

/**
 * @brief Allocating a new page table
 */
static map_addr_t *alloc_page_table(paddr_t *pa_out) {
    vm_page_t *page = pmm_alloc_page();
    if (!page) {
        return NULL;
    }

    paddr_t pa = vm_page_to_paddr(page);
    DEBUG_ASSERT(pa != (paddr_t)-1);

    map_addr_t *page_ptr = paddr_to_kvaddr(pa);
    DEBUG_ASSERT(page_ptr);

    memset(page_ptr, 0, PAGE_SIZE);
    if (pa_out) {
        *pa_out = pa;
    }

    return page_ptr;
}

/**
 * @brief  Add a new mapping for the given virtual address & physical address
 *
 * This is a API which handles the mapping b/w a virtual address & physical address
 * either by checking if the mapping already exists and is valid OR by adding a
 * new mapping with the required flags.
 *
 * In this scenario, we are considering the paging scheme to be a PAE mode with
 * 4KB pages.
 *
 */
static status_t x86_mmu_add_mapping(uint64_t * const pml4, const map_addr_t paddr,
                             const vaddr_t vaddr, const arch_flags_t mmu_flags) {
    status_t ret = NO_ERROR;

    LTRACEF("pml4 %p paddr %#llx vaddr %#lx flags %#llx\n", pml4, paddr, vaddr, mmu_flags);

    DEBUG_ASSERT(pml4);
    if ((!x86_mmu_check_vaddr(vaddr)) || (!x86_mmu_check_paddr(paddr)) )
        return ERR_INVALID_ARGS;

    /* track at every level if we had to allocate a new page table for eventual cleanup code */
    bool pdp_new = false, pd_new = false, pt_new = false;

    uint64_t pdpe = 0;
    uint64_t pml4e = get_pml4_entry_from_pml4_table(vaddr, pml4);
    if (!is_pte_present(pml4e)) {
        /* Creating a new pdp table */
        paddr_t pa;
        map_addr_t *m = alloc_page_table(&pa);
        if (m == NULL) {
            ret = ERR_NO_MEMORY;
            goto clean;
        }

        LTRACEF_LEVEL(2, "new pdp pml4 %p m %p pa %#lx\n", pml4, m, pa);

        update_pml4_entry(vaddr, pml4, pa, get_x86_arch_flags(mmu_flags));
        pml4e = pa | X86_MMU_PG_P;
        pdp_new = true;
    } else {
        pdpe = get_pdp_entry_from_pdp_table(vaddr, pml4e);
    }

    LTRACEF_LEVEL(2, "pdpe %#llx\n", pdpe);

    uint64_t pde = 0;
    if (!is_pte_present(pdpe)) {
        /* Creating a new pd table  */
        paddr_t pa;
        map_addr_t *m  = alloc_page_table(&pa);
        if (m == NULL) {
            ret = ERR_NO_MEMORY;
            goto clean;
        }

        LTRACEF_LEVEL(2, "new pdp pml4e %#llx m %p pa %#lx\n", pml4e, m, pa);

        update_pdp_entry(vaddr, pml4e, pa, get_x86_arch_flags(mmu_flags));
        pdpe = pa | X86_MMU_PG_P;
        pd_new = true;
    } else {
        /* make sure it's not a large page, we currently cant handle splitting it */
        DEBUG_ASSERT((pdpe & X86_MMU_PG_PS) == 0);

        pde = get_pd_entry_from_pd_table(vaddr, pdpe);
    }

    LTRACEF_LEVEL(2, "pde %#llx\n", pde);

    if (!is_pte_present(pde)) {
        /* Creating a new pt */
        paddr_t pa;
        map_addr_t *m  = alloc_page_table(&pa);
        if (m == NULL) {
            ret = ERR_NO_MEMORY;
            goto clean;
        }

        LTRACEF_LEVEL(2, "new pt pdpe %#llx m %p pa %#lx\n", pdpe, m, pa);

        update_pd_entry(vaddr, pdpe, pa, get_x86_arch_flags(mmu_flags));
        pde = pa | X86_MMU_PG_P;
        pt_new = true;
    } else {
        /* make sure it's not a large page, we currently cant handle splitting it */
        DEBUG_ASSERT((pde & X86_MMU_PG_PS) == 0);
    }

    LTRACEF_LEVEL(2, "pde %#llx\n", pde);
    DEBUG_ASSERT(pde);

    /* Updating the page table entry with the paddr and access flags required for the mapping */
    update_pt_entry(vaddr, pde, paddr, get_x86_arch_flags(mmu_flags));

    return NO_ERROR;

clean:
    if (pdp_new || pd_new || pt_new) {
        /* TODO: handle cleaning up the addition of new page tables at any level for this mapping */
        TRACEF("aborting map code pdp_new %u pd_new %u pt_new %u\n", pdp_new, pd_new, pt_new);
    }

    return ret;
}

/**
 * @brief  x86-64 MMU unmap an entry in the page tables recursively and clear out tables
 *
 */
static void x86_mmu_unmap_entry(const vaddr_t vaddr, const int level, uint64_t * const table) {
    LTRACEF("vaddr 0x%lx level %d table %p\n", vaddr, level, table);

    uint64_t *next_table_addr = NULL;
    paddr_t next_table_pa = 0;
    uint32_t index = 0;
    switch (level) {
        case PML4_L:
            index = (((uint64_t)vaddr >> PML4_SHIFT) & ((1ul << ADDR_OFFSET) - 1));
            LTRACEF_LEVEL(2, "index %u\n", index);
            if (!is_pte_present(table[index]))
                return;
            next_table_pa = get_pfn_from_pte(table[index]);
            next_table_addr = paddr_to_kvaddr(next_table_pa);
            LTRACEF_LEVEL(2, "next_table_addr %p\n", next_table_addr);
            break;
        case PDP_L:
            index = (((uint64_t)vaddr >> PDP_SHIFT) & ((1ul << ADDR_OFFSET) - 1));
            LTRACEF_LEVEL(2, "index %u\n", index);
            if (!is_pte_present(table[index]))
                return;
            next_table_pa = get_pfn_from_pte(table[index]);
            next_table_addr = paddr_to_kvaddr(next_table_pa);
            LTRACEF_LEVEL(2, "next_table_addr %p\n", next_table_addr);
            break;
        case PD_L:
            index = (((uint64_t)vaddr >> PD_SHIFT) & ((1ul << ADDR_OFFSET) - 1));
            LTRACEF_LEVEL(2, "index %u\n", index);
            if (!is_pte_present(table[index]))
                return;
            next_table_pa = get_pfn_from_pte(table[index]);
            next_table_addr = paddr_to_kvaddr(next_table_pa);
            LTRACEF_LEVEL(2, "next_table_addr %p\n", next_table_addr);
            break;
        case PT_L:
            index = (((uint64_t)vaddr >> PT_SHIFT) & ((1ul << ADDR_OFFSET) - 1));
            LTRACEF_LEVEL(2, "index %u\n", index);
            if (!is_pte_present(table[index]))
                return;

            /* page frame is present, wipe it out */
            LTRACEF_LEVEL(2, "writing zero to entry, old val %#llx\n", table[index]);
            table[index] = 0;
            tlbsync_local(vaddr);
            return;
        default:
            // shouldn't recurse this far
            DEBUG_ASSERT(0);
    }

    LTRACEF_LEVEL(2, "recursing\n");

    x86_mmu_unmap_entry(vaddr, level - 1, next_table_addr);

    LTRACEF_LEVEL(2, "next_table_addr %p\n", next_table_addr);

    if (level > PT_L) {
        /* Check all entries of next level table for present bit */
        for (uint32_t next_level_offset = 0; next_level_offset < (PAGE_SIZE/8); next_level_offset++) {
            if (is_pte_present(next_table_addr[next_level_offset]))
                return; /* There is an entry in the next level table */
        }
        /* All present bits for all entries in next level table for this address are 0, so we
         * can unlink this page table.
         */
        if (is_pte_present(table[index])) {
            table[index] = 0;
            tlbsync_local(vaddr);
        }
        pmm_free_page(paddr_to_vm_page(next_table_pa));
    }
}

static status_t x86_mmu_unmap(uint64_t * const pml4, const vaddr_t vaddr, uint count) {
    DEBUG_ASSERT(pml4);
    if (!(x86_mmu_check_vaddr(vaddr)))
        return ERR_INVALID_ARGS;

    if (count == 0)
        return NO_ERROR;

    vaddr_t next_aligned_v_addr = vaddr;
    while (count > 0) {
        x86_mmu_unmap_entry(next_aligned_v_addr, X86_PAGING_LEVELS, pml4);
        next_aligned_v_addr += PAGE_SIZE;
        count--;
    }
    return NO_ERROR;
}

int arch_mmu_unmap(arch_aspace_t * const aspace, const vaddr_t vaddr, const uint count) {
    LTRACEF("aspace %p, vaddr 0x%lx, count %u\n", aspace, vaddr, count);

    DEBUG_ASSERT(aspace);

    if (!is_valid_vaddr(aspace, vaddr))
        return ERR_INVALID_ARGS;

    if (count == 0)
        return NO_ERROR;

    return (x86_mmu_unmap(aspace->cr3, vaddr, count));
}

/**
 * @brief  Mapping a section/range with specific permissions
 *
 */
static status_t x86_mmu_map_range(uint64_t * const pml4, struct map_range * const range, arch_flags_t const flags) {
    LTRACEF("pml4 %p, range v %#lx p %#lx size %u flags %#llx\n", pml4,
            range->start_vaddr, range->start_paddr, range->size, flags);

    DEBUG_ASSERT(pml4);
    if (!range)
        return ERR_INVALID_ARGS;

    /* Calculating the number of 4k pages */
    uint32_t no_of_pages;
    if (IS_ALIGNED(range->size, PAGE_SIZE))
        no_of_pages = (range->size) >> PAGE_DIV_SHIFT;
    else
        no_of_pages = ((range->size) >> PAGE_DIV_SHIFT) + 1;

    vaddr_t next_aligned_v_addr = range->start_vaddr;
    paddr_t next_aligned_p_addr = range->start_paddr;

    for (uint32_t index = 0; index < no_of_pages; index++) {
        status_t map_status = x86_mmu_add_mapping(pml4, next_aligned_p_addr, next_aligned_v_addr, flags);
        if (map_status) {
            dprintf(SPEW, "Add mapping failed with err=%d\n", map_status);
            /* Unmap the partial mapping - if any */
            x86_mmu_unmap(pml4, range->start_vaddr, index);
            return map_status;
        }
        next_aligned_v_addr += PAGE_SIZE;
        next_aligned_p_addr += PAGE_SIZE;
    }
    return NO_ERROR;
}

status_t arch_mmu_query(arch_aspace_t * const aspace, const vaddr_t vaddr, paddr_t * const paddr, uint * const flags) {
    LTRACEF("aspace %p, vaddr 0x%lx, paddr %p, flags %p\n", aspace, vaddr, paddr, flags);

    DEBUG_ASSERT(aspace);

    if (!paddr)
        return ERR_INVALID_ARGS;

    if (!is_valid_vaddr(aspace, vaddr))
        return ERR_INVALID_ARGS;

    arch_flags_t ret_flags;
    uint32_t ret_level;
    status_t stat = x86_mmu_get_mapping(aspace->cr3, vaddr, &ret_level, &ret_flags, paddr);
    if (stat)
        return stat;

    /* converting x86 arch specific flags to arch mmu flags */
    if (flags)
        *flags = ret_flags;
    LTRACEF("paddr %#lx, flags %#llx\n", *paddr, ret_flags);

    return NO_ERROR;
}

int arch_mmu_map(arch_aspace_t *const aspace, const vaddr_t vaddr, const paddr_t paddr, const uint count, const uint flags) {
    DEBUG_ASSERT(aspace);

    LTRACEF("aspace %p, vaddr 0x%lx paddr 0x%lx count %u flags 0x%x\n", aspace, vaddr, paddr, count, flags);

    if (flags & ARCH_MMU_FLAG_NS)
        return ERR_INVALID_ARGS;

    if ((!x86_mmu_check_paddr(paddr)))
        return ERR_INVALID_ARGS;

    if (!is_valid_vaddr(aspace, vaddr))
        return ERR_INVALID_ARGS;

    if (count == 0)
        return NO_ERROR;

    struct map_range range;
    range.start_vaddr = vaddr;
    range.start_paddr = paddr;
    range.size = count * PAGE_SIZE;

    return (x86_mmu_map_range(aspace->cr3, &range, flags));
}

bool arch_mmu_supports_nx_mappings(void) { return true; }
bool arch_mmu_supports_ns_mappings(void) { return false; }
bool arch_mmu_supports_user_aspaces(void) { return true; }

void x86_mmu_early_init_percpu(void) {
    /* Set WP bit in CR0 */
    uint64_t cr0 = x86_get_cr0();
    cr0 |= X86_CR0_WP;
    x86_set_cr0(cr0);

    /* Set some mmu control bits in CR4 */
    uint32_t bits = 0;
    bits |= x86_feature_test(X86_FEATURE_PGE) ? X86_CR4_PGE : 0;
    bits |= x86_feature_test(X86_FEATURE_PSE) ? X86_CR4_PSE : 0;
    bits |= x86_feature_test(X86_FEATURE_SMEP) ? X86_CR4_SMEP : 0;
    /* for now, we dont support SMAP due to some tests that assume they can access user space */
    // bits |= x86_feature_test(X86_FEATURE_SMAP) ? X86_CR4_SMAP : 0;
    if (bits) {
        /* don't touch cr4 unless we need to, early cpus will fault if its not implemented */
        uint32_t cr4 = x86_get_cr4();
        cr4 |= bits;
        x86_set_cr4(cr4);
    }

    /* Set NXE bit in MSR_EFER */
    uint64_t efer_msr = read_msr(X86_MSR_IA32_EFER);
    efer_msr |= X86_EFER_NXE;
    write_msr(X86_MSR_IA32_EFER, efer_msr);
}

void x86_mmu_early_init(void) {
    /* getting the address width from CPUID instr */
    paddr_width = x86_get_paddr_width();
    vaddr_width = x86_get_vaddr_width();

    /* check to see if we support huge (1GB) and invpcid instruction */
    supports_huge_pages = x86_feature_test(X86_FEATURE_PG1G);
    supports_invpcid = x86_feature_test(X86_FEATURE_INVPCID);
    supports_pcid = x86_feature_test(X86_FEATURE_PCID);

    /* unmap the lower identity mapping */
    kernel_pml4[0] = 0;

    /* tlb flush */
    x86_set_cr3(kernel_pml4_phys);
}

void x86_mmu_init(void) {
    dprintf(SPEW, "X86: mmu max phys bits %u max virt bits %u\n", paddr_width, vaddr_width);
    dprintf(SPEW, "X86: mmu features: 1GB pages %u, pcid %u, invpcid %u\n", supports_huge_pages,
            supports_pcid, supports_invpcid);
}

/*
 * x86-64 does not support multiple address spaces at the moment, so fail if these apis
 * are used for it.
 */
status_t arch_mmu_init_aspace(arch_aspace_t * const aspace, const vaddr_t base, const size_t size, const uint flags) {
    DEBUG_ASSERT(aspace);

    LTRACEF("aspace %p, base %#lx, size %#zx, flags %#x\n", aspace, base, size, flags);

    /* validate that the base + size is sane and doesn't wrap */
    DEBUG_ASSERT(size > PAGE_SIZE);
    DEBUG_ASSERT(base + size - 1 > base);

    aspace->flags = flags;
    if (flags & ARCH_ASPACE_FLAG_KERNEL) {
        /* at the moment we can only deal with address spaces as globally defined */
        DEBUG_ASSERT(base == KERNEL_ASPACE_BASE);
        DEBUG_ASSERT(size == KERNEL_ASPACE_SIZE);

        aspace->base = base;
        aspace->size = size;
        aspace->cr3 = kernel_pml4;
        aspace->cr3_phys = kernel_pml4_phys;
    } else {
        DEBUG_ASSERT(base == USER_ASPACE_BASE);
        DEBUG_ASSERT(size == USER_ASPACE_SIZE);

        aspace->base = base;
        aspace->size = size;

        map_addr_t *va = pmm_alloc_kpages(1, NULL);
        if (!va) {
            return ERR_NO_MEMORY;
        }

        aspace->cr3 = va;
        aspace->cr3_phys = vaddr_to_paddr(aspace->cr3);

        /* copy the top entries from the kernel top table */
        memcpy(aspace->cr3 + NO_OF_PT_ENTRIES/2, kernel_pml4 + NO_OF_PT_ENTRIES/2, PAGE_SIZE/2);

        /* zero out the rest */
        memset(aspace->cr3, 0, PAGE_SIZE/2);
    }

    return NO_ERROR;
}

status_t arch_mmu_destroy_aspace(arch_aspace_t *aspace) {
    // TODO: assert that we're not active on any cpus
    if (aspace->flags & ARCH_ASPACE_FLAG_KERNEL) {
        // can't destroy the kernel aspace
        panic("attempt to destroy kernel aspace\n");
        return ERR_NOT_ALLOWED;
    }

    // free the page table
    pmm_free_kpages(aspace->cr3, 1);

    return NO_ERROR;
}

void arch_mmu_context_switch(arch_aspace_t *new_aspace) {
    if (TRACE_CONTEXT_SWITCH)
        TRACEF("aspace %p\n", new_aspace);

    uint64_t cr3;
    if (new_aspace) {
        DEBUG_ASSERT((new_aspace->flags & ARCH_ASPACE_FLAG_KERNEL) == 0);

        cr3 = new_aspace->cr3_phys;
    } else {
        cr3 = kernel_pml4_phys;
    }
    if (TRACE_CONTEXT_SWITCH) {
        TRACEF("cr3 %#llx\n", cr3);
    }

    x86_set_cr3(cr3);
}

