/*
 * Copyright (c) 2014 Google Inc. All rights reserved
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <arch/arm64/mmu.h>
#include <arch/atomic.h>
#include <assert.h>
#include <kernel/vm.h>
#include <kernel/vm/asid.h>
#include <lib/heap.h>
#include <lk/bits.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/trace.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#define LOCAL_TRACE          0
#define TRACE_CONTEXT_SWITCH 0

STATIC_ASSERT(((long)KERNEL_BASE >> MMU_KERNEL_SIZE_SHIFT) == -1);
STATIC_ASSERT(((long)KERNEL_ASPACE_BASE >> MMU_KERNEL_SIZE_SHIFT) == -1);
STATIC_ASSERT(MMU_KERNEL_SIZE_SHIFT <= 48);
STATIC_ASSERT(MMU_KERNEL_SIZE_SHIFT >= 25);

/* a user top level table is allocated as a single page */
STATIC_ASSERT(MMU_USER_PAGE_TABLE_ENTRIES_TOP * sizeof(pte_t) <= PAGE_SIZE);

/* the main translation table */
pte_t arm64_kernel_translation_table[MMU_KERNEL_PAGE_TABLE_ENTRIES_TOP] __ALIGNED(MMU_KERNEL_PAGE_TABLE_ENTRIES_TOP * 8)
    __SECTION(".bss.prebss.translation_table");

/* the base TCR flags, computed from early init code in start.S */
uint64_t arm64_mmu_tcr_flags __SECTION(".bss.prebss.tcr_flags");

/* User aspaces each get their own asid when the cpu implements 16 bits of them,
 * so their TLB entries survive context switches. With only 8 bits there are too
 * few to hand out, so every user aspace shares one and each switch to a user
 * aspace flushes it on the local cpu. Decided from the TCR start.S computed, once
 * the kernel aspace is initialized. */
#define MMU_ARM64_SHARED_USER_ASID ASID_FIRST_USER
static bool arm64_asids_enabled;
static asid_allocator_t arm64_asid_allocator;

/*
 * TLB maintenance. Every change follows the same shape: write the descriptor,
 * dsb ishst so the table walkers on every cpu see the write before the
 * invalidate that depends on it, tlbi ...is to broadcast the invalidate, then
 * dsb ish to wait for it to complete everywhere before anything relies on the
 * old translation being gone (freeing a table, reusing an asid, returning to
 * the caller). Kernel mappings add an isb so this cpu's own instruction stream
 * and any speculated accesses pick up the change.
 */
static inline void arm64_tlb_sync_table_writes(void) {
    __asm__ volatile("dsb ishst" ::: "memory");
}

static inline void arm64_tlb_sync_invalidates(void) {
    __asm__ volatile("dsb ish" ::: "memory");
}

/* Invalidate the translation of one address on every cpu. terminal picks the
 * last-level-only forms, which leave cached intermediate walk entries alone;
 * clear it when removing a table descriptor so those go too. */
static void arm64_tlb_invalidate_va(vaddr_t vaddr, uint asid, bool terminal) {
    uint64_t val = BITS_SHIFT(vaddr, 55, 12);

    if (asid == MMU_ARM64_GLOBAL_ASID) {
        /* kernel entries are global, so match them regardless of asid */
        if (terminal) {
            ARM64_TLBI(vaale1is, val);
        } else {
            ARM64_TLBI(vaae1is, val);
        }
    } else {
        val |= (uint64_t)asid << 48;
        if (terminal) {
            ARM64_TLBI(vale1is, val);
        } else {
            ARM64_TLBI(vae1is, val);
        }
    }
}

