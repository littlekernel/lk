/*
 * Copyright (c) 2008-2016 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#if ARCH_HAS_MMU

#include <lk/debug.h>
#include <lk/trace.h>
#include <stdlib.h>
#include <sys/types.h>
#include <lk/err.h>
#include <string.h>
#include <lk/compiler.h>
#include <lk/pow2.h>
#include <arch.h>
#include <arch/ops.h>
#include <arch/mmu.h>
#include <arch/arm.h>
#include <arch/arm/mmu.h>
#include <kernel/vm.h>

#define LOCAL_TRACE 0
#define TRACE_CONTEXT_SWITCH 0

#define IS_SECTION_ALIGNED(x) IS_ALIGNED(x, SECTION_SIZE)
#define IS_SUPERSECTION_ALIGNED(x) IS_ALIGNED(x, SUPERSECTION_SIZE)

/* TTBR0 translates [0, KERNEL_ASPACE_BASE) and TTBR1 the kernel above it */
STATIC_ASSERT((KERNEL_ASPACE_BASE & (KERNEL_ASPACE_BASE - 1)) == 0 &&
              KERNEL_ASPACE_BASE >= 32 * MB && KERNEL_ASPACE_BASE <= 2 * GB);
#define TTBCR_N     (__builtin_clz(KERNEL_ASPACE_BASE) + 1)
#define TTBCR_PD0   (1u << 4)   /* no TTBR0 walks */

/* locals */
static void arm_mmu_map_section(arch_aspace_t *aspace, addr_t paddr, addr_t vaddr, uint flags);

/* the main translation table */
uint32_t arm_kernel_translation_table[TT_ENTRY_COUNT] __ALIGNED(16384) __SECTION(".bss.prebss.translation_table");

/* convert user level mmu flags to flags that go in L1 descriptors */
static uint32_t mmu_flags_to_l1_arch_flags(uint flags) {
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
            /* arch_mmu_flags_valid() refuses the fourth policy */
            DEBUG_ASSERT_MSG(false, "flags 0x%x\n", flags);
            break;
    }

    switch (flags & (ARCH_MMU_FLAG_PERM_USER | ARCH_MMU_FLAG_PERM_WRITE)) {
        case 0:
            arch_flags |= MMU_MEMORY_L1_AP_P_RO_U_NA;
            break;
        case ARCH_MMU_FLAG_PERM_WRITE:
            arch_flags |= MMU_MEMORY_L1_AP_P_RW_U_NA;
            break;
        case ARCH_MMU_FLAG_PERM_USER:
            arch_flags |= MMU_MEMORY_L1_AP_P_RO_U_RO;
            break;
        case ARCH_MMU_FLAG_PERM_USER | ARCH_MMU_FLAG_PERM_WRITE:
            arch_flags |= MMU_MEMORY_L1_AP_P_RW_U_RW;
            break;
    }

    if (!(flags & ARCH_MMU_FLAG_PERM_EXECUTE)) {
        arch_flags |= MMU_MEMORY_L1_SECTION_XN;
    }

    if (flags & ARCH_MMU_FLAG_NS) {
        arch_flags |= MMU_MEMORY_L1_SECTION_NON_SECURE;
    }

    return arch_flags;
}

/* convert user level mmu flags to flags that go in L2 descriptors */
static uint32_t mmu_flags_to_l2_arch_flags_small_page(uint flags) {
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
            /* arch_mmu_flags_valid() refuses the fourth policy */
            DEBUG_ASSERT_MSG(false, "flags 0x%x\n", flags);
            break;
    }

    switch (flags & (ARCH_MMU_FLAG_PERM_USER | ARCH_MMU_FLAG_PERM_WRITE)) {
        case 0:
            arch_flags |= MMU_MEMORY_L2_AP_P_RO_U_NA;
            break;
        case ARCH_MMU_FLAG_PERM_WRITE:
            arch_flags |= MMU_MEMORY_L2_AP_P_RW_U_NA;
            break;
        case ARCH_MMU_FLAG_PERM_USER:
            arch_flags |= MMU_MEMORY_L2_AP_P_RO_U_RO;
            break;
        case ARCH_MMU_FLAG_PERM_USER | ARCH_MMU_FLAG_PERM_WRITE:
            arch_flags |= MMU_MEMORY_L2_AP_P_RW_U_RW;
            break;
    }

    if (flags & ARCH_MMU_FLAG_PERM_EXECUTE) {
        arch_flags |= MMU_MEMORY_L2_DESCRIPTOR_SMALL_PAGE;
    } else {
        arch_flags |= MMU_MEMORY_L2_DESCRIPTOR_SMALL_PAGE_XN;
    }

    return arch_flags;
}

