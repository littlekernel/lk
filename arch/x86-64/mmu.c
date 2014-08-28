/*
 * Copyright (c) 2009 Corey Tabaka
 * Copyright (c) 2014 Intel Corporation
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
#include <sys/types.h>
#include <compiler.h>
#include <arch.h>
#include <arch/x86.h>
#include <arch/x86/mmu.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>

/* Enable debug mode */
#define MMU_DEBUG	0

/* Address width */
extern uint32_t g_addr_width;

/**
 * @brief  check if the address is valid
 *
 */
bool x86_mmu_check_map_addr(map_addr_t address)
{
	uint64_t addr = (uint64_t)address;

	/* Check to see if the address is PAGE aligned */
	if((addr % PAGE_SIZE) != 0)
		return false;

	/* Check to see if the address in a canonical address */
	if(addr >> (g_addr_width - 1))
		if((addr >> (g_addr_width - 1)) ^ ((1ul << (64 - (g_addr_width - 1))) - 1))
			return false;

	return true;
}

uint64_t x86_mmu_check_map_flags(uint64_t flags, uint64_t entry)
{
	return ((entry & X86_FLAGS_MASK) ^ flags);
}

/**
 * To initialize a structure which holds a local context of the memory mapping
 *
 */
map_stat initialize_local_pmap_context(map_addr_t pml4, struct pmap *pmap,
				       map_addr_t paddr, map_addr_t vaddr,
				       uint64_t flags)
{
	/* Arguments checking & pml4_addr address validity */
	if(!x86_mmu_check_map_addr(vaddr))
		return MAP_ERR_INVLD_VADDR;

	if(!x86_mmu_check_map_addr(paddr))
                return MAP_ERR_INVLD_PADDR;

	pmap->pml4_addr = pml4;
	pmap->vaddr = vaddr;
	pmap->paddr = paddr;
	pmap->flags = flags;

	return MAP_SUCCESS;
}

static inline uint64_t get_pml4_entry_from_pml4_table(struct pmap *pmap)
{
	uint32_t pml4_index;
	uint64_t *pml4_table = (uint64_t *)X86_PHYS_TO_VIRT(pmap->pml4_addr);

	pml4_index = (((uint64_t)pmap->vaddr >> PML4_SHIFT) & ((1ul << ADDR_OFFSET) - 1));
	return X86_PHYS_TO_VIRT(pml4_table[pml4_index]);
}

static inline uint64_t get_pdp_entry_from_pdp_table(struct pmap *pmap, uint64_t pml4e)
{
	uint32_t pdp_index;
	uint64_t *pdpe;

	pdp_index = (((uint64_t)pmap->vaddr >> PDP_SHIFT) & ((1ul << ADDR_OFFSET) - 1));
	pdpe = (uint64_t *)(pml4e & X86_PG_FRAME);
	return X86_PHYS_TO_VIRT(pdpe[pdp_index]);
}

static inline uint64_t get_pd_entry_from_pd_table(struct pmap *pmap, uint64_t pdpe)
{
	uint32_t pd_index;
	uint64_t *pde;

	pd_index = (((uint64_t)pmap->vaddr >> PD_SHIFT) & ((1ul << ADDR_OFFSET) - 1));
	pde = (uint64_t *)(pdpe & X86_PG_FRAME);
	return X86_PHYS_TO_VIRT(pde[pd_index]);
}

static inline uint64_t get_pt_entry_from_pt_table(struct pmap *pmap, uint64_t pde)
{
	uint32_t pt_index;
	uint64_t *pte;

	pt_index = (((uint64_t)pmap->vaddr >> PT_SHIFT) & ((1ul << ADDR_OFFSET) - 1));
	pte = (uint64_t *)(pde & X86_PG_FRAME);
	return X86_PHYS_TO_VIRT(pte[pt_index]);
}

static inline uint64_t get_pfn_from_pte(uint64_t pte)
{
	uint64_t pfn;

	pfn = (pte & X86_PG_FRAME);
	return X86_PHYS_TO_VIRT(pfn);
}

void map_zero_page(uint64_t *ptr)
{
	memset(ptr, 0, PAGE_SIZE);
}

/**
 * @brief  Walk the page table structures to verify mapping
 *
 * Walk the page table structures to see if the mapping between a virtual address
 * and a physical address exists. And also to check if the mapping is valid.
 *
 * In this scenario, we are considering the paging scheme to be a PAE mode with
 * 4KB pages.
 *
 */