/* convert user level mmu flags to flags that go in L1 descriptors */
static status_t mmu_flags_to_pte_attr(uint flags, pte_t *out) {
    pte_t attr = MMU_PTE_ATTR_AF;

    switch (flags & ARCH_MMU_FLAG_CACHE_MASK) {
        case ARCH_MMU_FLAG_CACHED:
            attr |= MMU_PTE_ATTR_NORMAL_MEMORY | MMU_PTE_ATTR_SH_INNER_SHAREABLE;
            break;
        case ARCH_MMU_FLAG_UNCACHED:
            attr |= MMU_PTE_ATTR_STRONGLY_ORDERED;
            break;
        case ARCH_MMU_FLAG_UNCACHED_DEVICE:
            attr |= MMU_PTE_ATTR_DEVICE;
            break;
        default:
            /* invalid user-supplied flag */
            DEBUG_ASSERT(0);
            return ERR_INVALID_ARGS;
    }

    switch (flags & (ARCH_MMU_FLAG_PERM_USER | ARCH_MMU_FLAG_PERM_WRITE)) {
        case 0:
            attr |= MMU_PTE_ATTR_AP_P_RO_U_NA;
            break;
        case ARCH_MMU_FLAG_PERM_WRITE:
            attr |= MMU_PTE_ATTR_AP_P_RW_U_NA;
            break;
        case ARCH_MMU_FLAG_PERM_USER:
            attr |= MMU_PTE_ATTR_AP_P_RO_U_RO;
            break;
        case ARCH_MMU_FLAG_PERM_USER | ARCH_MMU_FLAG_PERM_WRITE:
            attr |= MMU_PTE_ATTR_AP_P_RW_U_RW;
            break;
    }

    if (!(flags & ARCH_MMU_FLAG_PERM_EXECUTE)) {
        attr |= MMU_PTE_ATTR_UXN | MMU_PTE_ATTR_PXN;
    } else {
        // execute permissions, so set user or privileged XN based on which mode
        if (flags & ARCH_MMU_FLAG_PERM_USER) {
            attr |= MMU_PTE_ATTR_PXN;
        } else {
            attr |= MMU_PTE_ATTR_UXN;
        }
    }

    if (flags & ARCH_MMU_FLAG_NS) {
        attr |= MMU_PTE_ATTR_NON_SECURE;
    }

    *out = attr;
    return NO_ERROR;
}

status_t arch_mmu_query(arch_aspace_t *aspace, vaddr_t vaddr, paddr_t *paddr, uint *flags) {
    uint index;
    uint index_shift;
    uint page_size_shift;
    pte_t pte;
    pte_t pte_addr;
    uint descriptor_type;
    pte_t *page_table;
    vaddr_t vaddr_rem;

    LTRACEF("aspace %p, vaddr 0x%lx\n", aspace, vaddr);

    DEBUG_ASSERT(aspace);
    DEBUG_ASSERT(aspace->tt_virt);

    if (!arch_mmu_range_in_aspace(aspace, vaddr, 1)) {
        return ERR_OUT_OF_RANGE;
    }

    /* compute shift values based on if this address space is for kernel or user space */
    if (aspace->flags & ARCH_ASPACE_FLAG_KERNEL) {
        index_shift = MMU_KERNEL_TOP_SHIFT;
        page_size_shift = MMU_KERNEL_PAGE_SIZE_SHIFT;

        vaddr_t kernel_base = ~0UL << MMU_KERNEL_SIZE_SHIFT;
        vaddr_rem = vaddr - kernel_base;

        index = vaddr_rem >> index_shift;
        ASSERT(index < MMU_KERNEL_PAGE_TABLE_ENTRIES_TOP);
    } else {
        index_shift = MMU_USER_TOP_SHIFT;
        page_size_shift = MMU_USER_PAGE_SIZE_SHIFT;

        vaddr_rem = vaddr;
        index = vaddr_rem >> index_shift;
        ASSERT(index < MMU_USER_PAGE_TABLE_ENTRIES_TOP);
    }

    page_table = aspace->tt_virt;

    while (true) {
        index = vaddr_rem >> index_shift;
        vaddr_rem -= (vaddr_t)index << index_shift;
        pte = page_table[index];
        descriptor_type = pte & MMU_PTE_DESCRIPTOR_MASK;
        pte_addr = pte & MMU_PTE_OUTPUT_ADDR_MASK;

        LTRACEF("va 0x%lx, index %d, index_shift %d, rem 0x%lx, pte 0x%llx\n",
                vaddr, index, index_shift, vaddr_rem, pte);

        if (descriptor_type == MMU_PTE_DESCRIPTOR_INVALID) {
            return ERR_NOT_FOUND;
        }

        if (descriptor_type == ((index_shift > page_size_shift) ? MMU_PTE_L012_DESCRIPTOR_BLOCK : MMU_PTE_L3_DESCRIPTOR_PAGE)) {
            break;
        }

        if (index_shift <= page_size_shift ||
            descriptor_type != MMU_PTE_L012_DESCRIPTOR_TABLE) {
            PANIC_UNIMPLEMENTED;
        }

        page_table = paddr_to_kvaddr(pte_addr);
        index_shift -= page_size_shift - 3;
    }

    if (paddr) {
        *paddr = pte_addr + vaddr_rem;
    }
    if (flags) {
        *flags = ARCH_MMU_FLAG_PERM_READ;
        if (pte & MMU_PTE_ATTR_NON_SECURE) {
            *flags |= ARCH_MMU_FLAG_NS;
        }
        switch (pte & MMU_PTE_ATTR_ATTR_INDEX_MASK) {
            case MMU_PTE_ATTR_STRONGLY_ORDERED:
                *flags |= ARCH_MMU_FLAG_UNCACHED;
                break;
            case MMU_PTE_ATTR_DEVICE:
                *flags |= ARCH_MMU_FLAG_UNCACHED_DEVICE;
                break;
            case MMU_PTE_ATTR_NORMAL_MEMORY:
                break;
            default:
                PANIC_UNIMPLEMENTED;
        }
        switch (pte & MMU_PTE_ATTR_AP_MASK) {
            case MMU_PTE_ATTR_AP_P_RW_U_NA:
                *flags |= ARCH_MMU_FLAG_PERM_WRITE;
                break;
            case MMU_PTE_ATTR_AP_P_RW_U_RW:
                *flags |= ARCH_MMU_FLAG_PERM_USER | ARCH_MMU_FLAG_PERM_WRITE;
                break;
            case MMU_PTE_ATTR_AP_P_RO_U_NA:
                break;
            case MMU_PTE_ATTR_AP_P_RO_U_RO:
                *flags |= ARCH_MMU_FLAG_PERM_USER;
                break;
        }

        // a user page executes from user mode, a kernel page from the kernel,
        // so the matching XN bit decides
        if (*flags & ARCH_MMU_FLAG_PERM_USER) {
            if (!(pte & MMU_PTE_ATTR_UXN)) {
                *flags |= ARCH_MMU_FLAG_PERM_EXECUTE;
            }
        } else {
            if (!(pte & MMU_PTE_ATTR_PXN)) {
                *flags |= ARCH_MMU_FLAG_PERM_EXECUTE;
            }
        }
    }
    LTRACEF("va 0x%lx, paddr 0x%lx, flags 0x%x\n",
            vaddr, paddr ? *paddr : ~0UL, flags ? *flags : ~0U);
    return 0;
}

