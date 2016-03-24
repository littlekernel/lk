/*
 * Copyright (c) 2009 Corey Tabaka
 * Copyright (c) 2015 Intel Corporation
 * Copyright (c) 2016 Travis Geiselbrecht
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

#include <debug.h>
#include <trace.h>
#include <sys/types.h>
#include <compiler.h>
#include <arch.h>
#include <arch/x86.h>
#include <arch/x86/mmu.h>
#include <stdlib.h>
#include <string.h>
#include <arch/mmu.h>
#include <assert.h>
#include <err.h>
#include <arch/arch_ops.h>
#include <kernel/vm.h>

#define LOCAL_TRACE 0

/* top level kernel page tables, initialized in start.S */
#ifdef PAE_MODE_ENABLED
map_addr_t pdp[NO_OF_PT_ENTRIES] __ALIGNED(PAGE_SIZE);
map_addr_t pdpt[NO_OF_PT_ENTRIES] __ALIGNED(PAGE_SIZE);
#endif
map_addr_t pd[NO_OF_PT_ENTRIES] __ALIGNED(PAGE_SIZE);

#ifdef PAE_MODE_ENABLED
/* PDP table address is 32 bit wide when on PAE mode, but the PDP entries are 64 bit wide */
static inline map_addr_t get_pdp_entry_from_pdp_table(vaddr_t vaddr, map_addr_t pdpt)
{
    uint32_t pdp_index;
    map_addr_t *pdp_table;

    pdp_index = ((vaddr >> PDP_SHIFT) & ((1ul << PDPT_ADDR_OFFSET) - 1));
    pdp_table = (map_addr_t *)(pdpt & X86_PDPT_ADDR_MASK);
    return X86_PHYS_TO_VIRT(pdp_table[pdp_index]);
}

static inline map_addr_t get_pfn_from_pt(map_addr_t pt)
{
    map_addr_t pfn;

    pfn = (pt & X86_2MB_PAGE_FRAME);
    return X86_PHYS_TO_VIRT(pfn);
}

#else
static inline map_addr_t get_pfn_from_pde(map_addr_t pde)
{
    map_addr_t pfn;

    pfn = (pde & X86_4MB_PAGE_FRAME);
    return X86_PHYS_TO_VIRT(pfn);
}
#endif

static inline map_addr_t get_pd_entry_from_pd_table(vaddr_t vaddr, map_addr_t pdt)
{
    uint32_t pd_index;
    map_addr_t *pd_table;

    pd_index = ((vaddr >> PD_SHIFT) & ((1 << ADDR_OFFSET) - 1));
    pd_table = (map_addr_t *)(pdt & X86_PG_FRAME);
    return X86_PHYS_TO_VIRT(pd_table[pd_index]);
}

static inline map_addr_t get_pt_entry_from_page_table(vaddr_t vaddr, map_addr_t pt)
{
    uint32_t pt_index;
    map_addr_t *pt_table;

    pt_index = ((vaddr >> PT_SHIFT) & ((1 << ADDR_OFFSET) - 1));
    pt_table = (map_addr_t *)(pt & X86_PG_FRAME);
    return X86_PHYS_TO_VIRT(pt_table[pt_index]);
}

static inline map_addr_t get_pfn_from_pte(map_addr_t pte)
{
    map_addr_t pfn;

    pfn = (pte & X86_PG_FRAME);
    return X86_PHYS_TO_VIRT(pfn);
}

/**
 * @brief Returning the x86 arch flags from generic mmu flags
 */
arch_flags_t get_x86_arch_flags(arch_flags_t flags)
{
    arch_flags_t arch_flags = 0;

    if (!(flags & ARCH_MMU_FLAG_PERM_RO))
        arch_flags |= X86_MMU_PG_RW;

    if (flags & ARCH_MMU_FLAG_PERM_USER)
        arch_flags |= X86_MMU_PG_U;

    if (flags & ARCH_MMU_FLAG_UNCACHED)
        arch_flags |= X86_MMU_CACHE_DISABLE;

#ifdef PAE_MODE_ENABLED
    if (flags & ARCH_MMU_FLAG_PERM_NO_EXECUTE)
        arch_flags |= X86_MMU_PG_NX;
#endif
    return arch_flags;
}

