/*
 * Copyright (c) 2014 Travis Geiselbrecht
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
#pragma once

/* some assembly #defines, need to match the structure below */
#if IS_64BIT
#define __MMU_INITIAL_MAPPING_PHYS_OFFSET 0
#define __MMU_INITIAL_MAPPING_VIRT_OFFSET 8
#define __MMU_INITIAL_MAPPING_SIZE_OFFSET 16
#define __MMU_INITIAL_MAPPING_FLAGS_OFFSET 24
#define __MMU_INITIAL_MAPPING_SIZE        40
#else
#define __MMU_INITIAL_MAPPING_PHYS_OFFSET 0
#define __MMU_INITIAL_MAPPING_VIRT_OFFSET 4
#define __MMU_INITIAL_MAPPING_SIZE_OFFSET 8
#define __MMU_INITIAL_MAPPING_FLAGS_OFFSET 12
#define __MMU_INITIAL_MAPPING_SIZE        20
#endif

/* flags for initial mapping struct */
#define MMU_INITIAL_MAPPING_TEMPORARY     (0x1)
#define MMU_INITIAL_MAPPING_FLAG_UNCACHED (0x2)
#define MMU_INITIAL_MAPPING_FLAG_DEVICE   (0x4)
#define MMU_INITIAL_MAPPING_FLAG_DYNAMIC  (0x8)  /* entry has to be patched up by platform_reset */

#ifndef ASSEMBLY

#include <sys/types.h>
#include <stdint.h>
#include <compiler.h>
#include <list.h>
#include <stdlib.h>
#include <arch.h>
#include <arch/mmu.h>

__BEGIN_CDECLS

#define PAGE_ALIGN(x) ALIGN(x, PAGE_SIZE)
#define IS_PAGE_ALIGNED(x) IS_ALIGNED(x, PAGE_SIZE)

struct mmu_initial_mapping {
    paddr_t phys;
    vaddr_t virt;
    size_t  size;
    unsigned int flags;
    const char *name;
};

/* Assert that the assembly macros above match this struct. */
STATIC_ASSERT(__offsetof(struct mmu_initial_mapping, phys) == __MMU_INITIAL_MAPPING_PHYS_OFFSET);
STATIC_ASSERT(__offsetof(struct mmu_initial_mapping, virt) == __MMU_INITIAL_MAPPING_VIRT_OFFSET);
STATIC_ASSERT(__offsetof(struct mmu_initial_mapping, size) == __MMU_INITIAL_MAPPING_SIZE_OFFSET);
STATIC_ASSERT(__offsetof(struct mmu_initial_mapping, flags) == __MMU_INITIAL_MAPPING_FLAGS_OFFSET);
STATIC_ASSERT(sizeof(struct mmu_initial_mapping) == __MMU_INITIAL_MAPPING_SIZE);

/* Platform or target must fill out one of these to set up the initial memory map
 * for kernel and enough IO space to boot.
 */
extern struct mmu_initial_mapping mmu_initial_mappings[];

/* core per page structure */
typedef struct vm_page {
    struct list_node node;

    uint flags : 8;
    uint ref : 24;
} vm_page_t;

#define VM_PAGE_FLAG_NONFREE  (0x1)

/* kernel address space */
#ifndef KERNEL_ASPACE_BASE
#define KERNEL_ASPACE_BASE ((vaddr_t)0x80000000UL)
#endif
#ifndef KERNEL_ASPACE_SIZE
#define KERNEL_ASPACE_SIZE ((vaddr_t)0x80000000UL)
#endif

STATIC_ASSERT(KERNEL_ASPACE_BASE + (KERNEL_ASPACE_SIZE - 1) > KERNEL_ASPACE_BASE);

static inline bool is_kernel_address(vaddr_t va)
{
    return (va >= (vaddr_t)KERNEL_ASPACE_BASE && va <= ((vaddr_t)KERNEL_ASPACE_BASE + ((vaddr_t)KERNEL_ASPACE_SIZE - 1)));
}

/* user address space, defaults to below kernel space with a 16MB guard gap on either side */
#ifndef USER_ASPACE_BASE
#define USER_ASPACE_BASE ((vaddr_t)0x01000000UL)
#endif
#ifndef USER_ASPACE_SIZE
#define USER_ASPACE_SIZE ((vaddr_t)KERNEL_ASPACE_BASE - USER_ASPACE_BASE - 0x01000000UL)
#endif

STATIC_ASSERT(USER_ASPACE_BASE + (USER_ASPACE_SIZE - 1) > USER_ASPACE_BASE);

static inline bool is_user_address(vaddr_t va)
{
    return (va >= USER_ASPACE_BASE && va <= (USER_ASPACE_BASE + (USER_ASPACE_SIZE - 1)));
}

/* physical allocator */
typedef struct pmm_arena {
    struct list_node node;
    const char *name;

    uint flags;
    uint priority;

    paddr_t base;
    size_t  size;

    size_t free_count;

    struct vm_page *page_array;
    struct list_node free_list;
} pmm_arena_t;

#define PMM_ARENA_FLAG_KMAP (0x1) /* this arena is already mapped and useful for kallocs */