static int alloc_page_table(paddr_t *paddrp, uint page_size_shift) {
    size_t size = 1U << page_size_shift;

    LTRACEF("page_size_shift %u\n", page_size_shift);

    if (!vm_ptable_budget_take()) {
        return ERR_NO_MEMORY;
    }

    if (size == PAGE_SIZE) {
        vm_page_t *p = pmm_alloc_page();
        if (!p) {
            return ERR_NO_MEMORY;
        }
        *paddrp = vm_page_to_paddr(p);
    } else if (size > PAGE_SIZE) {
        size_t count = size / PAGE_SIZE;
        size_t ret = pmm_alloc_contiguous(count, page_size_shift, paddrp, NULL);
        if (ret != count) {
            return ERR_NO_MEMORY;
        }
    } else {
        void *vaddr = memalign(size, size);
        if (!vaddr) {
            return ERR_NO_MEMORY;
        }
        *paddrp = vaddr_to_paddr(vaddr);
        if (*paddrp == 0) {
            free(vaddr);
            return ERR_NO_MEMORY;
        }
    }

    LTRACEF("allocated 0x%lx\n", *paddrp);
    return 0;
}

static void free_page_table(void *vaddr, paddr_t paddr, uint page_size_shift) {
    LTRACEF("vaddr %p paddr 0x%lx page_size_shift %u\n", vaddr, paddr, page_size_shift);

    size_t size = 1U << page_size_shift;
    vm_page_t *page;

    if (size >= PAGE_SIZE) {
        page = paddr_to_vm_page(paddr);
        if (!page) {
            panic("bad page table paddr 0x%lx\n", paddr);
        }
        pmm_free_page(page);
    } else {
        free(vaddr);
    }
}