/**
 * @brief Returning the generic mmu flags from x86 arch flags
 */
uint get_arch_mmu_flags(arch_flags_t flags)
{
    arch_flags_t mmu_flags = 0;

    if (!(flags & X86_MMU_PG_RW))
        mmu_flags |= ARCH_MMU_FLAG_PERM_RO;

    if (flags & X86_MMU_PG_U)
        mmu_flags |= ARCH_MMU_FLAG_PERM_USER;

    if (flags & X86_MMU_CACHE_DISABLE)
        mmu_flags |= ARCH_MMU_FLAG_UNCACHED;

#ifdef PAE_MODE_ENABLED
    if (flags & X86_MMU_PG_NX)
        mmu_flags |= ARCH_MMU_FLAG_PERM_NO_EXECUTE;
#endif
    return (uint)mmu_flags;
}

/**
 * @brief  Walk the page table structures - supported for both PAE & non-PAE modes
 *
 */
status_t x86_mmu_get_mapping(map_addr_t init_table, vaddr_t vaddr, uint32_t *ret_level,
                             arch_flags_t *mmu_flags, map_addr_t *last_valid_entry)
{
    map_addr_t pt, pte, pdt;
#ifdef PAE_MODE_ENABLED
    map_addr_t pdpt;
#endif

    DEBUG_ASSERT(init_table);
    if ((!ret_level) || (!last_valid_entry) || (!mmu_flags)) {
        return ERR_INVALID_ARGS;
    }

    *mmu_flags = 0;

#ifdef PAE_MODE_ENABLED
    pdpt = init_table; /* First level table in PAE mode is pdpt */
    *ret_level = PDP_L;
    *last_valid_entry = pdpt;

    pdt = get_pdp_entry_from_pdp_table(vaddr, pdpt);
    if ((pdt & X86_MMU_PG_P) == 0) {
        *ret_level = PDP_L;
        *last_valid_entry = pdpt;
        return ERR_NOT_FOUND;
    }

    pt = get_pd_entry_from_pd_table(vaddr, pdt);
    if ((pt & X86_MMU_PG_P) == 0) {
        *ret_level = PD_L;
        *last_valid_entry = pdt;
        return ERR_NOT_FOUND;
    }
#else
    pdt = init_table; /* First table in non PAE mode is pdt */
    *ret_level = PD_L;
    *last_valid_entry = pdt;

    pt = get_pd_entry_from_pd_table(vaddr, pdt);
    if ((pt & X86_MMU_PG_P) == 0)
        return ERR_NOT_FOUND;
#endif

    /* 4 MB pages (non PAE mode) and 2 MB pages (PAE mode) */
    /* In this case, the page directory entry is NOT actually a PT (page table) */
    if (pt & X86_MMU_PG_PS) {
#ifdef PAE_MODE_ENABLED
        /* Getting the Page frame & adding the 4KB page offset from the vaddr */
        *last_valid_entry = get_pfn_from_pt(pt) + (vaddr & PAGE_OFFSET_MASK_2MB);
#else
        /* Getting the Page frame & adding the 4MB page offset from the vaddr */
        *last_valid_entry = get_pfn_from_pde(pt) + (vaddr & PAGE_OFFSET_MASK_4MB);
#endif
        *mmu_flags = get_arch_mmu_flags((X86_PHYS_TO_VIRT(pt)) & X86_FLAGS_MASK);
        goto last;
    }

    /* 4 KB pages */
    pte = get_pt_entry_from_page_table(vaddr, pt);
    if ((pte & X86_MMU_PG_P) == 0) {
        *ret_level = PT_L;
        *last_valid_entry = pt;
        return ERR_NOT_FOUND;
    }

    /* Getting the Page frame & adding the 4KB page offset from the vaddr */
    *last_valid_entry = get_pfn_from_pte(pte) + (vaddr & PAGE_OFFSET_MASK_4KB);
    *mmu_flags = get_arch_mmu_flags((X86_PHYS_TO_VIRT(pte)) & X86_FLAGS_MASK);
last:
    *ret_level = PF_L;
    return NO_ERROR;
}