/* Add a pre-filled memory arena to the physical allocator. */
status_t pmm_add_arena(pmm_arena_t *arena) __NONNULL((1));

/* Allocate count pages of physical memory, adding to the tail of the passed list.
 * The list must be initialized.
 * Returns the number of pages allocated.
 */
size_t pmm_alloc_pages(uint count, struct list_node *list) __NONNULL((2));

/* Allocate a specific range of physical pages, adding to the tail of the passed list.
 * The list must be initialized.
 * Returns the number of pages allocated.
 */
size_t pmm_alloc_range(paddr_t address, uint count, struct list_node *list) __NONNULL((3));

/* Free a list of physical pages.
 * Returns the number of pages freed.
 */
size_t pmm_free(struct list_node *list) __NONNULL((1));

/* Helper routine for the above. */
size_t pmm_free_page(vm_page_t *page) __NONNULL((1));

/* Allocate a run of contiguous pages, aligned on log2 byte boundary (0-31)
 * If the optional physical address pointer is passed, return the address.
 * If the optional list is passed, append the allocate page structures to the tail of the list.
 */
size_t pmm_alloc_contiguous(uint count, uint8_t align_log2, paddr_t *pa, struct list_node *list);

/* Allocate a run of pages out of the kernel area and return the pointer in kernel space.
 * If the optional list is passed, append the allocate page structures to the tail of the list.
 */
void *pmm_alloc_kpages(uint count, struct list_node *list);

/* Helper routine for pmm_alloc_kpages. */
static inline void *pmm_alloc_kpage(void) { return pmm_alloc_kpages(1, NULL); }

size_t pmm_free_kpages(void *ptr, uint count);

/* physical to virtual */
void *paddr_to_kvaddr(paddr_t pa);

/* a hint as to which virtual addresses will be returned by pmm_alloc_kpages */
void *kvaddr_get_range(size_t* size_return);

/* virtual to physical */
paddr_t vaddr_to_paddr(void *va);

/* vm_page_t to physical address */
paddr_t vm_page_to_paddr(const vm_page_t *page);

/* paddr to vm_page_t */
vm_page_t *paddr_to_vm_page(paddr_t addr);

/* virtual allocator */
typedef struct vmm_aspace {
    struct list_node node;
    char name[32];

    uint flags;

    vaddr_t base;
    size_t  size;

    struct list_node region_list;

    arch_aspace_t arch_aspace;
} vmm_aspace_t;

#define VMM_ASPACE_FLAG_KERNEL 0x1

typedef struct vmm_region {
    struct list_node node;
    char name[32];

    uint flags;
    uint arch_mmu_flags;

    vaddr_t base;
    size_t  size;

    struct list_node page_list;
} vmm_region_t;

#define VMM_REGION_FLAG_RESERVED 0x1
#define VMM_REGION_FLAG_PHYSICAL 0x2

/* grab a handle to the kernel address space */
extern vmm_aspace_t _kernel_aspace;
static inline vmm_aspace_t *vmm_get_kernel_aspace(void)
{
    return &_kernel_aspace;
}

/* virtual to container address space */
struct vmm_aspace *vaddr_to_aspace(void *ptr);

/* reserve a chunk of address space to prevent allocations from that space */
status_t vmm_reserve_space(vmm_aspace_t *aspace, const char *name, size_t size, vaddr_t vaddr)
__NONNULL((1));

/* allocate a region of virtual space that maps a physical piece of address space.
   the physical pages that back this are not allocated from the pmm. */
status_t vmm_alloc_physical(vmm_aspace_t *aspace, const char *name, size_t size, void **ptr, uint8_t align_log2, paddr_t paddr, uint vmm_flags, uint arch_mmu_flags)
__NONNULL((1));

/* allocate a region of memory backed by newly allocated contiguous physical memory  */
status_t vmm_alloc_contiguous(vmm_aspace_t *aspace, const char *name, size_t size, void **ptr, uint8_t align_log2, uint vmm_flags, uint arch_mmu_flags)
__NONNULL((1));

/* allocate a region of memory backed by newly allocated physical memory */
status_t vmm_alloc(vmm_aspace_t *aspace, const char *name, size_t size, void **ptr, uint8_t align_log2, uint vmm_flags, uint arch_mmu_flags)
__NONNULL((1));

/* Unmap previously allocated region and free physical memory pages backing it (if any) */
status_t vmm_free_region(vmm_aspace_t *aspace, vaddr_t va);

/* For the above region creation routines. Allocate virtual space at the passed in pointer. */
#define VMM_FLAG_VALLOC_SPECIFIC 0x1

/* allocate a new address space */
status_t vmm_create_aspace(vmm_aspace_t **aspace, const char *name, uint flags)
__NONNULL((1));

/* destroy everything in the address space */
status_t vmm_free_aspace(vmm_aspace_t *aspace)
__NONNULL((1));

/* internal routine by the scheduler to swap mmu contexts */
void vmm_context_switch(vmm_aspace_t *oldspace, vmm_aspace_t *newaspace);

/* set the current user aspace as active on the current thread.
   NULL is a valid argument, which unmaps the current user address space */
void vmm_set_active_aspace(vmm_aspace_t *aspace);

__END_CDECLS

#endif // !ASSEMBLY