static pte_t *arm64_mmu_get_page_table(vaddr_t index, uint page_size_shift, pte_t *page_table) {
    pte_t pte;
    paddr_t paddr;
    void *vaddr;
    int ret;

    pte = page_table[index];
    switch (pte & MMU_PTE_DESCRIPTOR_MASK) {
        case MMU_PTE_DESCRIPTOR_INVALID:
            ret = alloc_page_table(&paddr, page_size_shift);
            if (ret) {
                TRACEF("failed to allocate page table\n");
                return NULL;
            }
            vaddr = paddr_to_kvaddr(paddr);

            LTRACEF("allocated page table, vaddr %p, paddr 0x%lx\n", vaddr, paddr);
            memset(vaddr, MMU_PTE_DESCRIPTOR_INVALID, 1U << page_size_shift);

            /* the walkers must observe the zeroed table before the descriptor that
             * points them at it */
            arm64_tlb_sync_table_writes();

            pte = paddr | MMU_PTE_L012_DESCRIPTOR_TABLE;
            page_table[index] = pte;
            LTRACEF("pte %p[0x%lx] = 0x%llx\n", page_table, index, pte);
            return vaddr;

        case MMU_PTE_L012_DESCRIPTOR_TABLE:
            paddr = pte & MMU_PTE_OUTPUT_ADDR_MASK;
            LTRACEF("found page table 0x%lx\n", paddr);
            return paddr_to_kvaddr(paddr);

        case MMU_PTE_L012_DESCRIPTOR_BLOCK:
            return NULL;

        default:
            PANIC_UNIMPLEMENTED;
    }
}

static bool page_table_is_clear(pte_t *page_table, uint page_size_shift) {
    const size_t count = 1UL << (page_size_shift - 3);
    for (size_t i = 0; i < count; i++) {
        const pte_t pte = page_table[i];
        if (pte != MMU_PTE_DESCRIPTOR_INVALID) {
            LTRACEF("page_table at %p still in use, index %zu is %#llx\n",
                    page_table, i, pte);
            return false;
        }
    }

    LTRACEF("page table at %p is clear\n", page_table);
    return true;
}

/* Unmap [vaddr, vaddr + size) below page_table, invalidating each entry as it
 * goes and freeing tables that end up empty. Returns ERR_NOT_SUPPORTED on a block
 * descriptor that covers more than the range, since splitting one needs a
 * break-before-make with a new table; whatever preceded it in the range is
 * already unmapped by then. */
static status_t arm64_mmu_unmap_pt(vaddr_t vaddr, vaddr_t vaddr_rel,
                                   size_t size,
                                   uint index_shift, uint page_size_shift,
                                   pte_t *page_table, uint asid) {
    pte_t *next_page_table;
    vaddr_t index;
    size_t chunk_size;
    vaddr_t vaddr_rem;
    vaddr_t block_size;
    vaddr_t block_mask;
    pte_t pte;
    paddr_t page_table_paddr;

    LTRACEF("vaddr 0x%lx, vaddr_rel 0x%lx, size 0x%lx, index shift %d, page_size_shift %d, page_table %p\n",
            vaddr, vaddr_rel, size, index_shift, page_size_shift, page_table);

    while (size) {
        block_size = 1UL << index_shift;
        block_mask = block_size - 1;
        vaddr_rem = vaddr_rel & block_mask;
        chunk_size = MIN(size, block_size - vaddr_rem);
        index = vaddr_rel >> index_shift;

        pte = page_table[index];

        if (index_shift > page_size_shift &&
            (pte & MMU_PTE_DESCRIPTOR_MASK) == MMU_PTE_L012_DESCRIPTOR_TABLE) {
            page_table_paddr = pte & MMU_PTE_OUTPUT_ADDR_MASK;
            next_page_table = paddr_to_kvaddr(page_table_paddr);
            status_t err = arm64_mmu_unmap_pt(vaddr, vaddr_rem, chunk_size,
                                              index_shift - (page_size_shift - 3),
                                              page_size_shift,
                                              next_page_table, asid);
            if (err < 0) {
                return err;
            }
            if (chunk_size == block_size ||
                page_table_is_clear(next_page_table, page_size_shift)) {
                LTRACEF("pte %p[0x%lx] = 0 (was page table)\n", page_table, index);
                page_table[index] = MMU_PTE_DESCRIPTOR_INVALID;

                /* The walkers may hold this descriptor in their walk caches, and
                 * a walk through the table may be in flight: invalidate the
                 * intermediate entry and wait for that to complete on every cpu
                 * before the page can be handed out again. */
                arm64_tlb_sync_table_writes();
                arm64_tlb_invalidate_va(vaddr, asid, false);
                arm64_tlb_sync_invalidates();

                free_page_table(next_page_table, page_table_paddr, page_size_shift);
            }
        } else if (pte) {
            if (index_shift > page_size_shift && chunk_size != block_size) {
                TRACEF("partial unmap of block descriptor at %p[0x%lx] (0x%llx) not supported\n",
                       page_table, index, pte);
                return ERR_NOT_SUPPORTED;
            }

            LTRACEF("pte %p[0x%lx] = 0\n", page_table, index);
            page_table[index] = MMU_PTE_DESCRIPTOR_INVALID;
            arm64_tlb_sync_table_writes();
            arm64_tlb_invalidate_va(vaddr, asid, true);
        } else {
            LTRACEF("pte %p[0x%lx] already clear\n", page_table, index);
        }
        vaddr += chunk_size;
        vaddr_rel += chunk_size;
        size -= chunk_size;
    }

    return NO_ERROR;
}

