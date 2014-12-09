/*
 * Copyright (c) 2008-2014 Travis Geiselbrecht
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
#include <stdlib.h>
#include <sys/types.h>
#include <err.h>
#include <string.h>
#include <compiler.h>
#include <arch.h>
#include <arch/ops.h>
#include <arch/mmu.h>
#include <arch/arm.h>
#include <arch/arm/mmu.h>
#include <kernel/vm.h>

#define LOCAL_TRACE 0

#if ARM_WITH_MMU

#define IS_SECTION_ALIGNED(x) IS_ALIGNED(x, SECTION_SIZE)
#define IS_SUPERSECTION_ALIGNED(x) IS_ALIGNED(x, SUPERSECTION_SIZE)

/* locals */
static void arm_mmu_map_section(addr_t paddr, addr_t vaddr, uint flags);
static void arm_mmu_unmap_section(addr_t vaddr);

/* the main translation table */
uint32_t arm_kernel_translation_table[4096] __ALIGNED(16384) __SECTION(".bss.prebss.translation_table");

/* convert user level mmu flags to flags that go in L1 descriptors */
static uint32_t mmu_flags_to_l1_arch_flags(uint flags)
{
    uint32_t arch_flags = 0;
    switch (flags & ARCH_MMU_FLAG_CACHE_MASK) {
        case ARCH_MMU_FLAG_CACHED:
            arch_flags |= MMU_MEMORY_L1_TYPE_NORMAL_WRITE_BACK_ALLOCATE;
            break;
        case ARCH_MMU_FLAG_UNCACHED:
            arch_flags |= MMU_MEMORY_L1_TYPE_STRONGLY_ORDERED;
            break;
        case ARCH_MMU_FLAG_UNCACHED_DEVICE:
            arch_flags |= MMU_MEMORY_L1_TYPE_DEVICE_SHARED;
            break;
        default:
            /* invalid user-supplied flag */
            DEBUG_ASSERT(1);
            return ERR_INVALID_ARGS;
    }

    switch (flags & (ARCH_MMU_FLAG_PERM_USER | ARCH_MMU_FLAG_PERM_RO)) {
        case 0:
            arch_flags |= MMU_MEMORY_L1_AP_P_RW_U_NA;
            break;
        case ARCH_MMU_FLAG_PERM_RO:
            /* this mapping is a lie, we don't support RO kernel mapping */
            arch_flags |= MMU_MEMORY_L1_AP_P_RW_U_NA;
            break;
        case ARCH_MMU_FLAG_PERM_USER:
            arch_flags |= MMU_MEMORY_L1_AP_P_RW_U_RW;
            break;
        case ARCH_MMU_FLAG_PERM_USER | ARCH_MMU_FLAG_PERM_RO:
            arch_flags |= MMU_MEMORY_L1_AP_P_RW_U_RO;
            break;
    }

    if (flags & ARCH_MMU_FLAG_NS) {
            arch_flags |= MMU_MEMORY_L1_SECTION_NON_SECURE;
    }

    return arch_flags;
}

/* convert user level mmu flags to flags that go in L2 descriptors */
static uint32_t mmu_flags_to_l2_arch_flags(uint flags)
{
    uint32_t arch_flags = 0;
    switch (flags & ARCH_MMU_FLAG_CACHE_MASK) {
        case ARCH_MMU_FLAG_CACHED:
            arch_flags |= MMU_MEMORY_L2_TYPE_NORMAL_WRITE_BACK_ALLOCATE;
            break;
        case ARCH_MMU_FLAG_UNCACHED:
            arch_flags |= MMU_MEMORY_L2_TYPE_STRONGLY_ORDERED;
            break;
        case ARCH_MMU_FLAG_UNCACHED_DEVICE:
            arch_flags |= MMU_MEMORY_L2_TYPE_DEVICE_SHARED;
            break;
        default:
            /* invalid user-supplied flag */
            DEBUG_ASSERT(1);
            return ERR_INVALID_ARGS;
    }

    switch (flags & (ARCH_MMU_FLAG_PERM_USER | ARCH_MMU_FLAG_PERM_RO)) {
        case 0:
            arch_flags |= MMU_MEMORY_L2_AP_P_RW_U_NA;
            break;
        case ARCH_MMU_FLAG_PERM_RO:
            /* this mapping is a lie, we don't support RO kernel mapping */
            arch_flags |= MMU_MEMORY_L2_AP_P_RW_U_NA;
            break;
        case ARCH_MMU_FLAG_PERM_USER:
            arch_flags |= MMU_MEMORY_L2_AP_P_RW_U_RW;
            break;
        case ARCH_MMU_FLAG_PERM_USER | ARCH_MMU_FLAG_PERM_RO:
            arch_flags |= MMU_MEMORY_L2_AP_P_RW_U_RO;
            break;
    }

    return arch_flags;
}