static void arm_mmu_map_section(arch_aspace_t *aspace, addr_t paddr, addr_t vaddr, uint flags) {
    int index;

    LTRACEF("aspace %p tt %p pa 0x%lx va 0x%lx flags 0x%x\n", aspace, aspace->tt_virt, paddr, vaddr, flags);

    DEBUG_ASSERT(aspace);
    DEBUG_ASSERT(aspace->tt_virt);
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
    aspace->tt_virt[index] = (paddr & ~(MB-1)) | (MMU_MEMORY_DOMAIN_MEM << 5) | MMU_MEMORY_L1_DESCRIPTOR_SECTION | flags;
}

static void arm_mmu_unmap_l1_entry(uint32_t *translation_table, uint32_t index) {
    DEBUG_ASSERT(translation_table);
    DEBUG_ASSERT(index < TT_ENTRY_COUNT);

    translation_table[index] = 0;
    DSB;
    arm_invalidate_tlb_mva_no_barrier((vaddr_t)index * SECTION_SIZE);
}

/*
 * Chain load only: map [base, base + count sections) onto itself read, write
 * and execute, replacing whatever the entries held. Nothing is allocated or
 * freed, so it runs with interrupts off; a page table it unlinks is left behind.
 */
void arm_mmu_chain_load_map(arch_aspace_t *aspace, vaddr_t base, uint count) {
    DEBUG_ASSERT(IS_SECTION_ALIGNED(base));
    DEBUG_ASSERT(arch_mmu_range_in_aspace(aspace, base, count * (SECTION_SIZE / PAGE_SIZE)));

    const uint32_t flags = mmu_flags_to_l1_arch_flags(ARCH_MMU_FLAG_PERM_READ |
                                                      ARCH_MMU_FLAG_PERM_WRITE |
                                                      ARCH_MMU_FLAG_PERM_EXECUTE);
    for (uint i = 0; i < count; i++) {
        const vaddr_t va = base + i * SECTION_SIZE;
        arm_mmu_map_section(aspace, va, va, flags | MMU_MEMORY_L1_DESCRIPTOR_SECTION);
    }

    /* a replaced page table's small pages are not covered by any one address */
    arm_invalidate_tlb_global();
}

void arm_mmu_early_init(void) {
}

void arm_mmu_init(void) {
    /* unmap the initial mappings that are marked temporary */
    struct mmu_initial_mapping *map = mmu_initial_mappings;
    while (map->size > 0) {
        if (map->flags & MMU_INITIAL_MAPPING_TEMPORARY) {
            vaddr_t va = map->virt;
            size_t size = map->size;

            DEBUG_ASSERT(IS_SECTION_ALIGNED(size));

            while (size > 0) {
                arm_mmu_unmap_l1_entry(arm_kernel_translation_table, va / SECTION_SIZE);
                va += MB;
                size -= MB;
            }
        }
        map++;
    }
    arm_after_invalidate_tlb_barrier();

    arm_mmu_init_percpu();
}

/*
 * Runs on every cpu while TTBR0 still holds the kernel table from start.S: moves
 * that table to TTBR1 and leaves TTBR0 walks off until a user aspace is loaded.
 */
void arm_mmu_init_percpu(void) {
    /* every aspace runs with ASID 0; the reset value is UNKNOWN */
    arm_write_contextidr(0);
    ISB;

    arm_write_ttbr1(arm_read_ttbr0());
    ISB;
    arm_write_ttbcr(TTBCR_PD0 | TTBCR_N);
    ISB;
    arm_write_ttbr0(0);
    ISB;

    /* drop anything this cpu cached through TTBR0 */
    arm_write_tlbiall(0);
    arm_write_bpiall(0);
    DSB;
    ISB;
}

/*
 * Every aspace runs with ASID 0 and marks its entries non-global, so a switch
 * drops what the old one left in this cpu's TLB; the kernel's global entries
 * stay. TTBCR changes only to turn TTBR0 walks on or off.
 */