static int arm64_mmu_map_pt(vaddr_t vaddr_in, vaddr_t vaddr_rel_in,
                            paddr_t paddr_in,
                            size_t size_in, pte_t attrs,
                            uint index_shift, uint page_size_shift,
                            pte_t *page_table, uint asid) {
    int ret;
    pte_t *next_page_table;
    vaddr_t index;
    vaddr_t vaddr = vaddr_in;
    vaddr_t vaddr_rel = vaddr_rel_in;
    paddr_t paddr = paddr_in;
    size_t size = size_in;
    size_t chunk_size;
    vaddr_t vaddr_rem;
    vaddr_t block_size;
    vaddr_t block_mask;
    pte_t pte;

    LTRACEF("vaddr 0x%lx, vaddr_rel 0x%lx, paddr 0x%lx, size 0x%lx, attrs 0x%llx, index shift %d, page_size_shift %d, page_table %p\n",
            vaddr, vaddr_rel, paddr, size, attrs,
            index_shift, page_size_shift, page_table);

    if ((vaddr_rel | paddr | size) & ((1UL << page_size_shift) - 1)) {
        TRACEF("not page aligned\n");
        return ERR_INVALID_ARGS;
    }

    while (size) {
        block_size = 1UL << index_shift;
        block_mask = block_size - 1;
        vaddr_rem = vaddr_rel & block_mask;
        chunk_size = MIN(size, block_size - vaddr_rem);
        index = vaddr_rel >> index_shift;

        if (((vaddr_rel | paddr) & block_mask) ||
            (chunk_size != block_size) ||
            (index_shift > MMU_PTE_DESCRIPTOR_BLOCK_MAX_SHIFT)) {
            next_page_table = arm64_mmu_get_page_table(index, page_size_shift,
                                                       page_table);
            if (!next_page_table) {
                goto err;
            }

            ret = arm64_mmu_map_pt(vaddr, vaddr_rem, paddr, chunk_size, attrs,
                                   index_shift - (page_size_shift - 3),
                                   page_size_shift, next_page_table, asid);
            if (ret) {
                goto err;
            }
        } else {
            pte = page_table[index];
            if (pte) {
                TRACEF("page table entry already in use, index 0x%lx, 0x%llx\n",
                       index, pte);
                goto err;
            }

            pte = paddr | attrs;
            if (index_shift > page_size_shift) {
                pte |= MMU_PTE_L012_DESCRIPTOR_BLOCK;
            } else {
                pte |= MMU_PTE_L3_DESCRIPTOR_PAGE;
            }

            LTRACEF("pte %p[0x%lx] = 0x%llx\n", page_table, index, pte);
            page_table[index] = pte;
        }
        vaddr += chunk_size;
        vaddr_rel += chunk_size;
        paddr += chunk_size;
        size -= chunk_size;
    }

    return 0;

err:
    arm64_mmu_unmap_pt(vaddr_in, vaddr_rel_in, size_in - size,
                       index_shift, page_size_shift, page_table, asid);
    arm64_tlb_sync_invalidates();
    return ERR_GENERIC;
}

int arm64_mmu_map(vaddr_t vaddr, paddr_t paddr, size_t size, pte_t attrs,
                  vaddr_t vaddr_base, uint top_size_shift,
                  uint top_index_shift, uint page_size_shift,
                  pte_t *top_page_table, uint asid) {
    int ret;
    vaddr_t vaddr_rel = vaddr - vaddr_base;
    vaddr_t vaddr_rel_max = 1UL << top_size_shift;

    LTRACEF("vaddr 0x%lx, paddr 0x%lx, size 0x%lx, attrs 0x%llx, asid 0x%x\n",
            vaddr, paddr, size, attrs, asid);

    if (vaddr_rel > vaddr_rel_max - size || size > vaddr_rel_max) {
        TRACEF("vaddr 0x%lx, size 0x%lx out of range vaddr 0x%lx, size 0x%lx\n",
               vaddr, size, vaddr_base, vaddr_rel_max);
        return ERR_OUT_OF_RANGE;
    }

    if (!top_page_table) {
        TRACEF("page table is NULL\n");
        return ERR_INVALID_ARGS;
    }

    ret = arm64_mmu_map_pt(vaddr, vaddr_rel, paddr, size, attrs,
                           top_index_shift, page_size_shift, top_page_table, asid);

    /* The new descriptors must reach the walkers before the caller touches the
     * mapping. A kernel mapping may be used right away by this cpu, so also keep
     * it from running ahead on a translation speculated before the write; user
     * mappings get that from the exception return into user space. */
    arm64_tlb_sync_table_writes();
    if (asid == MMU_ARM64_GLOBAL_ASID) {
        ISB;
    }
    return ret;
}