static void arm_mmu_map_section(addr_t paddr, addr_t vaddr, uint flags)
{
    int index;

    LTRACEF("pa 0x%lx va 0x%lx flags 0x%x\n", paddr, vaddr, flags);

    DEBUG_ASSERT(IS_SECTION_ALIGNED(paddr));
    DEBUG_ASSERT(IS_SECTION_ALIGNED(vaddr));
    DEBUG_ASSERT((flags & MMU_MEMORY_L1_DESCRIPTOR_MASK) == MMU_MEMORY_L1_DESCRIPTOR_SECTION);

    /* Get the index into the translation table */
    index = vaddr / SECTION_SIZE;

    /* Set the entry value:
     * (2<<0): Section entry
     * (0<<5): Domain = 0
     *  flags: TEX, CB and AP bit settings provided by the caller.
     */
    arm_kernel_translation_table[index] = (paddr & ~(MB-1)) | (MMU_MEMORY_DOMAIN_MEM << 5) | MMU_MEMORY_L1_DESCRIPTOR_SECTION | flags;
}

static void arm_mmu_unmap_section(addr_t vaddr)
{
    DEBUG_ASSERT(IS_SECTION_ALIGNED(vaddr));

    uint index = vaddr / SECTION_SIZE;
    arm_kernel_translation_table[index] = 0;

    arm_invalidate_tlb_mva(vaddr);
}

void arm_mmu_init(void)
{
    /* unmap the initial mapings that are marked temporary */
    struct mmu_initial_mapping *map = mmu_initial_mappings;
    while (map->size > 0) {
        if (map->flags & MMU_INITIAL_MAPPING_TEMPORARY) {
            vaddr_t va = map->virt;
            size_t size = map->size;

            DEBUG_ASSERT(IS_SECTION_ALIGNED(size));

            while (size > 0) {
                arm_mmu_unmap_section(va);
                va += MB;
                size -= MB;
            }
        }
        map++;
    }
}

void arch_disable_mmu(void)
{
    arm_write_sctlr(arm_read_sctlr() & ~(1<<0)); // mmu disabled
}