/**
 * Walk the page table structures to see if the mapping between a virtual address
 * and a physical address exists. Also, check the flags.
 *
 */
status_t x86_mmu_check_mapping(map_addr_t init_table, map_addr_t paddr,
                               vaddr_t vaddr, arch_flags_t in_flags,
                               uint32_t *ret_level, arch_flags_t *ret_flags,
                               map_addr_t *last_valid_entry)
{
    status_t status;
    arch_flags_t existing_flags = 0;

    DEBUG_ASSERT(init_table);
    if ((!ret_level) || (!last_valid_entry) || (!ret_flags) ||
            (!IS_ALIGNED(vaddr, PAGE_SIZE)) ||
            (!IS_ALIGNED(paddr, PAGE_SIZE))) {
        return ERR_INVALID_ARGS;
    }

    status = x86_mmu_get_mapping(init_table, vaddr, ret_level, &existing_flags, last_valid_entry);
    if (status || ((*last_valid_entry) != paddr)) {
        /* We did not reach till we check the access flags for the mapping */
        *ret_flags = in_flags;
        return ERR_NOT_FOUND;
    }

    /* Checking the access flags for the mapped address. If it is not zero, then
     * the access flags are different & the return flag will have those access bits
     * which are different.
     */
    *ret_flags = (in_flags ^ get_x86_arch_flags(existing_flags)) & X86_DIRTY_ACCESS_MASK;

    if (!(*ret_flags))
        return NO_ERROR;

    return ERR_NOT_FOUND;
}

#ifdef PAE_MODE_ENABLED
static void update_pdp_entry(vaddr_t vaddr, map_addr_t pdpt, map_addr_t *m, arch_flags_t flags)
{
    uint32_t pdp_index;

    map_addr_t *pdp_table = (map_addr_t *)(pdpt & X86_PG_FRAME);
    pdp_index = ((vaddr >> PDP_SHIFT) & ((1ul << PDPT_ADDR_OFFSET) - 1));
    pdp_table[pdp_index] = (map_addr_t)m;
    pdp_table[pdp_index] |= X86_MMU_PG_P;
}
#endif

static void update_pt_entry(vaddr_t vaddr, map_addr_t paddr, map_addr_t pt, arch_flags_t flags)
{
    uint32_t pt_index;

    map_addr_t *pt_table = (map_addr_t *)(pt & X86_PG_FRAME);
    pt_index = ((vaddr >> PT_SHIFT) & ((1 << ADDR_OFFSET) - 1));
    pt_table[pt_index] = paddr;
    pt_table[pt_index] |= flags | X86_MMU_PG_P; /* last level - actual page being mapped */
    if (!(flags & X86_MMU_PG_U))
        pt_table[pt_index] |= X86_MMU_PG_G; /* setting global flag for kernel pages */
}

static void update_pd_entry(vaddr_t vaddr, map_addr_t pdt, paddr_t m, arch_flags_t flags)
{
    uint32_t pd_index;

    map_addr_t *pd_table = (map_addr_t *)(pdt & X86_PG_FRAME);
    pd_index = ((vaddr >> PD_SHIFT) & ((1 << ADDR_OFFSET) - 1));
    pd_table[pd_index] = m;
    pd_table[pd_index] |= X86_MMU_PG_P | X86_MMU_PG_RW;
    if (flags & X86_MMU_PG_U)
        pd_table[pd_index] |= X86_MMU_PG_U;
    else
        pd_table[pd_index] |= X86_MMU_PG_G; /* setting global flag for kernel pages */
}

/**
 * @brief Allocating a new page table
 */
