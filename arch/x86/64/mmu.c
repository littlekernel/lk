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

/* Address width including virtual/physical address*/
uint8_t g_vaddr_width = 0;
uint8_t g_paddr_width = 0;

/* top level kernel page tables, initialized in start.S */
map_addr_t pml4[NO_OF_PT_ENTRIES] __ALIGNED(PAGE_SIZE);
map_addr_t pdp[NO_OF_PT_ENTRIES] __ALIGNED(PAGE_SIZE); /* temporary */
map_addr_t pte[NO_OF_PT_ENTRIES] __ALIGNED(PAGE_SIZE);

/* top level pdp needed to map the -512GB..0 space */
map_addr_t pdp_high[NO_OF_PT_ENTRIES] __ALIGNED(PAGE_SIZE);

/* a big pile of page tables needed to map 64GB of memory into kernel space using 2MB pages */
map_addr_t linear_map_pdp[(64ULL*GB) / (2*MB)];

/**
 * @brief  check if the virtual address is aligned and canonical
 *
 */
static bool x86_mmu_check_vaddr(vaddr_t vaddr)
{
    uint64_t addr = (uint64_t)vaddr;
    uint64_t max_vaddr_lohalf,
             min_vaddr_hihalf;

    /* Check to see if the address is PAGE aligned */
    if (!IS_ALIGNED(addr, PAGE_SIZE))
        return false;

    /* get max address in lower-half canonical addr space */
    /* e.g. if width is 48, then 0x00007FFF_FFFFFFFF */
    max_vaddr_lohalf = ((uint64_t)1ull << (g_vaddr_width - 1)) - 1;

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
static bool x86_mmu_check_paddr(paddr_t paddr)
{
    uint64_t addr = (uint64_t)paddr;
    uint64_t max_paddr;

    /* Check to see if the address is PAGE aligned */
    if (!IS_ALIGNED(addr, PAGE_SIZE))
        return false;

    max_paddr = ((uint64_t)1ull << g_paddr_width) - 1;

    return addr <= max_paddr;
}


static inline uint64_t get_pml4_entry_from_pml4_table(vaddr_t vaddr, addr_t pml4_addr)
{
    uint32_t pml4_index;
    uint64_t *pml4_table = (uint64_t *)pml4_addr;

    pml4_index = (((uint64_t)vaddr >> PML4_SHIFT) & ((1ul << ADDR_OFFSET) - 1));
    return X86_PHYS_TO_VIRT(pml4_table[pml4_index]);
}

static inline uint64_t get_pdp_entry_from_pdp_table(vaddr_t vaddr, uint64_t pml4e)
{
    uint32_t pdp_index;
    uint64_t *pdpe;

    pdp_index = (((uint64_t)vaddr >> PDP_SHIFT) & ((1ul << ADDR_OFFSET) - 1));
    pdpe = (uint64_t *)(pml4e & X86_PG_FRAME);
    return X86_PHYS_TO_VIRT(pdpe[pdp_index]);
}

static inline uint64_t get_pd_entry_from_pd_table(vaddr_t vaddr, uint64_t pdpe)
{
    uint32_t pd_index;
    uint64_t *pde;

    pd_index = (((uint64_t)vaddr >> PD_SHIFT) & ((1ul << ADDR_OFFSET) - 1));
    pde = (uint64_t *)(pdpe & X86_PG_FRAME);
    return X86_PHYS_TO_VIRT(pde[pd_index]);
}

static inline uint64_t get_pt_entry_from_pt_table(vaddr_t vaddr, uint64_t pde)
{
    uint32_t pt_index;
    uint64_t *pte;

    pt_index = (((uint64_t)vaddr >> PT_SHIFT) & ((1ul << ADDR_OFFSET) - 1));
    pte = (uint64_t *)(pde & X86_PG_FRAME);
    return X86_PHYS_TO_VIRT(pte[pt_index]);
}

static inline uint64_t get_pfn_from_pte(uint64_t pte)
{
    uint64_t pfn;

    pfn = (pte & X86_PG_FRAME);
    return pfn;
}

static inline uint64_t get_pfn_from_pde(uint64_t pde)
{
    uint64_t pfn;

    pfn = (pde & X86_2MB_PAGE_FRAME);

    LTRACEF_LEVEL(2, "pde 0x%llx, pfn 0x%llx\n", pde, pfn);

    return pfn;
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

    if (flags & ARCH_MMU_FLAG_PERM_NO_EXECUTE)
        arch_flags |= X86_MMU_PG_NX;

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
status_t x86_mmu_get_mapping(map_addr_t pml4, vaddr_t vaddr, uint32_t *ret_level,
                                    arch_flags_t *mmu_flags, map_addr_t *last_valid_entry)
{
    uint64_t pml4e, pdpe, pde, pte;

    DEBUG_ASSERT(pml4);
    if ((!ret_level) || (!last_valid_entry) || (!mmu_flags)) {
        return ERR_INVALID_ARGS;
    }

    *ret_level = PML4_L;
    *last_valid_entry = pml4;
    *mmu_flags = 0;

    LTRACEF_LEVEL(2, "pml4 0x%llx\n", pml4);

    pml4e = get_pml4_entry_from_pml4_table(vaddr, pml4);
    if ((pml4e & X86_MMU_PG_P) == 0) {
        return ERR_NOT_FOUND;
    }
    LTRACEF_LEVEL(2, "pml4e 0x%llx\n", pml4e);

    pdpe = get_pdp_entry_from_pdp_table(vaddr, pml4e);
    if ((pdpe & X86_MMU_PG_P) == 0) {
        *ret_level = PDP_L;
        *last_valid_entry = pml4e;
        return ERR_NOT_FOUND;
    }
    LTRACEF_LEVEL(2, "pdpe 0x%llx\n", pdpe);

    pde = get_pd_entry_from_pd_table(vaddr, pdpe);
    if ((pde & X86_MMU_PG_P) == 0) {
        *ret_level = PD_L;
        *last_valid_entry = pdpe;
        return ERR_NOT_FOUND;
    }
    LTRACEF_LEVEL(2, "pde 0x%llx\n", pde);

    /* 2 MB pages */
    if (pde & X86_MMU_PG_PS) {
        /* Getting the Page frame & adding the 4KB page offset from the vaddr */
        *last_valid_entry = get_pfn_from_pde(X86_VIRT_TO_PHYS(pde)) + ((uint64_t)vaddr & PAGE_OFFSET_MASK_2MB);
        *mmu_flags = get_arch_mmu_flags(pde & X86_FLAGS_MASK);
        goto last;
    }

    /* 4 KB pages */
    pte = get_pt_entry_from_pt_table(vaddr, pde);
    if ((pte & X86_MMU_PG_P) == 0) {
        *ret_level = PT_L;
        *last_valid_entry = pde;
        return ERR_NOT_FOUND;
    }

    /* Getting the Page frame & adding the 4KB page offset from the vaddr */
    *last_valid_entry = get_pfn_from_pte(X86_VIRT_TO_PHYS(pte)) + ((uint64_t)vaddr & PAGE_OFFSET_MASK_4KB);
    *mmu_flags = get_arch_mmu_flags(pte & X86_FLAGS_MASK);

last:
    *ret_level = PF_L;
    return NO_ERROR;
}

/**
 * Walk the page table structures to see if the mapping between a virtual address
 * and a physical address exists. Also, check the flags.
 *
 */
status_t x86_mmu_check_mapping(addr_t pml4, paddr_t paddr,
                               vaddr_t vaddr, arch_flags_t in_flags,
                               uint32_t *ret_level, arch_flags_t *ret_flags,
                               map_addr_t *last_valid_entry)
{
    status_t status;
    arch_flags_t existing_flags = 0;

    DEBUG_ASSERT(pml4);
    if ((!ret_level) || (!last_valid_entry) || (!ret_flags) ||
            (!x86_mmu_check_vaddr(vaddr)) ||
            (!x86_mmu_check_paddr(paddr))) {
        return ERR_INVALID_ARGS;
    }

    status = x86_mmu_get_mapping(pml4, vaddr, ret_level, &existing_flags, last_valid_entry);
    if (status || ((*last_valid_entry) != (uint64_t)paddr)) {
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

static void update_pt_entry(vaddr_t vaddr, paddr_t paddr,  uint64_t pde, arch_flags_t flags)
{
    uint32_t pt_index;

    uint64_t *pt_table = (uint64_t *)(pde & X86_PG_FRAME);
    pt_index = (((uint64_t)vaddr >> PT_SHIFT) & ((1ul << ADDR_OFFSET) - 1));
    pt_table[pt_index] = (uint64_t)paddr;
    pt_table[pt_index] |= flags | X86_MMU_PG_P;
    if (!(flags & X86_MMU_PG_U))
        pt_table[pt_index] |= X86_MMU_PG_G; /* setting global flag for kernel pages */
}

static void update_pd_entry(vaddr_t vaddr, uint64_t pdpe, map_addr_t m, arch_flags_t flags)
{
    uint32_t pd_index;

    uint64_t *pd_table = (uint64_t *)(pdpe & X86_PG_FRAME);
    pd_index = (((uint64_t)vaddr >> PD_SHIFT) & ((1ul << ADDR_OFFSET) - 1));
    pd_table[pd_index] = m;
    pd_table[pd_index] |= X86_MMU_PG_P | X86_MMU_PG_RW;
    if (flags & X86_MMU_PG_U)
        pd_table[pd_index] |= X86_MMU_PG_U;
    else
        pd_table[pd_index] |= X86_MMU_PG_G; /* setting global flag for kernel pages */
}

static void update_pdp_entry(vaddr_t vaddr, uint64_t pml4e, map_addr_t m, arch_flags_t flags)
{
    uint32_t pdp_index;

    uint64_t *pdp_table = (uint64_t *)(pml4e & X86_PG_FRAME);
    pdp_index = (((uint64_t)vaddr >> PDP_SHIFT) & ((1ul << ADDR_OFFSET) - 1));
    pdp_table[pdp_index] = m;
    pdp_table[pdp_index] |= X86_MMU_PG_P | X86_MMU_PG_RW;
    if (flags & X86_MMU_PG_U)
        pdp_table[pdp_index] |= X86_MMU_PG_U;
    else
        pdp_table[pdp_index] |= X86_MMU_PG_G; /* setting global flag for kernel pages */
}

static void update_pml4_entry(vaddr_t vaddr, addr_t pml4_addr, map_addr_t m, arch_flags_t flags)
{
    uint32_t pml4_index;
    uint64_t *pml4_table = (uint64_t *)(pml4_addr);

    pml4_index = (((uint64_t)vaddr >> PML4_SHIFT) & ((1ul << ADDR_OFFSET) - 1));
    pml4_table[pml4_index] = m;
    pml4_table[pml4_index] |= X86_MMU_PG_P | X86_MMU_PG_RW;
    if (flags & X86_MMU_PG_U)
        pml4_table[pml4_index] |= X86_MMU_PG_U;
    else
        pml4_table[pml4_index] |= X86_MMU_PG_G; /* setting global flag for kernel pages */
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
 * In this scenario, we are considering the paging scheme to be a PAE mode with
 * 4KB pages.
 *
 */
status_t x86_mmu_add_mapping(map_addr_t pml4, map_addr_t paddr,
                             vaddr_t vaddr, arch_flags_t mmu_flags)
{
    uint32_t pd_new = 0, pdp_new = 0;
    uint64_t pml4e, pdpe, pde;
    map_addr_t *m = NULL;
    status_t ret = NO_ERROR;

    LTRACEF("pml4 0x%llx paddr 0x%llx vaddr 0x%lx flags 0x%llx\n", pml4, paddr, vaddr, mmu_flags);

    DEBUG_ASSERT(pml4);
    if ((!x86_mmu_check_vaddr(vaddr)) || (!x86_mmu_check_paddr(paddr)) )
        return ERR_INVALID_ARGS;

    pml4e = get_pml4_entry_from_pml4_table(vaddr, pml4);

    if ((pml4e & X86_MMU_PG_P) == 0) {
        /* Creating a new pdp table */
        m = _map_alloc_page();
        if (m == NULL) {
            ret = ERR_NO_MEMORY;
            goto clean;
        }

        update_pml4_entry(vaddr, pml4, X86_VIRT_TO_PHYS(m), get_x86_arch_flags(mmu_flags));
        pml4e = (uint64_t)m;
        X86_SET_FLAG(pdp_new);
    }

    if (!pdp_new)
        pdpe = get_pdp_entry_from_pdp_table(vaddr, pml4e);

    if (pdp_new || (pdpe & X86_MMU_PG_P) == 0) {
        /* Creating a new pd table  */
        m  = _map_alloc_page();
        if (m == NULL) {
            ret = ERR_NO_MEMORY;
            if (pdp_new)
                goto clean_pdp;
            goto clean;
        }

        update_pdp_entry(vaddr, pml4e, X86_VIRT_TO_PHYS(m), get_x86_arch_flags(mmu_flags));
        pdpe = (uint64_t)m;
        X86_SET_FLAG(pd_new);
    }

    if (!pd_new)
        pde = get_pd_entry_from_pd_table(vaddr, pdpe);

    if (pd_new || (pde & X86_MMU_PG_P) == 0) {
        /* Creating a new pt */
        m  = _map_alloc_page();
        if (m == NULL) {
            ret = ERR_NO_MEMORY;
            if (pd_new)
                goto clean_pd;
            goto clean;
        }

        update_pd_entry(vaddr, pdpe, X86_VIRT_TO_PHYS(m), get_x86_arch_flags(mmu_flags));
        pde = (uint64_t)m;
    }

    /* Updating the page table entry with the paddr and access flags required for the mapping */
    update_pt_entry(vaddr, paddr, pde, get_x86_arch_flags(mmu_flags));
    ret = NO_ERROR;
    goto clean;

clean_pd:
    if (pd_new)
        pmm_free_page(paddr_to_vm_page(X86_PHYS_TO_VIRT(pd_new)));

clean_pdp:
    if (pdp_new)
        pmm_free_page(paddr_to_vm_page(X86_PHYS_TO_VIRT(pml4e)));

clean:
    return ret;
}

/**
 * @brief  x86-64 MMU unmap an entry in the page tables recursively and clear out tables
 *
 */
static void x86_mmu_unmap_entry(vaddr_t vaddr, int level, vaddr_t table_entry)
{
    uint32_t offset = 0, next_level_offset = 0;
    vaddr_t *table, *next_table_addr, value;

    LTRACEF("vaddr 0x%lx level %d table_entry 0x%lx\n", vaddr, level, table_entry);

    next_table_addr = NULL;
    table = (vaddr_t *)(table_entry & X86_PG_FRAME);
    LTRACEF_LEVEL(2, "table %p\n", table);

    switch (level) {
        case PML4_L:
            offset = (((uint64_t)vaddr >> PML4_SHIFT) & ((1ul << ADDR_OFFSET) - 1));
            LTRACEF_LEVEL(2, "offset %u\n", offset);
            next_table_addr = (vaddr_t *)X86_PHYS_TO_VIRT(table[offset]);
            LTRACEF_LEVEL(2, "next_table_addr %p\n", next_table_addr);
            if ((X86_PHYS_TO_VIRT(table[offset]) & X86_MMU_PG_P)== 0)
                return;
            break;
        case PDP_L:
            offset = (((uint64_t)vaddr >> PDP_SHIFT) & ((1ul << ADDR_OFFSET) - 1));
            LTRACEF_LEVEL(2, "offset %u\n", offset);
            next_table_addr = (vaddr_t *)X86_PHYS_TO_VIRT(table[offset]);
            LTRACEF_LEVEL(2, "next_table_addr %p\n", next_table_addr);
            if ((X86_PHYS_TO_VIRT(table[offset]) & X86_MMU_PG_P) == 0)
                return;
            break;
        case PD_L:
            offset = (((uint64_t)vaddr >> PD_SHIFT) & ((1ul << ADDR_OFFSET) - 1));
            LTRACEF_LEVEL(2, "offset %u\n", offset);
            next_table_addr = (vaddr_t *)X86_PHYS_TO_VIRT(table[offset]);
            LTRACEF_LEVEL(2, "next_table_addr %p\n", next_table_addr);
            if ((X86_PHYS_TO_VIRT(table[offset]) & X86_MMU_PG_P) == 0)
                return;
            break;
        case PT_L:
            offset = (((uint64_t)vaddr >> PT_SHIFT) & ((1ul << ADDR_OFFSET) - 1));
            LTRACEF_LEVEL(2, "offset %u\n", offset);
            next_table_addr = (vaddr_t *)X86_PHYS_TO_VIRT(table[offset]);
            LTRACEF_LEVEL(2, "next_table_addr %p\n", next_table_addr);
            if ((X86_PHYS_TO_VIRT(table[offset]) & X86_MMU_PG_P) == 0)
                return;
            break;
        case PF_L:
            /* Reached page frame, Let's go back */
        default:
            return;
    }

    LTRACEF_LEVEL(2, "recursing\n");

    level -= 1;
    x86_mmu_unmap_entry(vaddr, level, (vaddr_t)next_table_addr);
    level += 1;

    LTRACEF_LEVEL(2, "next_table_addr %p\n", next_table_addr);

    next_table_addr = (vaddr_t *)((vaddr_t)(next_table_addr) & X86_PG_FRAME);
    if (level > PT_L) {
        /* Check all entries of next level table for present bit */
        for (next_level_offset = 0; next_level_offset < (PAGE_SIZE/8); next_level_offset++) {
            if ((next_table_addr[next_level_offset] & X86_MMU_PG_P) != 0)
                return; /* There is an entry in the next level table */
        }
        pmm_free_page(paddr_to_vm_page(X86_VIRT_TO_PHYS(next_table_addr)));
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

status_t x86_mmu_unmap(map_addr_t pml4, vaddr_t vaddr, uint count)
{
    vaddr_t next_aligned_v_addr;

    DEBUG_ASSERT(pml4);
    if (!(x86_mmu_check_vaddr(vaddr)))
        return ERR_INVALID_ARGS;

    if (count == 0)
        return NO_ERROR;

    next_aligned_v_addr = vaddr;
    while (count > 0) {
        x86_mmu_unmap_entry(next_aligned_v_addr, X86_PAGING_LEVELS, pml4);
        next_aligned_v_addr += PAGE_SIZE;
        count--;
    }
    return NO_ERROR;
}

int arch_mmu_unmap(arch_aspace_t *aspace, vaddr_t vaddr, uint count)
{
    addr_t current_cr3_val;

    LTRACEF("aspace %p, vaddr 0x%lx, count %u\n", aspace, vaddr, count);

    DEBUG_ASSERT(aspace);

    if (!(x86_mmu_check_vaddr(vaddr)))
        return ERR_INVALID_ARGS;

    if (count == 0)
        return NO_ERROR;

    DEBUG_ASSERT(x86_get_cr3());
    current_cr3_val = (addr_t)x86_get_cr3();

    return (x86_mmu_unmap(X86_PHYS_TO_VIRT(current_cr3_val), vaddr, count));
}

/**
 * @brief  Mapping a section/range with specific permissions
 *
 */
status_t x86_mmu_map_range(map_addr_t pml4, struct map_range *range, arch_flags_t flags)
{
    vaddr_t next_aligned_v_addr;
    paddr_t next_aligned_p_addr;
    status_t map_status;
    uint32_t no_of_pages, index;

    LTRACEF("pml4 0x%llx, range v 0x%lx p 0x%llx size %u flags 0x%llx\n", pml4,
        range->start_vaddr, range->start_paddr, range->size, flags);

    DEBUG_ASSERT(pml4);
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
        map_status = x86_mmu_add_mapping(pml4, next_aligned_p_addr, next_aligned_v_addr, flags);
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

status_t arch_mmu_query(arch_aspace_t *aspace, vaddr_t vaddr, paddr_t *paddr, uint *flags)
{
    addr_t current_cr3_val;
    uint32_t ret_level;
    map_addr_t last_valid_entry;
    arch_flags_t ret_flags;
    status_t stat;

    LTRACEF("aspace %p, vaddr 0x%lx, paddr %p, flags %p\n", aspace, vaddr, paddr, flags);

    DEBUG_ASSERT(aspace);

    if (!paddr)
        return ERR_INVALID_ARGS;

    DEBUG_ASSERT(x86_get_cr3());
    current_cr3_val = (addr_t)x86_get_cr3();

    stat = x86_mmu_get_mapping(X86_PHYS_TO_VIRT(current_cr3_val), vaddr, &ret_level, &ret_flags, &last_valid_entry);
    if (stat)
        return stat;

    *paddr = (paddr_t)(last_valid_entry);
    LTRACEF("paddr 0x%llx\n", last_valid_entry);

    /* converting x86 arch specific flags to arch mmu flags */
    if (flags)
        *flags = ret_flags;

    return NO_ERROR;
}

int arch_mmu_map(arch_aspace_t *aspace, vaddr_t vaddr, paddr_t paddr, uint count, uint flags)
{
    addr_t current_cr3_val;
    struct map_range range;

    DEBUG_ASSERT(aspace);

    LTRACEF("aspace %p, vaddr 0x%lx paddr 0x%lx count %u flags 0x%x\n", aspace, vaddr, paddr, count, flags);

    if ((!x86_mmu_check_paddr(paddr)))
        return ERR_INVALID_ARGS;

    if (!x86_mmu_check_vaddr(vaddr))
        return ERR_INVALID_ARGS;

    if (count == 0)
        return NO_ERROR;

    DEBUG_ASSERT(x86_get_cr3());
    current_cr3_val = (addr_t)x86_get_cr3();

    range.start_vaddr = vaddr;
    range.start_paddr = paddr;
    range.size = count * PAGE_SIZE;

    return (x86_mmu_map_range(X86_PHYS_TO_VIRT(current_cr3_val), &range, flags));
}

void x86_mmu_early_init(void)
{
    volatile uint64_t efer_msr, cr0, cr4;

    /* Set WP bit in CR0*/
    cr0 = x86_get_cr0();
    cr0 |= X86_CR0_WP;
    x86_set_cr0(cr0);

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

    /* getting the address width from CPUID instr */
    /* Bits 07-00: Physical Address width info */
    /* Bits 15-08: Linear Address width info */
    uint32_t addr_width    = x86_get_address_width();
    g_paddr_width = (uint8_t)(addr_width & 0xFF);
    g_vaddr_width = (uint8_t)((addr_width >> 8) & 0xFF);

    LTRACEF("paddr_width %u vaddr_width %u\n", g_paddr_width, g_vaddr_width);

    /* unmap the lower identity mapping */
    pml4[0] = 0;

    /* tlb flush */
    x86_set_cr3(x86_get_cr3());
}

void x86_mmu_init(void)
{
}

/*
 * x86-64 does not support multiple address spaces at the moment, so fail if these apis
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

