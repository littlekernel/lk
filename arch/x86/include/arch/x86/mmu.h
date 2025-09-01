/*
 * Copyright (c) 2008 Travis Geiselbrecht
 * Copyright (c) 2015 Intel Corporation
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <arch/defines.h>

/* top level defines for the x86 mmu */
/* NOTE: the top part can be included from assembly */
#define KB (1024UL)
#define MB (1024UL * 1024UL)
#define GB (1024UL * 1024UL * 1024UL)

#define X86_MMU_PG_P          0x001 /* P    Valid                   */
#define X86_MMU_PG_RW         0x002 /* R/W  Read/Write              */
#define X86_MMU_PG_U          0x004 /* U/S  User/Supervisor         */
#define X86_MMU_PG_PS         0x080 /* PS   Page size (0=4k,1=4M)   */
#define X86_MMU_PG_PTE_PAT    0x080 /* PAT  PAT index               */
#define X86_MMU_PG_G          0x100 /* G    Global                  */
#define X86_MMU_CLEAR         0x0
#define X86_DIRTY_ACCESS_MASK 0xf9f
#define X86_MMU_CACHE_DISABLE 0x010 /* C Cache disable */

#if !X86_LEGACY
/* default flags for inner page directory entries */
#define X86_KERNEL_PD_FLAGS (X86_MMU_PG_RW | X86_MMU_PG_P)

/* default flags for 2MB/4MB/1GB page directory entries */
#define X86_KERNEL_PD_LP_FLAGS (X86_MMU_PG_G | X86_MMU_PG_PS | X86_MMU_PG_RW | X86_MMU_PG_P)
#else
/* default flags for page dir and page table entries in legacy (386) format */
#define X86_KERNEL_PT_FLAGS (X86_MMU_PG_RW | X86_MMU_PG_P)
#endif

#define PAGE_DIV_SHIFT PAGE_SIZE_SHIFT

#if ARCH_X86_64
/* PAE mode */
#define X86_PDPT_ADDR_MASK   (0x00000000ffffffe0ul)
#define X86_PG_FRAME         (0xfffffffffffff000ul)
#define X86_PHY_ADDR_MASK    (0x000ffffffffffffful)
#define X86_FLAGS_MASK       (0x8000000000000ffful)
#define X86_PTE_NOT_PRESENT  (0xFFFFFFFFFFFFFFFEul)
#define X86_2MB_PAGE_FRAME   (0x000fffffffe00000ul)
#define PAGE_OFFSET_MASK_4KB (0x0000000000000ffful)
#define PAGE_OFFSET_MASK_2MB (0x00000000001ffffful)
#define X86_MMU_PG_NX        (1ULL << 63)

#if ARCH_X86_64
#define X86_PAGING_LEVELS 4
#define PML4_SHIFT        39
#else
#define X86_PAGING_LEVELS 3
#endif

#define PDP_SHIFT        30
#define PD_SHIFT         21
#define PT_SHIFT         12
#define ADDR_OFFSET      9
#define PDPT_ADDR_OFFSET 2
#define NO_OF_PT_ENTRIES 512

#else
/* non PAE mode */
#define X86_PG_FRAME         (0xfffff000)
#define X86_FLAGS_MASK       (0x00000fff)
#define X86_PTE_NOT_PRESENT  (0xfffffffe)
#define X86_4MB_PAGE_FRAME   (0xffc00000)
#define PAGE_OFFSET_MASK_4KB (0x00000fff)
#define PAGE_OFFSET_MASK_4MB (0x003fffff)
#define NO_OF_PT_ENTRIES     1024
#define X86_PAGING_LEVELS    2
#define PD_SHIFT             22
#define PT_SHIFT             12
#define ADDR_OFFSET          10

#endif

/* C defines below */
#ifndef ASSEMBLY

#include <lk/compiler.h>
#include <sys/types.h>

__BEGIN_CDECLS

/* Different page table levels in the page table mgmt hirerachy */
enum page_table_levels {
    PF_L,
    PT_L,
    PD_L,
#if ARCH_X86_64
    PDP_L,
    PML4_L
#endif
};

struct map_range {
    vaddr_t start_vaddr;
    paddr_t start_paddr; /* Physical address in the PAE mode is 32 bits wide */
    uint32_t size;
};

#if ARCH_X86_64
typedef uint64_t map_addr_t;
typedef uint64_t arch_flags_t;
#else
typedef uint32_t map_addr_t;
typedef uint32_t arch_flags_t;
#endif

void x86_mmu_early_init(void);
void x86_mmu_init(void);
void x86_mmu_early_init_percpu(void);

__END_CDECLS

#endif // !ASSEMBLY
