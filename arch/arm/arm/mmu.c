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
#if WITH_SMP
            arch_flags |= MMU_MEMORY_L1_SECTION_SHAREABLE;
#endif
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
            arch_flags |= MMU_MEMORY_L1_AP_P_RO_U_NA;
            break;
        case ARCH_MMU_FLAG_PERM_USER:
            arch_flags |= MMU_MEMORY_L1_AP_P_RW_U_RW;
            break;
        case ARCH_MMU_FLAG_PERM_USER | ARCH_MMU_FLAG_PERM_RO:
            arch_flags |= MMU_MEMORY_L1_AP_P_RO_U_RO;
            break;
    }

    if (flags & ARCH_MMU_FLAG_PERM_NO_EXECUTE) {
         arch_flags |= MMU_MEMORY_L1_SECTION_XN;
    }

    if (flags & ARCH_MMU_FLAG_NS) {
            arch_flags |= MMU_MEMORY_L1_SECTION_NON_SECURE;
    }

    return arch_flags;
}

/* convert user level mmu flags to flags that go in L2 descriptors */
static uint32_t mmu_flags_to_l2_arch_flags_small_page(uint flags)
{
    uint32_t arch_flags = 0;
    switch (flags & ARCH_MMU_FLAG_CACHE_MASK) {
        case ARCH_MMU_FLAG_CACHED:
#if WITH_SMP
            arch_flags |= MMU_MEMORY_L2_SHAREABLE;
#endif
            arch_flags |= MMU_MEMORY_L2_TYPE_NORMAL_WRITE_BACK_ALLOCATE;
#if WITH_SMP
            arch_flags |= MMU_MEMORY_L2_SHAREABLE;
#endif
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
            arch_flags |= MMU_MEMORY_L2_AP_P_RO_U_NA;
            break;
        case ARCH_MMU_FLAG_PERM_USER:
            arch_flags |= MMU_MEMORY_L2_AP_P_RW_U_RW;
            break;
        case ARCH_MMU_FLAG_PERM_USER | ARCH_MMU_FLAG_PERM_RO:
            arch_flags |= MMU_MEMORY_L2_AP_P_RO_U_RO;
            break;
    }

    if (flags & ARCH_MMU_FLAG_PERM_NO_EXECUTE) {
         arch_flags |= MMU_MEMORY_L2_DESCRIPTOR_SMALL_PAGE_XN;
    } else {
         arch_flags |= MMU_MEMORY_L2_DESCRIPTOR_SMALL_PAGE;
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

static void arm_mmu_unmap_l1_entry(uint32_t index)
{
    DEBUG_ASSERT(index < countof(arm_kernel_translation_table));

    arm_kernel_translation_table[index] = 0;
    DSB;
    arm_invalidate_tlb_mva_no_barrier((vaddr_t)index * SECTION_SIZE);
}

static void arm_mmu_unmap_section(addr_t vaddr)
{
    DEBUG_ASSERT(IS_SECTION_ALIGNED(vaddr));
    arm_mmu_unmap_l1_entry(vaddr / SECTION_SIZE);
}

void arm_mmu_early_init(void)
{
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
    arm_after_invalidate_tlb_barrier();
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
                if (tt_entry & MMU_MEMORY_L1_SECTION_NON_SECURE)
                        *flags |= ARCH_MMU_FLAG_NS;
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
                    case MMU_MEMORY_L1_AP_P_RO_U_NA:
                        *flags |= ARCH_MMU_FLAG_PERM_RO;
                        break;
                    case MMU_MEMORY_L1_AP_P_RW_U_NA:
                        break;
                    case MMU_MEMORY_L1_AP_P_RO_U_RO:
                        *flags |= ARCH_MMU_FLAG_PERM_USER | ARCH_MMU_FLAG_PERM_RO;
                        break;
                    case MMU_MEMORY_L1_AP_P_RW_U_RW:
                        *flags |= ARCH_MMU_FLAG_PERM_USER;
                        break;
                }
                if (tt_entry & MMU_MEMORY_L1_SECTION_XN) {
                    *flags |= ARCH_MMU_FLAG_PERM_NO_EXECUTE;
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
                        /* NS flag is only present on L1 entry */
                        if (tt_entry & MMU_MEMORY_L1_PAGETABLE_NON_SECURE)
                                *flags |= ARCH_MMU_FLAG_NS;
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
                            case MMU_MEMORY_L2_AP_P_RO_U_NA:
                                *flags |= ARCH_MMU_FLAG_PERM_RO;
                                break;
                            case MMU_MEMORY_L2_AP_P_RW_U_NA:
                                break;
                            case MMU_MEMORY_L2_AP_P_RO_U_RO:
                                *flags |= ARCH_MMU_FLAG_PERM_USER | ARCH_MMU_FLAG_PERM_RO;
                                break;
                            case MMU_MEMORY_L2_AP_P_RW_U_RW:
                                *flags |= ARCH_MMU_FLAG_PERM_USER;
                                break;
                        }
                        if ((l2_entry & MMU_MEMORY_L2_DESCRIPTOR_MASK) ==
                            MMU_MEMORY_L2_DESCRIPTOR_SMALL_PAGE_XN) {
                            *flags |= ARCH_MMU_FLAG_PERM_NO_EXECUTE;
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


/*
 *  We allow up to 4 adjacent L1 entries to point within the same memory page
 *  allocated for L2 page tables.
 *
 *  L1:   | 0 | 1 | 2 | 3 | .... | N+0 | N+1 | N+2 | N+3 |
 *  L2:   [       0       | .....[      (N/4)            |
 */
#define L1E_PER_PAGE 4

static status_t get_l2_table(uint32_t l1_index, paddr_t *ppa)
{
    status_t ret;
    paddr_t pa;
    uint32_t tt_entry;

    DEBUG_ASSERT(ppa);

    /* lookup an existing l2 pagetable */
    for(uint i = 0; i < L1E_PER_PAGE; i++) {
        tt_entry = arm_kernel_translation_table[ROUNDDOWN(l1_index, L1E_PER_PAGE) + i];
        if ((tt_entry & MMU_MEMORY_L1_DESCRIPTOR_MASK)
                     == MMU_MEMORY_L1_DESCRIPTOR_PAGE_TABLE) {
            *ppa = (paddr_t)ROUNDDOWN(MMU_MEMORY_L1_PAGE_TABLE_ADDR(tt_entry), PAGE_SIZE)
                            + (PAGE_SIZE / L1E_PER_PAGE) * (l1_index & (L1E_PER_PAGE-1));
            return NO_ERROR;
        }
    }

    /* not found: allocate it */
    uint32_t *l2_va = pmm_alloc_kpage();
    if (!l2_va)
        return ERR_NO_MEMORY;

    /* wipe it clean to set no access */
    memset(l2_va, 0, PAGE_SIZE);

    /* get physical address */
    ret = arm_vtop((vaddr_t)l2_va, &pa);
    ASSERT(!ret);
    ASSERT(paddr_to_kvaddr(pa));

    DEBUG_ASSERT(IS_PAGE_ALIGNED((vaddr_t)l2_va));
    DEBUG_ASSERT(IS_PAGE_ALIGNED(pa));

    *ppa = pa + (PAGE_SIZE / L1E_PER_PAGE) * (l1_index & (L1E_PER_PAGE-1));

    LTRACEF("allocated pagetable at %p, pa 0x%lx, pa 0x%lx\n", l2_va, pa, *ppa);
    return NO_ERROR;
}


vm_page_t *address_to_page(paddr_t addr); // move to common

static void put_l2_table(uint32_t l1_index, paddr_t l2_pa)
{
    /* check if any l1 entry points to this l2 table */
    for (uint i = 0; i < L1E_PER_PAGE; i++) {
        uint32_t tt_entry = arm_kernel_translation_table[ROUNDDOWN(l1_index, L1E_PER_PAGE) + i];
        if ((tt_entry &  MMU_MEMORY_L1_DESCRIPTOR_MASK)
                == MMU_MEMORY_L1_DESCRIPTOR_PAGE_TABLE) {
            return;
        }
    }

    /* we can free this l2 table */
    vm_page_t *page = address_to_page(l2_pa);
    if (!page)
         panic("bad page table paddr 0x%lx\n", l2_pa);

    LTRACEF("freeing pagetable at 0x%lx\n", l2_pa);
    pmm_free_page(page);
}

#if WITH_ARCH_MMU_PICK_SPOT

static inline bool are_regions_compatible(uint new_region_flags,
                                          uint adjacent_region_flags)
{
    /*
     * Two regions are compatible if NS flag matches.
     */
    uint mask = ARCH_MMU_FLAG_NS;

    if ((new_region_flags & mask) == (adjacent_region_flags & mask))
        return true;

    return false;
}


vaddr_t arch_mmu_pick_spot(vaddr_t base, uint prev_region_flags,
                           vaddr_t end,  uint next_region_flags,
                           vaddr_t align, size_t size, uint flags)
{
    LTRACEF("base = 0x%lx, end=0x%lx, align=%ld, size=%zd, flags=0x%x\n",
             base, end, align, size, flags);

    vaddr_t spot;

    if (align >= SECTION_SIZE ||
        are_regions_compatible(flags, prev_region_flags)) {
        spot = ALIGN(base, align);
    } else {
        spot = ALIGN(base, SECTION_SIZE);
    }

    vaddr_t spot_end = spot + size - 1;
    if (spot_end < spot || spot_end > end)
        return end; /* wrapped around or it does not fit */

    if ((spot_end / SECTION_SIZE) == (end / SECTION_SIZE)) {
       if (!are_regions_compatible(flags, next_region_flags))
            return end;
    }

    return spot;
}
#endif  /* WITH_ARCH_MMU_PICK_SPOT */


int arch_mmu_map(vaddr_t vaddr, paddr_t paddr, uint count, uint flags)
{
    LTRACEF("vaddr 0x%lx paddr 0x%lx count %u flags 0x%x\n", vaddr, paddr, count, flags);

#if !WITH_ARCH_MMU_PICK_SPOT
    if (flags & ARCH_MMU_FLAG_NS) {
        /* WITH_ARCH_MMU_PICK_SPOT is required to support NS memory */
        panic("NS mem is not supported\n");
    }
#endif

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
                    paddr_t l2_pa = 0;
                    if (get_l2_table(l1_index, &l2_pa) != NO_ERROR) {
                        TRACEF("failed to allocate pagetable\n");
                        goto done;
                    }
                    tt_entry = l2_pa | MMU_MEMORY_L1_DESCRIPTOR_PAGE_TABLE;
                    if (flags & ARCH_MMU_FLAG_NS)
                        tt_entry |= MMU_MEMORY_L1_PAGETABLE_NON_SECURE;

                    arm_kernel_translation_table[l1_index] = tt_entry;
                    /* fallthrough */
                }
                case MMU_MEMORY_L1_DESCRIPTOR_PAGE_TABLE: {
                    uint32_t *l2_table = paddr_to_kvaddr(MMU_MEMORY_L1_PAGE_TABLE_ADDR(tt_entry));
                    LTRACEF("l2_table at %p\n", l2_table);

                    DEBUG_ASSERT(l2_table);

                    // XXX handle 64K pages here

                    /* compute the arch flags for L2 4K pages */
                    uint arch_flags = mmu_flags_to_l2_arch_flags_small_page(flags);

                    uint l2_index = (vaddr % SECTION_SIZE) / PAGE_SIZE;
                    do {
                        l2_table[l2_index++] = paddr | arch_flags;
                        count--;
                        mapped++;
                        vaddr += PAGE_SIZE;
                        paddr += PAGE_SIZE;
                    } while (count && (l2_index != (SECTION_SIZE / PAGE_SIZE)));
                    break;
                }
                default:
                    PANIC_UNIMPLEMENTED;
            }
        }
    }

