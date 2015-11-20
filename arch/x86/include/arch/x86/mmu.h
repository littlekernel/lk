/*
 * Copyright (c) 2008 Travis Geiselbrecht
 * Copyright (c) 2015 Intel Corporation
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

#define X86_MMU_PG_P        0x001           /* P    Valid                   */
#define X86_MMU_PG_RW       0x002           /* R/W  Read/Write              */
#define X86_MMU_PG_U        0x004           /* U/S  User/Supervisor         */
#define X86_MMU_PG_PS       0x080           /* PS   Page size (0=4k,1=4M)   */
#define X86_MMU_PG_PTE_PAT  0x080           /* PAT  PAT index               */
#define X86_MMU_PG_G        0x100           /* G    Global                  */
#define X86_MMU_CLEAR       0x0
#define X86_DIRTY_ACCESS_MASK   0xf9f
#define X86_MMU_CACHE_DISABLE   0x010       /* C Cache disable */

#define PAGE_SIZE       4096
#define PAGE_DIV_SHIFT      12

#ifdef PAE_MODE_ENABLED
/* PAE mode */
#define X86_PDPT_ADDR_MASK  (0x00000000ffffffe0ul)
#define X86_PG_FRAME        (0x000ffffffffff000ul)
#define X86_PHY_ADDR_MASK   (0x000ffffffffffffful)
#define X86_FLAGS_MASK      (0x0000000000000ffful)  /* NX Bit is ignored in the PAE mode */
#define X86_PTE_NOT_PRESENT (0xFFFFFFFFFFFFFFFEul)
#define X86_2MB_PAGE_FRAME  (0x000fffffffe00000ul)
#define PAGE_OFFSET_MASK_4KB    (0x0000000000000ffful)
#define PAGE_OFFSET_MASK_2MB    (0x00000000001ffffful)
#define X86_MMU_PG_NX       (1ul << 63)
#define X86_PAE_PAGING_LEVELS   3
#define PDP_SHIFT       30
#define PD_SHIFT        21
#define PT_SHIFT        12
#define ADDR_OFFSET     9
#define PDPT_ADDR_OFFSET    2
#define NO_OF_PT_ENTRIES    512

#else
/* non PAE mode */
#define X86_PG_FRAME        (0xfffff000)
#define X86_FLAGS_MASK          (0x00000fff)
#define X86_PTE_NOT_PRESENT     (0xfffffffe)
#define X86_4MB_PAGE_FRAME      (0xffc00000)
#define PAGE_OFFSET_MASK_4KB    (0x00000fff)
#define PAGE_OFFSET_MASK_4MB    (0x003fffff)
#define NO_OF_PT_ENTRIES    1024
#define X86_PAGING_LEVELS   2
#define PD_SHIFT        22
#define PT_SHIFT        12
#define ADDR_OFFSET     10

#endif

#define X86_PHYS_TO_VIRT(x)     (x)
#define X86_VIRT_TO_PHYS(x)     (x)

/* Different page table levels in the page table mgmt hirerachy */
enum page_table_levels {
    PF_L,
    PT_L,
    PD_L,
#ifdef PAE_MODE_ENABLED
    PDP_L
#endif
} page_level;


struct map_range {
    vaddr_t start_vaddr;
#ifdef PAE_MODE_ENABLED
    uint64_t start_paddr; /* Physical address in the PAE mode is 64 bits wide */
#else
    paddr_t start_paddr; /* Physical address in the PAE mode is 32 bits wide */
#endif
    uint32_t size;
};

#ifdef PAE_MODE_ENABLED
typedef uint64_t map_addr_t;
typedef uint64_t arch_flags_t;
#else
typedef uint32_t map_addr_t;
typedef uint32_t arch_flags_t;
#endif

status_t x86_mmu_map_range (map_addr_t pdpt, struct map_range *range, arch_flags_t flags);
status_t x86_mmu_add_mapping(map_addr_t init_table, map_addr_t paddr,
                             vaddr_t vaddr, arch_flags_t flags);
status_t x86_mmu_get_mapping(map_addr_t init_table, vaddr_t vaddr, uint32_t *ret_level,
                             arch_flags_t *mmu_flags, map_addr_t *last_valid_entry);
status_t x86_mmu_unmap(map_addr_t init_table, vaddr_t vaddr, uint count);
addr_t *x86_create_new_cr3(void);
map_addr_t get_kernel_cr3(void);

__END_CDECLS