map_stat x86_mmu_check_mapping(map_addr_t pml4, map_addr_t paddr,
			       map_addr_t vaddr, uint64_t in_flags,
			       uint32_t *ret_level, uint64_t *ret_flags,
			       uint64_t *last_valid_entry)
{
	struct pmap pmap_local;
	uint64_t pml4e, pdpe, pde, pte;
	uint64_t pfn_after_walk;
	map_stat ret_init;
	*ret_level = PML4_LEVEL;

	ret_init = initialize_local_pmap_context(pml4, &pmap_local, paddr, vaddr, in_flags);
	if(ret_init) {
		if (MMU_DEBUG)
			dprintf(SPEW, "initialize_local_pmap_context() failed with err=%d\n", ret_init);
		return ret_init;
	}

	pml4e = get_pml4_entry_from_pml4_table(&pmap_local);
	if ((pml4e & X86_MMU_PG_P) == 0) {
		goto end;
	}

	pdpe = get_pdp_entry_from_pdp_table(&pmap_local, pml4e);
	if ((pdpe & X86_MMU_PG_P) == 0) {
                *ret_level = PDP_LEVEL;
		*last_valid_entry = pml4e;
		goto end;
        }

	pde = get_pd_entry_from_pd_table(&pmap_local, pdpe);
	if ((pde & X86_MMU_PG_P) == 0) {
                *ret_level = PD_LEVEL;
		*last_valid_entry = pdpe;
		goto end;
	}

	pte = get_pt_entry_from_pt_table(&pmap_local, pde);
	if ((pte & X86_MMU_PG_P) == 0) {
		*ret_level = PT_LEVEL;
		*last_valid_entry = pde;
		goto end;
	}

	pfn_after_walk = get_pfn_from_pte(pte);
	if (pfn_after_walk != (uint64_t)pmap_local.paddr) {
		*ret_level = PAGE_FRAMES;
		*last_valid_entry = pte;
		goto end;
	}
	*last_valid_entry = pfn_after_walk;

	/* Checking the access flags for the mapped address. If it is not zero, then
	 * the access flags are different & the return flag will have those access bits
	 * which are different set.
	 */
	*ret_flags = x86_mmu_check_map_flags(pmap_local.flags, pfn_after_walk);

	/* Mapping is found & valid if the return flag is ZERO & the last valid entry is the paddr */
	if(!(*ret_flags))
		return MAP_SUCCESS;

	end:
		return MAP_NO_MAPPING;
}

void update_pt_entry(struct pmap *pmap, uint64_t pde)
{
	uint32_t pt_index;

	uint64_t *pt_table = (uint64_t *)(pde & X86_PG_FRAME);
	pt_index = (((uint64_t)pmap->vaddr >> PT_SHIFT) & ((1ul << ADDR_OFFSET) - 1));
	pt_table[pt_index] = (uint64_t)pmap->paddr;
	pt_table[pt_index] |= pmap->flags;
}

void update_pd_entry(struct pmap *pmap, uint64_t pdpe, uint64_t *m)
{
	uint32_t pd_index;

	uint64_t *pd_table = (uint64_t *)(pdpe & X86_PG_FRAME);
	pd_index = (((uint64_t)pmap->vaddr >> PD_SHIFT) & ((1ul << ADDR_OFFSET) - 1));
	pd_table[pd_index] = (uint64_t)m;
	pd_table[pd_index] |= X86_MMU_PG_P | X86_MMU_PG_RW;
}

void update_pdp_entry(struct pmap *pmap, uint64_t pml4e, uint64_t *m)
{
	uint32_t pdp_index;

	uint64_t *pdp_table = (uint64_t *)(pml4e & X86_PG_FRAME);
	pdp_index = (((uint64_t)pmap->vaddr >> PDP_SHIFT) & ((1ul << ADDR_OFFSET) - 1));
	pdp_table[pdp_index] = (uint64_t)m;
	pdp_table[pdp_index] |= X86_MMU_PG_P | X86_MMU_PG_RW;
}

void update_pml4_entry(struct pmap *pmap, uint64_t *m)
{
	uint32_t pml4_index;
	uint64_t *pml4_table = (uint64_t *)(pmap->pml4_addr);

	pml4_index = (((uint64_t)pmap->vaddr >> PML4_SHIFT) & ((1ul << ADDR_OFFSET) - 1));
	pml4_table[pml4_index] = (uint64_t)m;
	pml4_table[pml4_index] |= X86_MMU_PG_P | X86_MMU_PG_RW;
}

/**
 * @brief Allocating a new page table
 */
uint64_t *_map_alloc(size_t size)
{
	uint64_t *page_ptr = memalign(PAGE_SIZE, size);
	if(page_ptr)
		map_zero_page(page_ptr);
	return page_ptr;
}

/**
 * @brief  Add or create a mapping between a virtual address & physical address
 *
 * In this scenario, we are considering the paging scheme to be a PAE mode with
 * 4KB pages.
 *
 */