status_t arch_mmu_query(vaddr_t vaddr, paddr_t *paddr, uint *flags)
{
    //LTRACEF("vaddr 0x%lx\n", vaddr);

    /* Get the index into the translation table */
    uint index = vaddr / MB;

    /* decode it */
    uint32_t tt_entry = arm_kernel_translation_table[index];
    switch (tt_entry & MMU_MEMORY_L1_DESCRIPTOR_MASK) {
        case MMU_MEMORY_L1_DESCRIPTOR_INVALID:
            return ERR_NOT_FOUND;
        case MMU_MEMORY_L1_DESCRIPTOR_SECTION:
            if (tt_entry & (1<<18)) {
                /* supersection */
                PANIC_UNIMPLEMENTED;
            }

            /* section */
            if (paddr)
                *paddr = MMU_MEMORY_L1_SECTION_ADDR(tt_entry) + (vaddr & (SECTION_SIZE - 1));

            if (flags) {
                *flags = 0;
                switch (tt_entry & MMU_MEMORY_L1_TYPE_MASK) {
                    case MMU_MEMORY_L1_TYPE_STRONGLY_ORDERED:
                        *flags |= ARCH_MMU_FLAG_UNCACHED;
                        break;
                    case MMU_MEMORY_L1_TYPE_DEVICE_SHARED:
                    case MMU_MEMORY_L1_TYPE_DEVICE_NON_SHARED:
                        *flags |= ARCH_MMU_FLAG_UNCACHED_DEVICE;
                        break;
                }
                switch (tt_entry & MMU_MEMORY_L1_AP_MASK) {
                    case MMU_MEMORY_L1_AP_P_NA_U_NA:
                        // XXX no access, what to return?
                        break;
                    case MMU_MEMORY_L1_AP_P_RW_U_NA:
                        break;
                    case MMU_MEMORY_L1_AP_P_RW_U_RO:
                        *flags |= ARCH_MMU_FLAG_PERM_USER | ARCH_MMU_FLAG_PERM_RO; // XXX should it be rw anyway since kernel can rw it?
                        break;
                    case MMU_MEMORY_L1_AP_P_RW_U_RW:
                        *flags |= ARCH_MMU_FLAG_PERM_USER;
                        break;
                }
            }
            break;
        case MMU_MEMORY_L1_DESCRIPTOR_PAGE_TABLE: {
            uint32_t *l2_table = paddr_to_kvaddr(MMU_MEMORY_L1_PAGE_TABLE_ADDR(tt_entry));
            uint l2_index = (vaddr % SECTION_SIZE) / PAGE_SIZE;
            uint32_t l2_entry = l2_table[l2_index];

            //LTRACEF("l2_table at %p, index %u, entry 0x%x\n", l2_table, l2_index, l2_entry);

            switch (l2_entry & MMU_MEMORY_L2_DESCRIPTOR_MASK) {
                default:
                case MMU_MEMORY_L2_DESCRIPTOR_INVALID:
                    return ERR_NOT_FOUND;
                case MMU_MEMORY_L2_DESCRIPTOR_LARGE_PAGE:
                    PANIC_UNIMPLEMENTED;
                    break;
                case MMU_MEMORY_L2_DESCRIPTOR_SMALL_PAGE:
                case MMU_MEMORY_L2_DESCRIPTOR_SMALL_PAGE_XN:
                    if (paddr)
                        *paddr = MMU_MEMORY_L2_SMALL_PAGE_ADDR(l2_entry);

                    if (flags) {
                        *flags = 0;
                        switch (l2_entry & MMU_MEMORY_L2_TYPE_MASK) {
                            case MMU_MEMORY_L2_TYPE_STRONGLY_ORDERED:
                                *flags |= ARCH_MMU_FLAG_UNCACHED;
                                break;
                            case MMU_MEMORY_L2_TYPE_DEVICE_SHARED:
                            case MMU_MEMORY_L2_TYPE_DEVICE_NON_SHARED:
                                *flags |= ARCH_MMU_FLAG_UNCACHED_DEVICE;
                                break;
                        }
                        switch (l2_entry & MMU_MEMORY_L2_AP_MASK) {
                            case MMU_MEMORY_L2_AP_P_NA_U_NA:
                                // XXX no access, what to return?
                                break;
                            case MMU_MEMORY_L2_AP_P_RW_U_NA:
                                break;
                            case MMU_MEMORY_L2_AP_P_RW_U_RO:
                                *flags |= ARCH_MMU_FLAG_PERM_USER | ARCH_MMU_FLAG_PERM_RO; // XXX should it be rw anyway since kernel can rw it?
                                break;
                            case MMU_MEMORY_L2_AP_P_RW_U_RW:
                                *flags |= ARCH_MMU_FLAG_PERM_USER;
                                break;
                        }
                    }
                    break;
            }

            break;
        }
        default:
            PANIC_UNIMPLEMENTED;
    }

    return NO_ERROR;
}

