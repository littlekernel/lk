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

/* top level kernel page tables, initialized in start.S */
#if X86_LEGACY
/* enough page tables to map 16MB ram using 4K pages */
map_addr_t kernel_pt[NO_OF_PT_ENTRIES][4] __ALIGNED(PAGE_SIZE);
#endif
map_addr_t kernel_pd[NO_OF_PT_ENTRIES] __ALIGNED(PAGE_SIZE);

static inline paddr_t get_pfn_from_pde(map_addr_t pde) {
    return pde & X86_4MB_PAGE_FRAME;
}

static inline paddr_t get_pfn_from_pte(map_addr_t pte) {
    return pte & X86_PG_FRAME;
}

static inline map_addr_t get_pd_entry_from_pd_table(vaddr_t vaddr, const map_addr_t *pdt) {
    uint32_t pd_index = ((vaddr >> PD_SHIFT) & ((1 << ADDR_OFFSET) - 1));
    return pdt[pd_index];
}

static inline map_addr_t get_pt_entry_from_page_table(vaddr_t vaddr, const map_addr_t *pt) {
    uint32_t pt_index = ((vaddr >> PT_SHIFT) & ((1 << ADDR_OFFSET) - 1));
    return pt[pt_index];
}

/**
 * @brief Returning the x86 arch flags from generic mmu flags
 */
static arch_flags_t get_x86_arch_flags(arch_flags_t flags) {
    arch_flags_t arch_flags = 0;

    if (!(flags & ARCH_MMU_FLAG_PERM_RO))
        arch_flags |= X86_MMU_PG_RW;

    if (flags & ARCH_MMU_FLAG_PERM_USER)
        arch_flags |= X86_MMU_PG_U;

    if (flags & ARCH_MMU_FLAG_UNCACHED)
        arch_flags |= X86_MMU_CACHE_DISABLE;

    return arch_flags;
}

/**
 * @brief Returning the generic mmu flags from x86 arch flags
 */
static uint get_arch_mmu_flags(arch_flags_t flags) {
    arch_flags_t mmu_flags = 0;

    if (!(flags & X86_MMU_PG_RW))
        mmu_flags |= ARCH_MMU_FLAG_PERM_RO;

    if (flags & X86_MMU_PG_U)
        mmu_flags |= ARCH_MMU_FLAG_PERM_USER;

    if (flags & X86_MMU_CACHE_DISABLE)
        mmu_flags |= ARCH_MMU_FLAG_UNCACHED;

    return (uint)mmu_flags;
}

/**
 * @brief  Walk the page table structures - supported for both PAE & non-PAE modes
 *
 */
static status_t x86_mmu_get_mapping(map_addr_t * const init_table, const vaddr_t vaddr, uint32_t * const ret_level,
                             arch_flags_t * const mmu_flags, paddr_t * const paddr) {
    DEBUG_ASSERT(init_table);
    DEBUG_ASSERT(ret_level);
    DEBUG_ASSERT(mmu_flags);
    DEBUG_ASSERT(paddr);

    *ret_level = PD_L;
    *paddr = 0;
    *mmu_flags = 0;

    map_addr_t * const pdt = init_table; /* First table in non PAE mode is pdt */
    LTRACEF("pdt %p\n", pdt);

    const map_addr_t pde = get_pd_entry_from_pd_table(vaddr, pdt);
    LTRACEF("pde %#x\n", pde);
    if ((pde & X86_MMU_PG_P) == 0)
        return ERR_NOT_FOUND;

    /* 4 MB pages */
    /* In this case, the page directory entry is NOT actually a PT (page table) */
    if (pde & X86_MMU_PG_PS) {
        /* Getting the Page frame & adding the 4MB page offset from the vaddr */
        *paddr = get_pfn_from_pde(pde) + (vaddr & PAGE_OFFSET_MASK_4MB);
        *mmu_flags = get_arch_mmu_flags(pde & X86_FLAGS_MASK);
        *ret_level = PT_L;
        return NO_ERROR;
    }

    /* 4 KB pages */
    map_addr_t * const pt = paddr_to_kvaddr(get_pfn_from_pte(pde));
    const map_addr_t pte = get_pt_entry_from_page_table(vaddr, pt);
    LTRACEF("pte %#x\n", pte);
    if ((pte & X86_MMU_PG_P) == 0) {
        *ret_level = PT_L;
        *paddr = 0;
        return ERR_NOT_FOUND;
    }

    /* Getting the Page frame & adding the 4KB page offset from the vaddr */
    *paddr = get_pfn_from_pte(pte) + (vaddr & PAGE_OFFSET_MASK_4KB);
    *mmu_flags = get_arch_mmu_flags(pte & X86_FLAGS_MASK);

last:
    *ret_level = PF_L;
    return NO_ERROR;
}