int arm64_mmu_unmap(vaddr_t vaddr, size_t size,
                    vaddr_t vaddr_base, uint top_size_shift,
                    uint top_index_shift, uint page_size_shift,
                    pte_t *top_page_table, uint asid) {
    vaddr_t vaddr_rel = vaddr - vaddr_base;
    vaddr_t vaddr_rel_max = 1UL << top_size_shift;

    LTRACEF("vaddr 0x%lx, size 0x%lx, asid 0x%x\n", vaddr, size, asid);

    if (vaddr_rel > vaddr_rel_max - size || size > vaddr_rel_max) {
        TRACEF("vaddr 0x%lx, size 0x%lx out of range vaddr 0x%lx, size 0x%lx\n",
               vaddr, size, vaddr_base, vaddr_rel_max);
        return ERR_OUT_OF_RANGE;
    }

    if (!top_page_table) {
        TRACEF("page table is NULL\n");
        return ERR_INVALID_ARGS;
    }

    status_t err = arm64_mmu_unmap_pt(vaddr, vaddr_rel, size,
                                      top_index_shift, page_size_shift, top_page_table, asid);

    /* wait for every invalidate above to complete on every cpu */
    arm64_tlb_sync_invalidates();
    if (asid == MMU_ARM64_GLOBAL_ASID) {
        ISB;
    }
    return err;
}

int arch_mmu_map(arch_aspace_t *aspace, vaddr_t vaddr, paddr_t paddr, uint count, uint flags) {
    LTRACEF("vaddr 0x%lx paddr 0x%lx count %u flags 0x%x\n", vaddr, paddr, count, flags);

    DEBUG_ASSERT(aspace);
    DEBUG_ASSERT(aspace->tt_virt);

    if (!arch_mmu_range_in_aspace(aspace, vaddr, count)) {
        return ERR_OUT_OF_RANGE;
    }

    /* paddr and vaddr must be aligned */
    DEBUG_ASSERT(IS_PAGE_ALIGNED(vaddr));
    DEBUG_ASSERT(IS_PAGE_ALIGNED(paddr));
    if (!IS_PAGE_ALIGNED(vaddr) || !IS_PAGE_ALIGNED(paddr)) {
        return ERR_INVALID_ARGS;
    }

    /* there are no write-only or execute-only mappings */
    if (!(flags & ARCH_MMU_FLAG_PERM_READ)) {
        return ERR_INVALID_ARGS;
    }

    if (count == 0) {
        return NO_ERROR;
    }

    pte_t attrs;
    status_t err = mmu_flags_to_pte_attr(flags, &attrs);
    if (err < 0) {
        return err;
    }

    int ret;
    if (aspace->flags & ARCH_ASPACE_FLAG_KERNEL) {
        ret = arm64_mmu_map(vaddr, paddr, count * PAGE_SIZE, attrs,
                            ~0UL << MMU_KERNEL_SIZE_SHIFT, MMU_KERNEL_SIZE_SHIFT,
                            MMU_KERNEL_TOP_SHIFT, MMU_KERNEL_PAGE_SIZE_SHIFT,
                            aspace->tt_virt, MMU_ARM64_GLOBAL_ASID);
    } else {
        /* user entries are tagged with the aspace's asid rather than shared by all */
        attrs |= MMU_PTE_ATTR_NON_GLOBAL;
        ret = arm64_mmu_map(vaddr, paddr, count * PAGE_SIZE, attrs,
                            0, MMU_USER_SIZE_SHIFT,
                            MMU_USER_TOP_SHIFT, MMU_USER_PAGE_SIZE_SHIFT,
                            aspace->tt_virt, aspace->asid);
    }

    return ret;
}