static map_addr_t *_map_alloc_page(void)
{
    map_addr_t *page_ptr = pmm_alloc_kpage();
    DEBUG_ASSERT(page_ptr);

    if (page_ptr)
        memset(page_ptr, 0, PAGE_SIZE);

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
status_t x86_mmu_add_mapping(map_addr_t init_table, map_addr_t paddr,
                             vaddr_t vaddr, arch_flags_t mmu_flags)
{
#ifdef PAE_MODE_ENABLED
    map_addr_t pdt;
    uint32_t pd_new = 0;
#endif
    map_addr_t pt, *m = NULL;
    status_t ret = NO_ERROR;

    DEBUG_ASSERT(init_table);
    if ((!IS_ALIGNED(vaddr, PAGE_SIZE)) || (!IS_ALIGNED(paddr, PAGE_SIZE)) )
        return ERR_INVALID_ARGS;

#ifdef PAE_MODE_ENABLED
#error fix map_alloc_page to translate to physical
    pdt = get_pdp_entry_from_pdp_table(vaddr, init_table);
    if ((pdt & X86_MMU_PG_P) == 0) {
        /* Creating a new pd table  */
        m  = _map_alloc_page();
        if (m == NULL) {
            ret = ERR_NO_MEMORY;
            goto clean;
        }
        update_pdp_entry(vaddr, init_table, m, get_x86_arch_flags(mmu_flags));
        pdt = (map_addr_t)m;
        pd_new = 1;
    }

    if (!pd_new)
        pt = get_pd_entry_from_pd_table(vaddr, pdt);

    if (pd_new || (pt & X86_MMU_PG_P) == 0) {
        /* Creating a new pt */
        m  = _map_alloc_page();
        if (m == NULL) {
            ret = ERR_NO_MEMORY;
            if (pd_new)
                goto clean_pd;
            goto clean;
        }

        update_pd_entry(vaddr, pdt, m, get_x86_arch_flags(mmu_flags));
        pt = (map_addr_t)m;
    }
#else
    pt = get_pd_entry_from_pd_table(vaddr, init_table);
    if ((pt & X86_MMU_PG_P) == 0) {
        /* Creating a new pt */
        m  = _map_alloc_page();
        if (m == NULL) {
            ret = ERR_NO_MEMORY;
            goto clean;
        }

        paddr_t pd_paddr = vaddr_to_paddr(m);
        DEBUG_ASSERT(pd_paddr);

        update_pd_entry(vaddr, init_table, pd_paddr, get_x86_arch_flags(mmu_flags));
        pt = (map_addr_t)m;
    }
#endif

    /* Updating the page table entry with the paddr and access flags required for the mapping */
    update_pt_entry(vaddr, paddr, pt, get_x86_arch_flags(mmu_flags));
    ret = NO_ERROR;
#ifdef PAE_MODE_ENABLED
    goto clean;

clean_pd:
    if (pd_new)
        free((map_addr_t *)pdt);
#endif
clean:
    return ret;
}

/**
 * @brief  x86 MMU unmap an entry in the page tables recursively and clear out tables
 *
 */
static void x86_mmu_unmap_entry(vaddr_t vaddr, int level, map_addr_t table_entry)
{
    uint32_t offset = 0, next_level_offset = 0;
    map_addr_t *table, *next_table_addr, value;

    next_table_addr = NULL;
    table = (map_addr_t *)(X86_VIRT_TO_PHYS(table_entry) & X86_PG_FRAME);

    switch (level) {
#ifdef PAE_MODE_ENABLED
        case PDP_L:
            offset = ((vaddr >> PDP_SHIFT) & ((1 << PDPT_ADDR_OFFSET) - 1));
            next_table_addr = (map_addr_t *)X86_PHYS_TO_VIRT(table[offset]);
            if ((X86_PHYS_TO_VIRT(table[offset]) & X86_MMU_PG_P) == 0)
                return;
            break;
#endif
        case PD_L:
            offset = ((vaddr >> PD_SHIFT) & ((1 << ADDR_OFFSET) - 1));
            next_table_addr = (map_addr_t *)X86_PHYS_TO_VIRT(table[offset]);
            if ((X86_PHYS_TO_VIRT(table[offset]) & X86_MMU_PG_P) == 0)
                return;
            break;
        case PT_L:
            offset = ((vaddr >> PT_SHIFT) & ((1 << ADDR_OFFSET) - 1));
            next_table_addr = (map_addr_t *)X86_PHYS_TO_VIRT(table[offset]);
            if ((X86_PHYS_TO_VIRT(table[offset]) & X86_MMU_PG_P) == 0)
                return;
            break;
        case PF_L:
            /* Reached page frame, Let's go back */
        default:
            return;
    }

    level -= 1;
    x86_mmu_unmap_entry(vaddr, level, (map_addr_t)next_table_addr);
    level += 1;

    next_table_addr = (map_addr_t *)((map_addr_t)(X86_VIRT_TO_PHYS(next_table_addr)) & X86_PG_FRAME);
    if (level > PT_L) {
        /* Check all entries of next level table for present bit */
        for (next_level_offset = 0; next_level_offset < NO_OF_PT_ENTRIES; next_level_offset++) {
            if ((next_table_addr[next_level_offset] & X86_MMU_PG_P) != 0)
                return; /* There is an entry in the next level table */
        }
        free(next_table_addr);
    }
    /* All present bits for all entries in next level table for this address are 0 */
    if ((X86_PHYS_TO_VIRT(table[offset]) & X86_MMU_PG_P) != 0) {
        arch_disable_ints();
        value = table[offset];
        value = value & X86_PTE_NOT_PRESENT;
        table[offset] = value;
        arch_enable_ints();
    }
}

status_t x86_mmu_unmap(map_addr_t init_table, vaddr_t vaddr, uint count)
{
    vaddr_t next_aligned_v_addr;

    DEBUG_ASSERT(init_table);
    if (!IS_ALIGNED(vaddr, PAGE_SIZE))
        return ERR_INVALID_ARGS;

    if (count == 0)
        return NO_ERROR;

    next_aligned_v_addr = vaddr;
    while (count > 0) {
#ifdef PAE_MODE_ENABLED
        x86_mmu_unmap_entry(next_aligned_v_addr, X86_PAE_PAGING_LEVELS, init_table);
#else
        x86_mmu_unmap_entry(next_aligned_v_addr, X86_PAGING_LEVELS, init_table);
#endif
        next_aligned_v_addr += PAGE_SIZE;
        count--;
    }
    return NO_ERROR;
}

int arch_mmu_unmap(arch_aspace_t *aspace, vaddr_t vaddr, uint count)
{
    map_addr_t init_table_from_cr3;

    DEBUG_ASSERT(aspace);

    if (!IS_ALIGNED(vaddr, PAGE_SIZE))
        return ERR_INVALID_ARGS;

    if (count == 0)
        return NO_ERROR;

    DEBUG_ASSERT(x86_get_cr3());
    init_table_from_cr3 = x86_get_cr3();

    return (x86_mmu_unmap(X86_PHYS_TO_VIRT(init_table_from_cr3), vaddr, count));
}

/**
 * @brief  Mapping a section/range with specific permissions
 *
 */
status_t x86_mmu_map_range(map_addr_t init_table, struct map_range *range, arch_flags_t flags)
{
    vaddr_t next_aligned_v_addr;
    map_addr_t next_aligned_p_addr;
    status_t map_status;
    uint32_t no_of_pages, index;

    TRACEF("table 0x%x, range vaddr 0x%lx paddr 0x%lx size %u\n", init_table, range->start_vaddr, range->start_paddr, range->size);

    DEBUG_ASSERT(init_table);
    if (!range)
        return ERR_INVALID_ARGS;

    /* Calculating the number of 4k pages */
    if (IS_ALIGNED(range->size, PAGE_SIZE))
        no_of_pages = (range->size) >> PAGE_DIV_SHIFT;
    else
        no_of_pages = ((range->size) >> PAGE_DIV_SHIFT) + 1;

    next_aligned_v_addr = range->start_vaddr;
    next_aligned_p_addr = range->start_paddr;

    for (index = 0; index < no_of_pages; index++) {
        map_status = x86_mmu_add_mapping(init_table, next_aligned_p_addr, next_aligned_v_addr, flags);
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

status_t arch_mmu_query(arch_aspace_t *aspace, vaddr_t vaddr, paddr_t *paddr, uint *flags)
{
    uint32_t ret_level, current_cr3_val;
    map_addr_t last_valid_entry;
    arch_flags_t ret_flags;
    status_t stat;

    LTRACEF("aspace %p, vaddr 0x%lx, paddr %p, flags %p\n", aspace, vaddr, paddr, flags);

    DEBUG_ASSERT(aspace);

    if (!paddr)
        return ERR_INVALID_ARGS;

    DEBUG_ASSERT(x86_get_cr3());
    current_cr3_val = (map_addr_t)x86_get_cr3();

    stat = x86_mmu_get_mapping(X86_PHYS_TO_VIRT(current_cr3_val), vaddr, &ret_level, &ret_flags, &last_valid_entry);
    if (stat)
        return stat;

    *paddr = (paddr_t)last_valid_entry;

    /* converting x86 arch specific flags to arch mmu flags */
    if (flags)
        *flags = ret_flags;

    return NO_ERROR;
}

int arch_mmu_map(arch_aspace_t *aspace, vaddr_t vaddr, paddr_t paddr, uint count, uint flags)
{
    uint32_t current_cr3_val;
    struct map_range range;

    DEBUG_ASSERT(aspace);

    if ((!IS_ALIGNED(paddr, PAGE_SIZE)) || (!IS_ALIGNED(vaddr, PAGE_SIZE)))
        return ERR_INVALID_ARGS;

    if (count == 0)
        return NO_ERROR;

    DEBUG_ASSERT(x86_get_cr3());
    current_cr3_val = (map_addr_t)x86_get_cr3();

    range.start_vaddr = vaddr;
    range.start_paddr = (map_addr_t)paddr;
    range.size = count * PAGE_SIZE;

    return (x86_mmu_map_range(X86_PHYS_TO_VIRT(current_cr3_val), &range, flags));
}

void x86_mmu_early_init(void)
{
    volatile uint32_t cr0;

    /* Set WP bit in CR0*/
    cr0 = x86_get_cr0();
    cr0 |= X86_CR0_WP;
    x86_set_cr0(cr0);

#ifdef PAE_MODE_ENABLED
    volatile uint32_t efer_msr, cr4;

    /* Setting the SMEP & SMAP bit in CR4 */
    cr4 = x86_get_cr4();
    if (check_smep_avail())
        cr4 |= X86_CR4_SMEP;
    if (check_smap_avail())
        cr4 |=X86_CR4_SMAP;
    x86_set_cr4(cr4);

    /* Set NXE bit in MSR_EFER*/
    efer_msr = read_msr(x86_MSR_EFER);
    efer_msr |= x86_EFER_NXE;
    write_msr(x86_MSR_EFER, efer_msr);
#endif

    /* unmap the lower identity mapping */
    for (uint i = 0; i < (1024*1024*1024) / (4*1024*1024); i++) {
        pd[i] = 0;
    }

    /* tlb flush */
    x86_set_cr3(x86_get_cr3());
}

void x86_mmu_init(void)
{
}

/*
 * x86 does not support multiple address spaces at the moment, so fail if these apis
 * are used for it.
 */
status_t arch_mmu_init_aspace(arch_aspace_t *aspace, vaddr_t base, size_t size, uint flags)
{
    DEBUG_ASSERT(aspace);

    if ((flags & ARCH_ASPACE_FLAG_KERNEL) == 0) {
        return ERR_NOT_SUPPORTED;
    }

    return NO_ERROR;
}

status_t arch_mmu_destroy_aspace(arch_aspace_t *aspace)
{
    return NO_ERROR;
}

void arch_mmu_context_switch(arch_aspace_t *aspace)
{
    if (aspace != NULL) {
        PANIC_UNIMPLEMENTED;
    }
}