map_stat x86_mmu_create_mapping(struct pmap *pmap)
{

	uint32_t pd_new = 0, pdp_new = 0;
	uint64_t  pml4e, pdpe, pde, *m = NULL;
	map_stat ret = MAP_SUCCESS;

	pml4e = get_pml4_entry_from_pml4_table(pmap);

	if((pml4e & X86_MMU_PG_P) == 0) {
		/* Creating a new pdp table */
		m = _map_alloc(PAGE_SIZE);
		if(m == NULL) {
			ret = MAP_ERR_NO_MEM;
			goto clean;
		}

		update_pml4_entry(pmap, m);
		pml4e = (uint64_t)m;
		X86_SET_FLAG(pdp_new);
	}

	if(!pdp_new)
		pdpe = get_pdp_entry_from_pdp_table(pmap, pml4e);

	if(pdp_new || (pdpe & X86_MMU_PG_P) == 0) {
		/* Creating a new pd table  */
		m  = _map_alloc(PAGE_SIZE);
		if(m == NULL) {
			ret = MAP_ERR_NO_MEM;
			if(pdp_new)
				goto clean_pdp;
			goto clean;
		}

		update_pdp_entry(pmap, pml4e, m);
		pdpe = (uint64_t)m;
		X86_SET_FLAG(pd_new);
	}

	if(!pd_new)
		pde = get_pd_entry_from_pd_table(pmap, pdpe);

	if(pd_new || (pde & X86_MMU_PG_P) == 0) {
		/* Creating a new pt */
		m  = _map_alloc(PAGE_SIZE);
		if(m == NULL) {
			ret = MAP_ERR_NO_MEM;
			if(pd_new)
				goto clean_pd;
			goto clean;
		}

		update_pd_entry(pmap, pdpe, m);
		pde = (uint64_t)m;
	}

	/* Updating the page table entry with the paddr and access flags required for the mapping */
	update_pt_entry(pmap, pde);
	ret = MAP_SUCCESS;
	goto clean;

	clean_pd:
		if(pd_new)
			free((uint64_t *)pdpe);

	clean_pdp:
		if(pdp_new)
			free((uint64_t *)pml4e);

	clean:
		return ret;
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
map_stat x86_mmu_add_mapping(map_addr_t pml4, map_addr_t paddr,
			     map_addr_t vaddr, uint64_t flags)
{
	struct pmap pmap_new;
	map_stat ret_init, ret_create_map;

	ret_init = initialize_local_pmap_context(pml4, &pmap_new, paddr, vaddr, flags);
	if(ret_init) {
		if (MMU_DEBUG)
			dprintf(SPEW, "initialize_local_pmap_context() failed with err=%d\n", ret_init);
		return ret_init;
	}

	ret_create_map = x86_mmu_create_mapping(&pmap_new);
	if(ret_create_map) {
		if (MMU_DEBUG)
			dprintf(SPEW, "Creating a new mapping failed with err=%d\n", ret_create_map);
		return ret_create_map;
	}
	return MAP_SUCCESS;
}

/**
 * @brief  Mapping a section/range with specific permissions
 *
 */
map_stat x86_mmu_map_range(map_addr_t pml4, struct map_range *range, uint64_t flags)
{
	map_addr_t next_aligned_v_addr, next_aligned_p_addr;
	map_stat map_status;
	uint32_t no_of_pages, index;

	/* Calculating the number of 4k pages */
	if ((range->size % PAGE_SIZE) == 0)
		no_of_pages = (range->size) >> PAGE_DIV_SHIFT;
	else
		no_of_pages = ((range->size) >> PAGE_DIV_SHIFT) + 1;

	next_aligned_v_addr = range->start_vaddr;
	next_aligned_p_addr = range->start_paddr;

	for(index = 0; index < no_of_pages; index++) {
		map_status = x86_mmu_add_mapping(pml4, next_aligned_p_addr, next_aligned_v_addr, flags);
		if(map_status)
			return map_status;
		next_aligned_v_addr += PAGE_SIZE;
		next_aligned_p_addr += PAGE_SIZE;
	}
	return MAP_SUCCESS;
}

/**
 * @brief  x86-64 MMU basic initialization
 *
 */
void x86_mmu_init(void)
{
	uint64_t efer_msr, cr0;

	/* Set WP bit in CR0*/
	cr0 = x86_get_cr0();
	cr0 |= X86_CR0_WP;
	x86_set_cr0(cr0);

	/* Set NXE bit in MSR_EFER*/
	efer_msr = read_msr(x86_MSR_EFER);
	efer_msr |= x86_EFER_NXE;
	write_msr(x86_MSR_EFER, efer_msr);
}