int arch_mmu_unmap(arch_aspace_t *aspace, vaddr_t vaddr, uint count) {
    LTRACEF("vaddr 0x%lx count %u\n", vaddr, count);

    DEBUG_ASSERT(aspace);
    DEBUG_ASSERT(aspace->tt_virt);

    if (!arch_mmu_range_in_aspace(aspace, vaddr, count)) {
        return ERR_OUT_OF_RANGE;
    }

    DEBUG_ASSERT(IS_PAGE_ALIGNED(vaddr));
    if (!IS_PAGE_ALIGNED(vaddr)) {
        return ERR_INVALID_ARGS;
    }

    int ret;
    if (aspace->flags & ARCH_ASPACE_FLAG_KERNEL) {
        ret = arm64_mmu_unmap(vaddr, count * PAGE_SIZE,
                              ~0UL << MMU_KERNEL_SIZE_SHIFT, MMU_KERNEL_SIZE_SHIFT,
                              MMU_KERNEL_TOP_SHIFT, MMU_KERNEL_PAGE_SIZE_SHIFT,
                              aspace->tt_virt,
                              MMU_ARM64_GLOBAL_ASID);
    } else {
        ret = arm64_mmu_unmap(vaddr, count * PAGE_SIZE,
                              0, MMU_USER_SIZE_SHIFT,
                              MMU_USER_TOP_SHIFT, MMU_USER_PAGE_SIZE_SHIFT,
                              aspace->tt_virt,
                              aspace->asid);
    }

    return ret;
}

status_t arch_mmu_init_aspace(arch_aspace_t *aspace, vaddr_t base, size_t size, uint flags) {
    LTRACEF("aspace %p, base 0x%lx, size 0x%zx, flags 0x%x\n", aspace, base, size, flags);

    DEBUG_ASSERT(aspace);

    /* validate that the base + size is sane and doesn't wrap */
    DEBUG_ASSERT(size > PAGE_SIZE);
    DEBUG_ASSERT(base + size - 1 > base);

    aspace->flags = flags;
    aspace->active_cpus = 0;
    if (flags & ARCH_ASPACE_FLAG_KERNEL) {
        /* at the moment we can only deal with address spaces as globally defined */
        DEBUG_ASSERT(base == ~0UL << MMU_KERNEL_SIZE_SHIFT);
        DEBUG_ASSERT(size == 1UL << MMU_KERNEL_SIZE_SHIFT);

        aspace->base = base;
        aspace->size = size;
        aspace->tt_virt = arm64_kernel_translation_table;
        aspace->tt_phys = vaddr_to_paddr(aspace->tt_virt);
        aspace->asid = ASID_KERNEL;

        /* The kernel aspace comes first, before any user aspace exists, so set
         * up the asid scheme here from what start.S found in the cpu. */
        arm64_asids_enabled = (arm64_mmu_tcr_flags & MMU_TCR_AS) != 0;
        asid_allocator_init(&arm64_asid_allocator, ASID_MAX);
    } else {
        // DEBUG_ASSERT(base >= 0);
        DEBUG_ASSERT(base + size <= 1UL << MMU_USER_SIZE_SHIFT);
        DEBUG_ASSERT(arm64_asid_allocator.max != 0);

        aspace->base = base;
        aspace->size = size;

        if (arm64_asids_enabled) {
            status_t err = asid_alloc(&arm64_asid_allocator, &aspace->asid);
            if (err < 0) {
                return err;
            }
        } else {
            aspace->asid = MMU_ARM64_SHARED_USER_ASID;
        }

        pte_t *va = pmm_alloc_kpages(1, NULL);
        if (!va) {
            if (arm64_asids_enabled) {
                asid_free(&arm64_asid_allocator, aspace->asid);
            }
            return ERR_NO_MEMORY;
        }

        aspace->tt_virt = va;
        aspace->tt_phys = vaddr_to_paddr(aspace->tt_virt);

        /* zero the top level translation table */
        /* XXX remove when PMM starts returning pre-zeroed pages */
        memset(aspace->tt_virt, 0, PAGE_SIZE);

        /* the walkers must see the empty table before a TTBR0 write points them at it */
        arm64_tlb_sync_table_writes();
    }

    LTRACEF("tt_phys 0x%lx tt_virt %p asid %#x\n", aspace->tt_phys, aspace->tt_virt, aspace->asid);

    return NO_ERROR;
}