void arch_mmu_context_switch(arch_aspace_t *old_aspace, arch_aspace_t *aspace) {
    if (LOCAL_TRACE && TRACE_CONTEXT_SWITCH)
        LTRACEF("old %p, aspace %p\n", old_aspace, aspace);

    if (old_aspace == aspace)
        return;

    if (aspace) {
        arm_write_ttbr0(MMU_TTBRx_FLAGS | aspace->tt_phys);
        ISB;
        if (!old_aspace) {
            arm_write_ttbcr(TTBCR_N);
            ISB;
        }
    } else {
        /* walks off before TTBR0 stops pointing at a table */
        arm_write_ttbcr(TTBCR_PD0 | TTBCR_N);
        ISB;
        arm_write_ttbr0(0);
        ISB;
    }

    arm_write_tlbiasid(0);
    arm_write_bpiall(0);
    DSB;
    ISB;
}

status_t arch_mmu_query(arch_aspace_t *aspace, vaddr_t vaddr, paddr_t *paddr, uint *flags) {
    LTRACEF("aspace %p, vaddr 0x%lx\n", aspace, vaddr);

    DEBUG_ASSERT(aspace);
    DEBUG_ASSERT(aspace->tt_virt);

    if (!arch_mmu_range_in_aspace(aspace, vaddr, 1))
        return ERR_OUT_OF_RANGE;

    /* Get the index into the translation table */
    uint index = vaddr / MB;

    /* decode it */
    uint32_t tt_entry = aspace->tt_virt[index];
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
                *flags = ARCH_MMU_FLAG_PERM_READ;
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
                        break;
                    case MMU_MEMORY_L1_AP_P_RW_U_NA:
                        *flags |= ARCH_MMU_FLAG_PERM_WRITE;
                        break;
                    case MMU_MEMORY_L1_AP_P_RO_U_RO:
                        *flags |= ARCH_MMU_FLAG_PERM_USER;
                        break;
                    case MMU_MEMORY_L1_AP_P_RW_U_RW:
                        *flags |= ARCH_MMU_FLAG_PERM_USER | ARCH_MMU_FLAG_PERM_WRITE;
                        break;
                }
                if (!(tt_entry & MMU_MEMORY_L1_SECTION_XN)) {
                    *flags |= ARCH_MMU_FLAG_PERM_EXECUTE;
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
                        *paddr = MMU_MEMORY_L2_SMALL_PAGE_ADDR(l2_entry) + (vaddr & (PAGE_SIZE - 1));

                    if (flags) {
                        *flags = ARCH_MMU_FLAG_PERM_READ;
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
                                break;
                            case MMU_MEMORY_L2_AP_P_RW_U_NA:
                                *flags |= ARCH_MMU_FLAG_PERM_WRITE;
                                break;
                            case MMU_MEMORY_L2_AP_P_RO_U_RO:
                                *flags |= ARCH_MMU_FLAG_PERM_USER;
                                break;
                            case MMU_MEMORY_L2_AP_P_RW_U_RW:
                                *flags |= ARCH_MMU_FLAG_PERM_USER | ARCH_MMU_FLAG_PERM_WRITE;
                                break;
                        }
                        if ((l2_entry & MMU_MEMORY_L2_DESCRIPTOR_MASK) ==
                                MMU_MEMORY_L2_DESCRIPTOR_SMALL_PAGE) {
                            *flags |= ARCH_MMU_FLAG_PERM_EXECUTE;
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
#define L2E_PER_TABLE (SECTION_SIZE / PAGE_SIZE)

/* a section or supersection; bit 0 of either is PXN where that is implemented */
static inline bool l1_is_section(uint32_t tt_entry) {
    return tt_entry & MMU_MEMORY_L1_DESCRIPTOR_SECTION;
}

static inline bool l1_is_supersection(uint32_t tt_entry) {
    return l1_is_section(tt_entry) && (tt_entry & (1 << 18));
}

static inline uint32_t *l1_l2_table(uint32_t tt_entry) {
    return paddr_to_kvaddr(MMU_MEMORY_L1_PAGE_TABLE_ADDR(tt_entry));
}

static inline bool l2_is_valid(uint32_t l2_entry) {
    return (l2_entry & MMU_MEMORY_L2_DESCRIPTOR_MASK) != MMU_MEMORY_L2_DESCRIPTOR_INVALID;
}

/* The L2 table for l1_index: in the page a neighbouring entry already uses, or in
 * a fresh zeroed one. Either way it holds no valid entry. */
static status_t get_l2_table(arch_aspace_t *aspace, uint32_t l1_index, paddr_t *ppa) {
    status_t ret;
    paddr_t pa;
    uint32_t tt_entry;

    DEBUG_ASSERT(aspace);
    DEBUG_ASSERT(ppa);

    /* lookup an existing l2 pagetable */
    for (uint i = 0; i < L1E_PER_PAGE; i++) {
        tt_entry = aspace->tt_virt[ROUNDDOWN(l1_index, L1E_PER_PAGE) + i];
        if ((tt_entry & MMU_MEMORY_L1_DESCRIPTOR_MASK)
                == MMU_MEMORY_L1_DESCRIPTOR_PAGE_TABLE) {
            *ppa = (paddr_t)ROUNDDOWN(MMU_MEMORY_L1_PAGE_TABLE_ADDR(tt_entry), PAGE_SIZE)
                   + (PAGE_SIZE / L1E_PER_PAGE) * (l1_index & (L1E_PER_PAGE-1));
            return NO_ERROR;
        }
    }

    /* not found: allocate it */
    if (!vm_ptable_budget_take())
        return ERR_NO_MEMORY;
    uint32_t *l2_va = pmm_alloc_kpages(1, &aspace->pt_page_list);
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

/* Move the page holding l1_index's L2 table from the aspace's list to freed,
 * once none of the entries sharing the page points to a table. */
static void put_l2_table(arch_aspace_t *aspace, uint32_t l1_index, paddr_t l2_pa,
                         struct list_node *freed) {
    DEBUG_ASSERT(aspace);

    /* check if any l1 entry points to this l2 table */
    for (uint i = 0; i < L1E_PER_PAGE; i++) {
        uint32_t tt_entry = aspace->tt_virt[ROUNDDOWN(l1_index, L1E_PER_PAGE) + i];
        if ((tt_entry &  MMU_MEMORY_L1_DESCRIPTOR_MASK)
                == MMU_MEMORY_L1_DESCRIPTOR_PAGE_TABLE) {
            return;
        }
    }

    /* we can free this l2 table */
    vm_page_t *page = paddr_to_vm_page(l2_pa);
    if (!page)
        panic("bad page table paddr 0x%lx\n", l2_pa);

    /* verify that it is in our page list */
    DEBUG_ASSERT(list_in_list(&page->node));

    list_delete(&page->node);

    LTRACEF("freeing pagetable at 0x%lx\n", l2_pa);
    list_add_tail(freed, &page->node);
}

/*
 * Clear [vaddr, last], last inclusive since the kernel aspace ends at 4GB,
 * stepping over holes. The caller has made sure no section is only partly
 * covered. A page table left empty is unlinked; the pages that held them are
 * freed after the TLB maintenance has completed on every cpu.
 */
static void arm_mmu_unmap_range(arch_aspace_t *aspace, vaddr_t vaddr, vaddr_t last) {
    struct list_node freed = LIST_INITIAL_VALUE(freed);

    for (;;) {
        const uint l1_index = vaddr / SECTION_SIZE;
        const uint32_t tt_entry = aspace->tt_virt[l1_index];
        const vaddr_t section_base = ROUNDDOWN(vaddr, SECTION_SIZE);
        const vaddr_t entry_last = MIN(section_base + (SECTION_SIZE - 1), last);

        switch (tt_entry & MMU_MEMORY_L1_DESCRIPTOR_MASK) {
            case MMU_MEMORY_L1_DESCRIPTOR_INVALID:
                break;
            case MMU_MEMORY_L1_DESCRIPTOR_PAGE_TABLE: {
                uint32_t *l2_table = l1_l2_table(tt_entry);
                const uint end = (entry_last - section_base) / PAGE_SIZE + 1;
                uint first_cleared = end;
                uint last_cleared = 0;
                for (uint i = (vaddr - section_base) / PAGE_SIZE; i < end; i++) {
                    if (l2_is_valid(l2_table[i])) {
                        l2_table[i] = 0;
                        first_cleared = MIN(first_cleared, i);
                        last_cleared = i;
                    }
                }
                DSB;
                for (uint i = first_cleared; i <= last_cleared; i++) {
                    arm_invalidate_tlb_mva_no_barrier(section_base + i * PAGE_SIZE);
                }

                bool empty = true;
                for (uint i = 0; i < L2E_PER_TABLE && empty; i++) {
                    empty = !l2_is_valid(l2_table[i]);
                }
                if (empty) {
                    /* the invalidate also drops a walk cached through the entry */
                    aspace->tt_virt[l1_index] = 0;
                    DSB;
                    arm_invalidate_tlb_mva_no_barrier(section_base);
                    put_l2_table(aspace, l1_index, MMU_MEMORY_L1_PAGE_TABLE_ADDR(tt_entry), &freed);
                }
                break;
            }
            default:
                /* a whole section, or one of a supersection's sixteen entries */
                aspace->tt_virt[l1_index] = 0;
                DSB;
                arm_invalidate_tlb_mva_no_barrier(section_base);
                break;
        }

        if (entry_last == last)
            break;
        vaddr = entry_last + 1;
    }

    arm_after_invalidate_tlb_barrier();
    pmm_free(&freed);
}

/* Unmap and protect change whole sections only. Only the two ends of a range
 * can cut one, and a supersection, which map never makes, is refused at either. */
static status_t arm_mmu_check_range_ends(const arch_aspace_t *aspace, vaddr_t vaddr, vaddr_t last) {
    const uint32_t first_entry = aspace->tt_virt[vaddr / SECTION_SIZE];
    const uint32_t last_entry = aspace->tt_virt[last / SECTION_SIZE];

    if (l1_is_section(first_entry) &&
            (l1_is_supersection(first_entry) || !IS_SECTION_ALIGNED(vaddr))) {
        LTRACEF("range at 0x%lx starts inside a section\n", vaddr);
        return ERR_NOT_SUPPORTED;
    }
    if (l1_is_section(last_entry) &&
            (l1_is_supersection(last_entry) || !IS_SECTION_ALIGNED(last + 1))) {
        LTRACEF("range ending at 0x%lx ends inside a section\n", last);
        return ERR_NOT_SUPPORTED;
    }
    return NO_ERROR;
}

#if WITH_ARCH_MMU_PICK_SPOT

static inline bool are_regions_compatible(uint new_region_flags,
        uint adjacent_region_flags) {
    /*
     * Two regions are compatible if NS flag matches.
     */
    uint mask = ARCH_MMU_FLAG_NS;

    if ((new_region_flags & mask) == (adjacent_region_flags & mask))
        return true;

    return false;
}


vaddr_t arch_mmu_pick_spot(arch_aspace_t *aspace,
                           vaddr_t base, uint prev_region_flags,
                           vaddr_t end,  uint next_region_flags,
                           vaddr_t align, size_t size, uint flags) {
    LTRACEF("base 0x%lx, end 0x%lx, align %ld, size %zd, flags 0x%x\n",
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


status_t arch_mmu_map(arch_aspace_t *aspace, addr_t vaddr, paddr_t paddr, uint count, uint flags) {
    LTRACEF("vaddr 0x%lx paddr 0x%lx count %u flags 0x%x\n", vaddr, paddr, count, flags);

    DEBUG_ASSERT(aspace);
    DEBUG_ASSERT(aspace->tt_virt);

    if (!IS_PAGE_ALIGNED(vaddr) || !IS_PAGE_ALIGNED(paddr) ||
            !arch_mmu_paddr_range_valid(paddr, count, 32) || !arch_mmu_flags_valid(flags))
        return ERR_INVALID_ARGS;

    if (!arch_mmu_range_in_aspace(aspace, vaddr, count))
        return ERR_OUT_OF_RANGE;

    if (count == 0)
        return NO_ERROR;

    /* a user aspace's entries are tagged with the ASID, so a switch can drop them */
    const bool user = aspace->tt_virt != arm_kernel_translation_table;
    const uint32_t section_flags = mmu_flags_to_l1_arch_flags(flags) |
                                   MMU_MEMORY_L1_DESCRIPTOR_SECTION |
                                   (user ? MMU_MEMORY_L1_SECTION_NON_GLOBAL : 0);
    const uint32_t page_flags = mmu_flags_to_l2_arch_flags_small_page(flags) |
                                (user ? MMU_MEMORY_L2_NON_GLOBAL : 0);
    const uint32_t table_ns = (flags & ARCH_MMU_FLAG_NS) ? MMU_MEMORY_L1_PAGETABLE_NON_SECURE : 0;

    vaddr_t va = vaddr;
    paddr_t pa = paddr;
    uint left = count;
    status_t err = NO_ERROR;
    while (left > 0 && err == NO_ERROR) {
        const uint l1_index = va / SECTION_SIZE;
        uint32_t tt_entry = aspace->tt_virt[l1_index];

        LTRACEF("tt_entry 0x%x\n", tt_entry);
        switch (tt_entry & MMU_MEMORY_L1_DESCRIPTOR_MASK) {
            case MMU_MEMORY_L1_DESCRIPTOR_INVALID: {
                if (IS_SECTION_ALIGNED(va) && IS_SECTION_ALIGNED(pa) && left >= L2E_PER_TABLE) {
                    arm_mmu_map_section(aspace, pa, va, section_flags);
                    va += SECTION_SIZE;
                    pa += SECTION_SIZE;
                    left -= L2E_PER_TABLE;
                    continue;
                }

                paddr_t l2_pa = 0;
                err = get_l2_table(aspace, l1_index, &l2_pa);
                if (err != NO_ERROR) {
                    LTRACEF("failed to allocate pagetable\n");
                    continue;
                }

                /* the table reads as empty before an entry points at it */
                tt_entry = l2_pa | table_ns | MMU_MEMORY_L1_DESCRIPTOR_PAGE_TABLE;
                DSB;
                aspace->tt_virt[l1_index] = tt_entry;
                break;
            }
            case MMU_MEMORY_L1_DESCRIPTOR_PAGE_TABLE:
                /* the security state is the table's, for all of its pages */
                if ((tt_entry & MMU_MEMORY_L1_PAGETABLE_NON_SECURE) != table_ns) {
                    err = ERR_ALREADY_EXISTS;
                    continue;
                }
                break;
            default:
                /* a section or supersection covers the page */
                err = ERR_ALREADY_EXISTS;
                continue;
        }

        uint32_t *l2_table = l1_l2_table(tt_entry);
        uint l2_index = (va % SECTION_SIZE) / PAGE_SIZE;
        do {
            if (l2_is_valid(l2_table[l2_index])) {
                err = ERR_ALREADY_EXISTS;
                break;
            }
            l2_table[l2_index++] = pa | page_flags;
            va += PAGE_SIZE;
            pa += PAGE_SIZE;
            left--;
        } while (left && l2_index != L2E_PER_TABLE);
    }

    if (err != NO_ERROR) {
        /* [vaddr, va) is what went in, tables linked for it included */
        if (va != vaddr) {
            arm_mmu_unmap_range(aspace, vaddr, va - 1);
        }
        return err;
    }

    /* the entries were all invalid before, and no TLB holds an invalid entry */
    DSB;
    ISB;
    return NO_ERROR;
}

status_t arch_mmu_unmap(arch_aspace_t *aspace, vaddr_t vaddr, uint count) {
    LTRACEF("vaddr 0x%lx count %u\n", vaddr, count);

    DEBUG_ASSERT(aspace);
    DEBUG_ASSERT(aspace->tt_virt);

    if (!IS_PAGE_ALIGNED(vaddr))
        return ERR_INVALID_ARGS;

    if (!arch_mmu_range_in_aspace(aspace, vaddr, count))
        return ERR_OUT_OF_RANGE;

    if (count == 0)
        return NO_ERROR;

    const vaddr_t last = vaddr + ((vaddr_t)count * PAGE_SIZE - 1);
    status_t err = arm_mmu_check_range_ends(aspace, vaddr, last);
    if (err != NO_ERROR)
        return err;

    arm_mmu_unmap_range(aspace, vaddr, last);
    return NO_ERROR;
}

static inline bool l1_is_uncached(uint32_t tt_entry) {
    switch (tt_entry & MMU_MEMORY_L1_TYPE_MASK) {
        case MMU_MEMORY_L1_TYPE_STRONGLY_ORDERED:
        case MMU_MEMORY_L1_TYPE_DEVICE_SHARED:
        case MMU_MEMORY_L1_TYPE_DEVICE_NON_SHARED:
            return true;
    }
    return false;
}

static inline bool l2_is_uncached(uint32_t l2_entry) {
    switch (l2_entry & MMU_MEMORY_L2_TYPE_MASK) {
        case MMU_MEMORY_L2_TYPE_STRONGLY_ORDERED:
        case MMU_MEMORY_L2_TYPE_DEVICE_SHARED:
        case MMU_MEMORY_L2_TYPE_DEVICE_NON_SHARED:
            return true;
    }
    return false;
}

/*
 * Give every section and small page in [vaddr, last] the read only and XN bits
 * flags asks for, invalidating the ones that change. With check_only nothing is
 * written, and the walk only reports ERR_INVALID_ARGS on an uncached page when
 * flags asks for execute.
 */
static status_t arm_mmu_protect_range(arch_aspace_t *aspace, vaddr_t vaddr, vaddr_t last, uint flags,
                                      bool check_only) {
    const bool execute = flags & ARCH_MMU_FLAG_PERM_EXECUTE;
    const bool read_only = !(flags & ARCH_MMU_FLAG_PERM_WRITE);
    const uint32_t l1_bits = (read_only ? MMU_MEMORY_L1_AP_RO : 0) |
                             (execute ? 0 : MMU_MEMORY_L1_SECTION_XN);
    const uint32_t l2_bits = (read_only ? MMU_MEMORY_L2_AP_RO : 0) |
                             (execute ? 0 : MMU_MEMORY_L2_SMALL_PAGE_XN);
    bool changed = false;

    for (;;) {
        const uint l1_index = vaddr / SECTION_SIZE;
        const uint32_t tt_entry = aspace->tt_virt[l1_index];
        const vaddr_t section_base = ROUNDDOWN(vaddr, SECTION_SIZE);
        const vaddr_t entry_last = MIN(section_base + (SECTION_SIZE - 1), last);

        switch (tt_entry & MMU_MEMORY_L1_DESCRIPTOR_MASK) {
            case MMU_MEMORY_L1_DESCRIPTOR_INVALID:
                break;
            case MMU_MEMORY_L1_DESCRIPTOR_PAGE_TABLE: {
                uint32_t *l2_table = l1_l2_table(tt_entry);
                const uint end = (entry_last - section_base) / PAGE_SIZE + 1;
                uint first_changed = end;
                uint last_changed = 0;
                for (uint i = (vaddr - section_base) / PAGE_SIZE; i < end; i++) {
                    const uint32_t l2_entry = l2_table[i];
                    if (!l2_is_valid(l2_entry)) {
                        continue;
                    }
                    if (check_only) {
                        if (execute && l2_is_uncached(l2_entry)) {
                            return ERR_INVALID_ARGS;
                        }
                        continue;
                    }
                    /* map makes small pages only, whose XN is descriptor bit 0 */
                    DEBUG_ASSERT((l2_entry & MMU_MEMORY_L2_DESCRIPTOR_MASK) !=
                                 MMU_MEMORY_L2_DESCRIPTOR_LARGE_PAGE);
                    const uint32_t new_entry =
                        (l2_entry & ~(MMU_MEMORY_L2_AP_RO | MMU_MEMORY_L2_SMALL_PAGE_XN)) | l2_bits;
                    if (new_entry != l2_entry) {
                        l2_table[i] = new_entry;
                        first_changed = MIN(first_changed, i);
                        last_changed = i;
                    }
                }
                if (first_changed != end) {
                    DSB;
                    for (uint i = first_changed; i <= last_changed; i++) {
                        arm_invalidate_tlb_mva_no_barrier(section_base + i * PAGE_SIZE);
                    }
                    changed = true;
                }
                break;
            }
            default: {
                /* a whole section, or one of a supersection's sixteen entries */
                if (check_only) {
                    if (execute && l1_is_uncached(tt_entry)) {
                        return ERR_INVALID_ARGS;
                    }
                    break;
                }
                const uint32_t new_entry =
                    (tt_entry & ~(MMU_MEMORY_L1_AP_RO | MMU_MEMORY_L1_SECTION_XN)) | l1_bits;
                if (new_entry != tt_entry) {
                    aspace->tt_virt[l1_index] = new_entry;
                    DSB;
                    arm_invalidate_tlb_mva_no_barrier(section_base);
                    changed = true;
                }
                break;
            }
        }

        if (entry_last == last)
            break;
        vaddr = entry_last + 1;
    }

    /* upgrades included: a cpu may otherwise fault on the entry it has cached */
    if (changed) {
        arm_after_invalidate_tlb_barrier();
    }
    return NO_ERROR;
}

status_t arch_mmu_protect(arch_aspace_t *aspace, vaddr_t vaddr, uint count, uint flags) {
    LTRACEF("aspace %p, vaddr 0x%lx, count %u, flags 0x%x\n", aspace, vaddr, count, flags);

    DEBUG_ASSERT(aspace);
    DEBUG_ASSERT(aspace->tt_virt);

    if (!IS_PAGE_ALIGNED(vaddr) || !arch_mmu_protect_flags_valid(flags))
        return ERR_INVALID_ARGS;

    if (!arch_mmu_range_in_aspace(aspace, vaddr, count))
        return ERR_OUT_OF_RANGE;

    if (count == 0)
        return NO_ERROR;

    const vaddr_t last = vaddr + ((vaddr_t)count * PAGE_SIZE - 1);
    status_t err = arm_mmu_check_range_ends(aspace, vaddr, last);
    if (err != NO_ERROR)
        return err;

    if (flags & ARCH_MMU_FLAG_PERM_EXECUTE) {
        err = arm_mmu_protect_range(aspace, vaddr, last, flags, true);
        if (err != NO_ERROR)
            return err;
    }

    return arm_mmu_protect_range(aspace, vaddr, last, flags, false);
}

status_t arch_mmu_init_aspace(arch_aspace_t *aspace, vaddr_t base, size_t size, uint flags) {
    LTRACEF("aspace %p, base 0x%lx, size 0x%zx, flags 0x%x\n", aspace, base, size, flags);

    DEBUG_ASSERT(aspace);

    /* validate that the base + size is sane and doesn't wrap */
    DEBUG_ASSERT(size > PAGE_SIZE);
    DEBUG_ASSERT(base + size - 1 > base);

    list_initialize(&aspace->pt_page_list);

    if (flags & ARCH_ASPACE_FLAG_KERNEL) {
        aspace->base = base;
        aspace->size = size;
        aspace->tt_virt = arm_kernel_translation_table;
        aspace->tt_phys = vaddr_to_paddr(aspace->tt_virt);
    } else {

        // XXX at the moment we can only really deal with 1GB user space, and thus
        // needing only a single page for the top level translation table
        DEBUG_ASSERT(base < GB && (base + size) <= GB);

        aspace->base = base;
        aspace->size = size;

        /* on the page list with the L2 tables, freed with them in destroy */
        uint32_t *va = pmm_alloc_kpages(1, &aspace->pt_page_list);
        if (!va)
            return ERR_NO_MEMORY;
        memset(va, 0, PAGE_SIZE);
        DSB;

        aspace->tt_virt = va;
        aspace->tt_phys = vaddr_to_paddr(aspace->tt_virt);
    }

    LTRACEF("tt_phys 0x%lx tt_virt %p\n", aspace->tt_phys, aspace->tt_virt);

    return NO_ERROR;
}

status_t arch_mmu_destroy_aspace(arch_aspace_t *aspace) {
    LTRACEF("aspace %p\n", aspace);

    DEBUG_ASSERT(aspace->tt_virt != arm_kernel_translation_table);

    /* nothing mapped means no L2 table either, which leaves the root on the list */
    for (uint i = aspace->base / SECTION_SIZE; i <= (aspace->base + aspace->size - 1) / SECTION_SIZE; i++) {
        DEBUG_ASSERT_MSG(aspace->tt_virt[i] == 0, "aspace %p still maps 0x%lx\n", aspace,
                         (vaddr_t)i * SECTION_SIZE);
    }

    vm_page_t *p;
    while ((p = list_remove_head_type(&aspace->pt_page_list, vm_page_t, node)) != NULL) {
        LTRACEF("freeing page %p\n", p);
        pmm_free_page(p);
    }

    return NO_ERROR;
}

bool arch_mmu_supports_nx_mappings(void) { return true; }
bool arch_mmu_supports_user_aspaces(void) { return true; }

/* arch_mmu_pick_spot() keeps secure and non-secure regions out of each other's
 * sections, since a page table's security state covers all of its pages */
bool arch_mmu_supports_ns_mappings(void) {
#if WITH_ARCH_MMU_PICK_SPOT
    return true;
#else
    return false;
#endif
}

#endif // ARCH_HAS_MMU