static void update_pt_entry(const vaddr_t vaddr, map_addr_t * const pt_table, const paddr_t paddr, const arch_flags_t flags) {
    const uint32_t pt_index = ((vaddr >> PT_SHIFT) & ((1 << ADDR_OFFSET) - 1));

    map_addr_t entry = paddr;
    entry |= flags | X86_MMU_PG_P; /* last level - actual page being mapped */
    if (!(flags & X86_MMU_PG_U))
        entry |= X86_MMU_PG_G; /* setting global flag for kernel pages */

    pt_table[pt_index] = entry;
    LTRACEF_LEVEL(2, "writing entry %#x in pt %p at index %u\n", pt_table[pt_index], pt_table, pt_index);
}

static void update_pd_entry(const vaddr_t vaddr, map_addr_t * const pd_table, const paddr_t paddr, const arch_flags_t flags) {
    const uint32_t pd_index = ((vaddr >> PD_SHIFT) & ((1 << ADDR_OFFSET) - 1));

    map_addr_t entry = paddr;
    entry |= X86_MMU_PG_P | X86_MMU_PG_RW;
    if (flags & X86_MMU_PG_U)
        entry |= X86_MMU_PG_U;
    else
        entry |= X86_MMU_PG_G; /* setting global flag for kernel pages */

    pd_table[pd_index] = entry;
    LTRACEF_LEVEL(2, "writing entry %#x in pd %p at index %u\n", pd_table[pd_index], pd_table, pd_index);
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
 */
static status_t x86_mmu_add_mapping(map_addr_t * const init_table, const map_addr_t paddr,
                             const vaddr_t vaddr, const arch_flags_t mmu_flags) {
    status_t ret;

    DEBUG_ASSERT(init_table);
    if ((!IS_ALIGNED(vaddr, PAGE_SIZE)) || (!IS_ALIGNED(paddr, PAGE_SIZE)) )
        return ERR_INVALID_ARGS;

    map_addr_t *pt;

    const map_addr_t pte = get_pd_entry_from_pd_table(vaddr, init_table);
    if ((pte & X86_MMU_PG_P) == 0) {
        /* Creating a new pt */
        paddr_t pd_paddr;
        map_addr_t *m  = alloc_page_table(&pd_paddr);
        if (m == NULL) {
            ret = ERR_NO_MEMORY;
            goto clean;
        }

        DEBUG_ASSERT(pd_paddr);

        update_pd_entry(vaddr, init_table, pd_paddr, get_x86_arch_flags(mmu_flags));
        pt = m;
    } else {
        pt = paddr_to_kvaddr(get_pfn_from_pte(pte));
    }

    /* Updating the page table entry with the paddr and access flags required for the mapping */
    update_pt_entry(vaddr, pt, paddr, get_x86_arch_flags(mmu_flags));
    ret = NO_ERROR;
clean:
    return ret;
}

/**
 * @brief  x86 MMU unmap an entry in the page tables recursively and clear out tables
 *
 */
static void x86_mmu_unmap_entry(const vaddr_t vaddr, const int level, map_addr_t * const table) {
    LTRACEF("vaddr %#lx, level %d, table %p\n", vaddr, level, table);

    uint32_t index = 0;
    map_addr_t *next_table_addr = NULL;
    paddr_t next_table_pa = 0;
    switch (level) {
        case PD_L:
            index = ((vaddr >> PD_SHIFT) & ((1 << ADDR_OFFSET) - 1));
            LTRACEF_LEVEL(2, "index %u\n", index);
            if ((table[index] & X86_MMU_PG_P) == 0)
                return;
            next_table_pa = get_pfn_from_pte(table[index]);
            next_table_addr = paddr_to_kvaddr(next_table_pa);
            LTRACEF_LEVEL(2, "next_table_addr %p\n", next_table_addr);
            break;
        case PT_L:
            index = ((vaddr >> PT_SHIFT) & ((1 << ADDR_OFFSET) - 1));
            LTRACEF_LEVEL(2, "index %u\n", index);
            if ((table[index] & X86_MMU_PG_P) == 0)
                return;

            /* page frame is present, wipe it out */
            LTRACEF_LEVEL(2, "writing zero to entry, old val %#x\n", table[index]);
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
        for (uint32_t next_level_offset = 0; next_level_offset < NO_OF_PT_ENTRIES; next_level_offset++) {
            if ((next_table_addr[next_level_offset] & X86_MMU_PG_P) != 0)
                return; /* There is an entry in the next level table */
        }
        /* All present bits for all entries in next level table for this address are 0, so we
         * can unlink this page table.
         */
        if ((table[index] & X86_MMU_PG_P) != 0) {
            table[index] = 0;
            tlbsync_local(vaddr);
        }
        pmm_free_page(paddr_to_vm_page(next_table_pa));
    }
}

static status_t x86_mmu_unmap(map_addr_t * const init_table, const vaddr_t vaddr, uint count) {
    LTRACEF("init_table %p, vaddr %#lx, count %u\n", init_table, vaddr, count);

    DEBUG_ASSERT(init_table);
    if (!IS_ALIGNED(vaddr, PAGE_SIZE))
        return ERR_INVALID_ARGS;

    if (count == 0)
        return NO_ERROR;

    vaddr_t next_aligned_v_addr = vaddr;
    while (count > 0) {
        x86_mmu_unmap_entry(next_aligned_v_addr, X86_PAGING_LEVELS, init_table);
        next_aligned_v_addr += PAGE_SIZE;
        count--;
    }
    return NO_ERROR;
}

int arch_mmu_unmap(arch_aspace_t * const aspace, const vaddr_t vaddr, const uint count) {
    map_addr_t init_table_from_cr3;

    LTRACEF("aspace %p, vaddr %#lx, count %u\n", aspace, vaddr, count);

    DEBUG_ASSERT(aspace);

    if (!IS_ALIGNED(vaddr, PAGE_SIZE))
        return ERR_INVALID_ARGS;

    if (count == 0)
        return NO_ERROR;

    DEBUG_ASSERT(x86_get_cr3());
    init_table_from_cr3 = x86_get_cr3();

    return (x86_mmu_unmap(paddr_to_kvaddr(init_table_from_cr3), vaddr, count));
}

/**
 * @brief  Mapping a section/range with specific permissions
 *
 */
static status_t x86_mmu_map_range(map_addr_t * const init_table, struct map_range * const range, const arch_flags_t flags) {

    LTRACEF("table %p, range vaddr 0x%lx paddr 0x%lx size %u\n", init_table, range->start_vaddr, range->start_paddr, range->size);

    DEBUG_ASSERT(init_table);
    if (!range)
        return ERR_INVALID_ARGS;

    /* Calculating the number of 4k pages */
    uint32_t no_of_pages;
    if (IS_ALIGNED(range->size, PAGE_SIZE))
        no_of_pages = (range->size) >> PAGE_DIV_SHIFT;
    else
        no_of_pages = ((range->size) >> PAGE_DIV_SHIFT) + 1;

    vaddr_t next_aligned_v_addr = range->start_vaddr;
    map_addr_t next_aligned_p_addr = range->start_paddr;

    for (uint32_t index = 0; index < no_of_pages; index++) {
        status_t map_status = x86_mmu_add_mapping(init_table, next_aligned_p_addr, next_aligned_v_addr, flags);
        if (map_status) {
            dprintf(SPEW, "Add mapping failed with err=%d\n", map_status);
            /* Unmap the partial mapping - if any */
            x86_mmu_unmap(init_table, range->start_vaddr, index);
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

    DEBUG_ASSERT(x86_get_cr3());
    uint32_t current_cr3_val = (map_addr_t)x86_get_cr3();

    arch_flags_t ret_flags;
    uint32_t ret_level;
    status_t stat = x86_mmu_get_mapping(paddr_to_kvaddr(current_cr3_val), vaddr, &ret_level, &ret_flags, paddr);
    if (stat)
        return stat;

    /* converting x86 arch specific flags to arch mmu flags */
    if (flags)
        *flags = ret_flags;

    LTRACEF("returning paddr %#lx flags %#x\n", *paddr, ret_flags);

    return NO_ERROR;
}

int arch_mmu_map(arch_aspace_t * const aspace, const vaddr_t vaddr, const paddr_t paddr, const uint count, const uint flags) {
    LTRACEF("aspace %p, vaddr %#lx, paddr %#lx, count %u, flags %#x\n", aspace, vaddr, paddr, count, flags);

    DEBUG_ASSERT(aspace);

    if (flags & (ARCH_MMU_FLAG_PERM_NO_EXECUTE | ARCH_MMU_FLAG_NS))
        return ERR_INVALID_ARGS;

    if ((!IS_ALIGNED(paddr, PAGE_SIZE)) || (!IS_ALIGNED(vaddr, PAGE_SIZE)))
        return ERR_INVALID_ARGS;

    if (count == 0)
        return NO_ERROR;

    DEBUG_ASSERT(x86_get_cr3());
    uint32_t current_cr3_val = (map_addr_t)x86_get_cr3();

    struct map_range range;
    range.start_vaddr = vaddr;
    range.start_paddr = (map_addr_t)paddr;
    range.size = count * PAGE_SIZE;

    return (x86_mmu_map_range(paddr_to_kvaddr(current_cr3_val), &range, flags));
}

bool arch_mmu_supports_nx_mappings(void) { return false; }
bool arch_mmu_supports_ns_mappings(void) { return false; }
bool arch_mmu_supports_user_aspaces(void) { return false; }

void x86_mmu_early_init(void) {
    /* Set WP bit in CR0*/
    volatile uint32_t cr0 = x86_get_cr0();
    cr0 |= X86_CR0_WP;
    x86_set_cr0(cr0);

    /* unmap the lower identity mapping */
    for (uint i = 0; i < (1024*1024*1024) / (4*1024*1024); i++) {
        kernel_pd[i] = 0;
    }

    /* tlb flush */
    x86_set_cr3(x86_get_cr3());
}

void x86_mmu_init(void) {
}

/*
 * x86 does not support multiple address spaces at the moment, so fail if these apis
 * are used for it.
 */
status_t arch_mmu_init_aspace(arch_aspace_t * const aspace, const vaddr_t base, const size_t size, const uint flags) {
    DEBUG_ASSERT(aspace);

    if ((flags & ARCH_ASPACE_FLAG_KERNEL) == 0) {
        return ERR_NOT_SUPPORTED;
    }

    return NO_ERROR;
}

status_t arch_mmu_destroy_aspace(arch_aspace_t * const aspace) {
    return NO_ERROR;
}

void arch_mmu_context_switch(arch_aspace_t * const aspace) {
    if (aspace != NULL) {
        PANIC_UNIMPLEMENTED;
    }
}

