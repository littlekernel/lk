/*
 * Copyright (c) 2008 Travis Geiselbrecht
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

#include <sys/types.h>
#include <compiler.h>

__BEGIN_CDECLS

void x86_mmu_init(void);

#define X86_MMU_PG_P		0x001		/* P    Valid			*/
#define X86_MMU_PG_RW		0x002		/* R/W  Read/Write		*/
#define X86_MMU_PG_U		0x004		/* U/S  User/Supervisor		*/
#define X86_MMU_PG_PS		0x080		/* PS   Page size (0=4k,1=2M)	*/
#define X86_MMU_PG_PTE_PAT	0x080		/* PAT  PAT index		*/
#define X86_MMU_PG_G		0x100		/* G    Global			*/
#define X86_MMU_PG_NX		(1ul << 63)	/* NX   No Execute		*/
#define x86_MMU_CLEAR		0x0
#define X86_PG_FRAME		(0x000ffffffffff000ul)
#define X86_PHY_ADDR_MASK	(0x000ffffffffffffful)
#define X86_FLAGS_MASK		(0x8000000000000ffful)

#define PAGE_SIZE		4096
#define PAGING_LEVELS		4
#define PAGE_DIV_SHIFT		12
#define PML4_SHIFT		39
#define PDP_SHIFT		30
#define PD_SHIFT		21
#define PT_SHIFT		12
#define ADDR_OFFSET		9

#define X86_PHYS_TO_VIRT(x)	(x)
#define X86_VIRT_TO_PHYS(x)	(x)
#define X86_SET_FLAG(x)		(x=1)

typedef uint64_t map_addr_t;

struct pmap {
	map_addr_t pml4_addr;
	map_addr_t vaddr;
	map_addr_t paddr;
	uint64_t flags;
};

/* Return status of the Memory mapping request */
typedef enum mapping_status {
	MAP_SUCCESS,
	MAP_NO_MAPPING,
	MAP_ERR_NO_MEM,
	MAP_ERR_INVLD_VADDR,
	MAP_ERR_INVLD_PADDR,
} map_stat;

/* Different paging levels in the page table hirerachy */
typedef enum paging_level {
	PML4_LEVEL,
	PDP_LEVEL,
	PD_LEVEL,
	PT_LEVEL,
	PAGE_FRAMES
} page_level;

struct map_range {
	map_addr_t start_vaddr;
	map_addr_t start_paddr;
	uint32_t size;
};

map_stat x86_mmu_add_mapping (map_addr_t pml4, map_addr_t paddr, map_addr_t vaddr, uint64_t flags);
map_stat x86_mmu_map_range (map_addr_t pml4, struct map_range *range, uint64_t flags);
map_stat x86_mmu_check_mapping (map_addr_t pml4, map_addr_t paddr,
				map_addr_t vaddr, uint64_t in_flags,
				uint32_t *ret_level, uint64_t *ret_flags,
				uint64_t *last_valid_entry);
__END_CDECLS