status_t arch_mmu_destroy_aspace(arch_aspace_t *aspace) {
    LTRACEF("aspace %p\n", aspace);

    DEBUG_ASSERT(aspace);
    DEBUG_ASSERT((aspace->flags & ARCH_ASPACE_FLAG_KERNEL) == 0);
    DEBUG_ASSERT(aspace->asid >= ASID_FIRST_USER);

    /* no cpu may still be walking these tables, and every mapping must already be
     * gone: unmapping is what frees the lower level tables */
    DEBUG_ASSERT(__atomic_load_n(&aspace->active_cpus, __ATOMIC_RELAXED) == 0);
    DEBUG_ASSERT(page_table_is_clear(aspace->tt_virt, MMU_USER_PAGE_SIZE_SHIFT));

    /* Drop whatever the TLBs still hold under this asid on every cpu, walk cache
     * entries included, and wait for that to finish before the asid or the root
     * table can be reused. */
    arm64_tlb_sync_table_writes();
    ARM64_TLBI(aside1is, (uint64_t)aspace->asid << 48);
    arm64_tlb_sync_invalidates();

    if (arm64_asids_enabled) {
        asid_free(&arm64_asid_allocator, aspace->asid);
    }
    aspace->asid = ASID_UNUSED;

    vm_page_t *page = paddr_to_vm_page(aspace->tt_phys);
    DEBUG_ASSERT(page);
    pmm_free_page(page);
    aspace->tt_virt = NULL;
    aspace->tt_phys = 0;

    return NO_ERROR;
}

/*
 * TTBR0 walks are disabled (TCR_EL1.EPD0) whenever no user aspace is loaded,
 * from boot onwards, so a user aspace is only ever reachable through the
 * TTBR0 value written here. Each ARM64_WRITE_SYSREG ends in an isb.
 */
void arch_mmu_context_switch(arch_aspace_t *old_aspace, arch_aspace_t *aspace) {
    if (TRACE_CONTEXT_SWITCH) {
        TRACEF("old aspace %p, aspace %p\n", old_aspace, aspace);
    }

    DEBUG_ASSERT(!old_aspace || (old_aspace->flags & ARCH_ASPACE_FLAG_KERNEL) == 0);

    if (aspace) {
        DEBUG_ASSERT((aspace->flags & ARCH_ASPACE_FLAG_KERNEL) == 0);
        DEBUG_ASSERT(aspace->asid >= ASID_FIRST_USER);

        const uint64_t ttbr = ((uint64_t)aspace->asid << 48) | aspace->tt_phys;

        /* TTBR0 walks are on exactly when a user aspace is loaded */
        bool walks_enabled = (old_aspace != NULL);

        if (!arm64_asids_enabled && old_aspace != aspace) {
            /* Every user aspace shares the asid, so whatever the TLB holds under
             * it (from the outgoing aspace, or from one unloaded earlier) must
             * go before the new tables are walked. Turn walks off first so
             * nothing can be speculatively refilled from the old tables between
             * the invalidate and the switch. */
            if (walks_enabled) {
                ARM64_WRITE_SYSREG(tcr_el1, arm64_mmu_tcr_flags | MMU_TCR_FLAGS_KERNEL);
                walks_enabled = false;
            }
            ARM64_TLBI(aside1, (uint64_t)aspace->asid << 48);
            arm64_tlb_sync_invalidates();
        }

        /* the asid and table base change together in the one TTBR0 write, then
         * walks are enabled if they were off */
        if (old_aspace != aspace) {
            ARM64_WRITE_SYSREG(ttbr0_el1, ttbr);
        }
        if (!walks_enabled) {
            ARM64_WRITE_SYSREG(tcr_el1, arm64_mmu_tcr_flags | MMU_TCR_FLAGS_USER);
        }

        if (TRACE_CONTEXT_SWITCH) {
            TRACEF("ttbr 0x%llx\n", ttbr);
        }
    } else {
        /* kernel only: stop TTBR0 walks, then drop the stale table pointer */
        ARM64_WRITE_SYSREG(tcr_el1, arm64_mmu_tcr_flags | MMU_TCR_FLAGS_KERNEL);
        ARM64_WRITE_SYSREG(ttbr0_el1, 0);
    }

    if (old_aspace) {
        __UNUSED int prev = atomic_add(&old_aspace->active_cpus, -1);
        DEBUG_ASSERT(prev > 0);
    }
    if (aspace) {
        __UNUSED int prev = atomic_add(&aspace->active_cpus, 1);
        DEBUG_ASSERT(prev < SMP_MAX_CPUS);
    }
}

bool arch_mmu_supports_nx_mappings(void) {
    return true;
}
bool arch_mmu_supports_ns_mappings(void) {
    return true;
}
bool arch_mmu_supports_user_aspaces(void) {
    return true;
}