done:
    DSB;
    return mapped;
}

int arch_mmu_unmap(vaddr_t vaddr, uint count)
{
    DEBUG_ASSERT(IS_PAGE_ALIGNED(vaddr));
    if (!IS_PAGE_ALIGNED(vaddr))
        return ERR_INVALID_ARGS;

    LTRACEF("vaddr 0x%lx count %u\n", vaddr, count);

    int unmapped = 0;
    while (count > 0) {
        uint l1_index = vaddr / SECTION_SIZE;
        uint32_t tt_entry = arm_kernel_translation_table[l1_index];

        switch (tt_entry & MMU_MEMORY_L1_DESCRIPTOR_MASK) {
            case MMU_MEMORY_L1_DESCRIPTOR_INVALID: {
                /* this top level page is not mapped, move on to the next one */
                uint page_cnt = MIN((SECTION_SIZE - (vaddr % SECTION_SIZE)) / PAGE_SIZE, count);
                vaddr += page_cnt * PAGE_SIZE;
                count -= page_cnt;
                break;
            }
            case MMU_MEMORY_L1_DESCRIPTOR_SECTION:
                if (IS_SECTION_ALIGNED(vaddr) && count >= SECTION_SIZE / PAGE_SIZE) {
                    /* we're asked to remove at least all of this section, so just zero it out */
                    // XXX test for supersection
                    arm_mmu_unmap_section(vaddr);

                    vaddr += SECTION_SIZE;
                    count -= SECTION_SIZE / PAGE_SIZE;
                    unmapped += SECTION_SIZE / PAGE_SIZE;
                } else {
                    // XXX handle unmapping just part of a section
                    // will need to convert to a L2 table and then unmap the parts we are asked to
                    PANIC_UNIMPLEMENTED;
                }
                break;
            case MMU_MEMORY_L1_DESCRIPTOR_PAGE_TABLE: {
                uint32_t *l2_table = paddr_to_kvaddr(MMU_MEMORY_L1_PAGE_TABLE_ADDR(tt_entry));
                uint page_idx = (vaddr % SECTION_SIZE) / PAGE_SIZE;
                uint page_cnt = MIN((SECTION_SIZE / PAGE_SIZE) - page_idx, count);

                /* unmap page run */
                for (uint i = 0; i < page_cnt; i++) {
                    l2_table[page_idx++] = 0;
                }
                DSB;

                /* invalidate tlb */
                for (uint i = 0; i < page_cnt; i++) {
                    arm_invalidate_tlb_mva_no_barrier(vaddr);
                    vaddr += PAGE_SIZE;
                }
                count -= page_cnt;
                unmapped += page_cnt;

                /*
                 * Check if all pages related to this l1 entry are deallocated.
                 * We only need to check pages that we did not clear above starting
                 * from page_idx and wrapped around SECTION.
                 */
                page_cnt = (SECTION_SIZE / PAGE_SIZE) - page_cnt;
                while (page_cnt) {
                    if (page_idx == (SECTION_SIZE / PAGE_SIZE))
                        page_idx = 0;
                    if (l2_table[page_idx++])
                        break;
                    page_cnt--;
                }
                if (!page_cnt) {
                    /* we can kill l1 entry */
                    arm_mmu_unmap_l1_entry(l1_index);

                    /* try to free l2 page itself */
                    put_l2_table(l1_index, MMU_MEMORY_L1_PAGE_TABLE_ADDR(tt_entry));
                }
                break;
            }

            default:
                // XXX not implemented supersections or L2 tables
                PANIC_UNIMPLEMENTED;
        }
    }
    arm_after_invalidate_tlb_barrier();
    return unmapped;
}


#endif // ARM_WITH_MMU

/* vim: set ts=4 sw=4 expandtab: */