int arch_mmu_map(vaddr_t vaddr, paddr_t paddr, uint count, uint flags)
{
    LTRACEF("vaddr 0x%lx paddr 0x%lx count %u flags 0x%x\n", vaddr, paddr, count, flags);

    /* paddr and vaddr must be aligned */
    DEBUG_ASSERT(IS_PAGE_ALIGNED(vaddr));
    DEBUG_ASSERT(IS_PAGE_ALIGNED(paddr));
    if (!IS_PAGE_ALIGNED(vaddr) || !IS_PAGE_ALIGNED(paddr))
        return ERR_INVALID_ARGS;

    if (count == 0)
        return NO_ERROR;

    /* see what kind of mapping we can use */
    int mapped = 0;
    while (count > 0) {
        if (IS_SECTION_ALIGNED(vaddr) && IS_SECTION_ALIGNED(paddr) && count >= SECTION_SIZE / PAGE_SIZE) {
            /* we can use a section */

            /* compute the arch flags for L1 sections */
            uint arch_flags = mmu_flags_to_l1_arch_flags(flags) |
                MMU_MEMORY_L1_DESCRIPTOR_SECTION;

            /* map it */
            arm_mmu_map_section(paddr, vaddr, arch_flags);
            count -= SECTION_SIZE / PAGE_SIZE;
            mapped += SECTION_SIZE / PAGE_SIZE;
            vaddr += SECTION_SIZE;
            paddr += SECTION_SIZE;
        } else {
            /* will have to use a L2 mapping */
            uint l1_index = vaddr / SECTION_SIZE;
            uint32_t tt_entry = arm_kernel_translation_table[l1_index];

            LTRACEF("tt_entry 0x%x\n", tt_entry);
            switch (tt_entry & MMU_MEMORY_L1_DESCRIPTOR_MASK) {
                case MMU_MEMORY_L1_DESCRIPTOR_SECTION:
                    // XXX will have to break L1 mapping into a L2 page table
                    PANIC_UNIMPLEMENTED;
                    break;
                case MMU_MEMORY_L1_DESCRIPTOR_INVALID: {
                    /* alloc and put in a L2 page table */
                    uint32_t *l2_table = pmm_alloc_kpage();
                    if (!l2_table) {
                        TRACEF("failed to allocate pagetable\n");
                        goto done;
                    }

                    /* get physical address */
                    paddr_t l2_pa = 0;
                    arm_vtop((vaddr_t)l2_table, &l2_pa);

                    LTRACEF("allocated pagetable at %p, pa 0x%lx\n", l2_table, l2_pa);

                    DEBUG_ASSERT(IS_PAGE_ALIGNED((vaddr_t)l2_table));
                    DEBUG_ASSERT(IS_PAGE_ALIGNED(l2_pa));

                    /* zero the L2 table and add it to the L1 table */
                    memset(l2_table, 0, PAGE_SIZE);

                    /* put it in the adjacent 4 entries filling in 1K page tables at once */
                    l1_index = ROUNDDOWN(l1_index, 4);
                    arm_kernel_translation_table[l1_index] = l2_pa | MMU_MEMORY_L1_DESCRIPTOR_PAGE_TABLE;
                    arm_kernel_translation_table[l1_index + 1] = (l2_pa + 1024) | MMU_MEMORY_L1_DESCRIPTOR_PAGE_TABLE;
                    arm_kernel_translation_table[l1_index + 2] = (l2_pa + 2048) |  MMU_MEMORY_L1_DESCRIPTOR_PAGE_TABLE;
                    arm_kernel_translation_table[l1_index + 3] = (l2_pa + 3072) |  MMU_MEMORY_L1_DESCRIPTOR_PAGE_TABLE;
                    tt_entry = arm_kernel_translation_table[l1_index];

                    /* fallthrough */
                }
                case MMU_MEMORY_L1_DESCRIPTOR_PAGE_TABLE: {
                    uint32_t *l2_table = paddr_to_kvaddr(MMU_MEMORY_L1_PAGE_TABLE_ADDR(tt_entry));
                    LTRACEF("l2_table at %p\n", l2_table);

                    DEBUG_ASSERT(l2_table);

                    // XXX handle 64K pages here

                    /* compute the arch flags for L2 4K pages */
                    uint arch_flags = mmu_flags_to_l2_arch_flags(flags) |
                        MMU_MEMORY_L2_DESCRIPTOR_SMALL_PAGE;

                    /* add the entry */
                    uint l2_index = (vaddr % SECTION_SIZE) / PAGE_SIZE;
                    l2_table[l2_index] = paddr | arch_flags;

                    count--;
                    mapped++;
                    vaddr += PAGE_SIZE;
                    paddr += PAGE_SIZE;
                    break;
                }
                default:
                    PANIC_UNIMPLEMENTED;
            }
        }
    }

done:
    return mapped;
}

int arch_mmu_unmap(vaddr_t vaddr, uint count)
{
    DEBUG_ASSERT(IS_PAGE_ALIGNED(vaddr));
    if (!IS_PAGE_ALIGNED(vaddr))
        return ERR_INVALID_ARGS;

    int unmapped = 0;
    while (count > 0) {
        uint l1_index = vaddr / SECTION_SIZE;
        uint32_t tt_entry = arm_kernel_translation_table[l1_index];

        switch (tt_entry & MMU_MEMORY_L1_DESCRIPTOR_MASK) {
            case MMU_MEMORY_L1_DESCRIPTOR_INVALID:
                /* this top level page is not mapped, move on to the next one */
                goto next_page;
            case MMU_MEMORY_L1_DESCRIPTOR_SECTION:
                if (IS_SECTION_ALIGNED(vaddr) && count >= SECTION_SIZE / PAGE_SIZE) {
                    /* we're asked to remove at least all of this section, so just zero it out */
                    // XXX test for supersection
                    arm_mmu_unmap_section(vaddr);

                    vaddr += SECTION_SIZE;
                    count -= SECTION_SIZE / PAGE_SIZE;
                    unmapped += SECTION_SIZE / PAGE_SIZE;
                    goto next;
                } else {
                    // XXX handle unmapping just part of a section
                    // will need to convert to a L2 table and then unmap the parts we are asked to
                    PANIC_UNIMPLEMENTED;
                }
                break;
            default:
                // XXX not implemented supersections or L2 tables
                PANIC_UNIMPLEMENTED;
        }

next_page:
        vaddr += PAGE_SIZE;
        count--;
next:
        ;
    }

    return unmapped;
}


#endif // ARM_WITH_MMU

/* vim: set ts=4 sw=4 expandtab: */
