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
 * TLB maintenance. Every change follows the same shape: write the descriptors,
 * dsb ishst so the table walkers on every cpu see the writes before the
 * invalidates that depend on them, tlbi ...is to broadcast each invalidate, then
 * dsb ish to wait for them to complete everywhere before anything relies on the
 * old translations being gone (freeing a table, reusing an asid, returning to
 * the caller). Kernel mappings add an isb so this cpu's own instruction stream
 * and any speculated accesses pick up the change. arm64_tlb_batch below runs
 * that shape over a whole unmap or protect call.
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

/*
 * The invalidates one unmap or protect call issues, so the descriptor writes
 * behind them share a dsb ishst and the tables they empty are handed back only
 * after every invalidate has completed. Above ARM64_TLB_BATCH_MAX by-address
 * invalidates the call drops them for one invalidate of the whole asid (the
 * whole TLB for the kernel) at the end instead, as Linux does: past that point
 * the broadcasts cost more than the refills they save.
 */
#define ARM64_TLB_BATCH_MAX 1024

struct arm64_tlb_batch {
    uint asid;
    uint count;                 /* by-address invalidates asked for */
    bool flush_all;
    uint npending;
    vaddr_t pending[32];        /* page aligned, bit 0 set for a last-level invalidate */
    struct list_node freed_tables;
};

static void arm64_tlb_batch_init(struct arm64_tlb_batch *b, uint asid) {
    b->asid = asid;
    b->count = 0;
    b->flush_all = false;
    b->npending = 0;
    list_initialize(&b->freed_tables);
}

/* issue the pending invalidates, after the descriptor writes behind them */
static void arm64_tlb_batch_issue(struct arm64_tlb_batch *b) {
    if (b->npending == 0) {
        return;
    }
    arm64_tlb_sync_table_writes();
    for (uint i = 0; i < b->npending; i++) {
        arm64_tlb_invalidate_va(b->pending[i] & ~1UL, b->asid, b->pending[i] & 1);
    }
    b->npending = 0;
}

/* called after the descriptor for vaddr was rewritten; terminal as for arm64_tlb_invalidate_va */
static void arm64_tlb_batch_invalidate(struct arm64_tlb_batch *b, vaddr_t vaddr, bool terminal) {
    if (b->flush_all) {
        return;
    }
    if (b->count++ >= ARM64_TLB_BATCH_MAX) {
        b->flush_all = true;
        b->npending = 0;
        return;
    }
    if (b->npending == countof(b->pending)) {
        arm64_tlb_batch_issue(b);
    }
    b->pending[b->npending++] = vaddr | (terminal ? 1 : 0);
}

/* a page table whose descriptor was just cleared; freed by arm64_tlb_batch_finish() */
static void arm64_tlb_batch_free_table(struct arm64_tlb_batch *b, paddr_t paddr) {
    vm_page_t *page = paddr_to_vm_page(paddr);
    if (!page) {
        panic("bad page table paddr 0x%lx\n", paddr);
    }
    list_add_tail(&b->freed_tables, &page->node);
}

/* Complete every invalidate on every cpu, then free the emptied tables. Kernel
 * mappings add an isb so this cpu's own instruction stream and any speculated
 * accesses pick up the change. */
static void arm64_tlb_batch_finish(struct arm64_tlb_batch *b) {
    if (b->flush_all) {
        arm64_tlb_sync_table_writes();
        if (b->asid == MMU_ARM64_GLOBAL_ASID) {
            ARM64_TLBI_NOADDR(vmalle1is);
        } else {
            ARM64_TLBI(aside1is, (uint64_t)b->asid << 48);
        }
    } else {
        arm64_tlb_batch_issue(b);
    }
    arm64_tlb_sync_invalidates();
    if (b->asid == MMU_ARM64_GLOBAL_ASID) {
        ISB;
    }

    if (!list_is_empty(&b->freed_tables)) {
        vm_ptable_freed(pmm_free(&b->freed_tables));
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

/* How an aspace's tables are laid out: the address the top table's first entry
 * maps, the shifts, the top table and the asid its entries are tagged with. */
struct arm64_aspace_layout {
    vaddr_t base;
    uint size_shift;
    uint top_index_shift;
    uint page_size_shift;
    pte_t *top;
    uint asid;
};

static struct arm64_aspace_layout arm64_layout(const arch_aspace_t *aspace) {
    if (aspace->flags & ARCH_ASPACE_FLAG_KERNEL) {
        return (struct arm64_aspace_layout){
            .base = ~0UL << MMU_KERNEL_SIZE_SHIFT,
            .size_shift = MMU_KERNEL_SIZE_SHIFT,
            .top_index_shift = MMU_KERNEL_TOP_SHIFT,
            .page_size_shift = MMU_KERNEL_PAGE_SIZE_SHIFT,
            .top = aspace->tt_virt,
            .asid = MMU_ARM64_GLOBAL_ASID,
        };
    }
    return (struct arm64_aspace_layout){
        .base = 0,
        .size_shift = MMU_USER_SIZE_SHIFT,
        .top_index_shift = MMU_USER_TOP_SHIFT,
        .page_size_shift = MMU_USER_PAGE_SIZE_SHIFT,
        .top = aspace->tt_virt,
        .asid = aspace->asid,
    };
}

/* The descriptor that ends the walk for vaddr, which must lie in the aspace: a
 * page, a block, or an empty entry, with *shift the log2 of the span it covers.
 * Blocks can sit at the top level: with a 64K granule a kernel top level entry
 * is a 512MB block. */
static pte_t arm64_mmu_leaf_at(const arch_aspace_t *aspace, vaddr_t vaddr, uint *shift) {
    const struct arm64_aspace_layout l = arm64_layout(aspace);
    const pte_t *page_table = l.top;
    uint index_shift = l.top_index_shift;
    vaddr_t vaddr_rem = vaddr - l.base;

    for (;;) {
        const vaddr_t index = vaddr_rem >> index_shift;
        vaddr_rem -= index << index_shift;
        const pte_t pte = page_table[index];

        LTRACEF_LEVEL(2, "va 0x%lx, index %lu, index_shift %u, pte 0x%llx\n", vaddr, index, index_shift, pte);

        if (index_shift > l.page_size_shift &&
            (pte & MMU_PTE_DESCRIPTOR_MASK) == MMU_PTE_L012_DESCRIPTOR_TABLE) {
            page_table = paddr_to_kvaddr(pte & MMU_PTE_OUTPUT_ADDR_MASK);
            index_shift -= l.page_size_shift - 3;
            continue;
        }
        *shift = index_shift;
        return pte;
    }
}

status_t arch_mmu_query(arch_aspace_t *aspace, vaddr_t vaddr, paddr_t *paddr, uint *flags) {
    LTRACEF("aspace %p, vaddr 0x%lx\n", aspace, vaddr);

    DEBUG_ASSERT(aspace);
    DEBUG_ASSERT(aspace->tt_virt);

    if (!arch_mmu_range_in_aspace(aspace, vaddr, 1)) {
        return ERR_OUT_OF_RANGE;
    }

    uint shift;
    const pte_t pte = arm64_mmu_leaf_at(aspace, vaddr, &shift);
    const uint descriptor_type = pte & MMU_PTE_DESCRIPTOR_MASK;
    if (descriptor_type == MMU_PTE_DESCRIPTOR_INVALID) {
        return ERR_NOT_FOUND;
    }
    const uint page_size_shift = (aspace->flags & ARCH_ASPACE_FLAG_KERNEL) ?
                                 MMU_KERNEL_PAGE_SIZE_SHIFT : MMU_USER_PAGE_SIZE_SHIFT;
    if (descriptor_type != ((shift > page_size_shift) ? MMU_PTE_L012_DESCRIPTOR_BLOCK : MMU_PTE_L3_DESCRIPTOR_PAGE)) {
        PANIC_UNIMPLEMENTED;
    }

    if (paddr) {
        *paddr = (pte & MMU_PTE_OUTPUT_ADDR_MASK) + (vaddr & ((1UL << shift) - 1));
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
        if (!(pte & MMU_PTE_ATTR_AP_RO)) {
            *flags |= ARCH_MMU_FLAG_PERM_WRITE;
        }
        /* a user page executes from user mode, a kernel page from the kernel,
         * so the matching XN bit decides */
        if (pte & MMU_PTE_ATTR_AP_USER) {
            *flags |= ARCH_MMU_FLAG_PERM_USER;
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
    return NO_ERROR;
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

/* The table the entry at index points to, allocating and linking an empty one
 * if the entry is empty. ERR_NO_MEMORY if that fails, ERR_ALREADY_EXISTS if a
 * block is there. */
static status_t arm64_mmu_get_page_table(vaddr_t index, uint page_size_shift, pte_t *page_table,
                                         pte_t **next) {
    pte_t pte = page_table[index];
    paddr_t paddr;

    switch (pte & MMU_PTE_DESCRIPTOR_MASK) {
        case MMU_PTE_DESCRIPTOR_INVALID: {
            status_t err = alloc_page_table(&paddr, page_size_shift);
            if (err != NO_ERROR) {
                LTRACEF("failed to allocate page table\n");
                return err;
            }
            void *vaddr = paddr_to_kvaddr(paddr);

            LTRACEF("allocated page table, vaddr %p, paddr 0x%lx\n", vaddr, paddr);
            memset(vaddr, MMU_PTE_DESCRIPTOR_INVALID, 1U << page_size_shift);

            /* the walkers must observe the zeroed table before the descriptor that
             * points them at it */
            arm64_tlb_sync_table_writes();

            pte = paddr | MMU_PTE_L012_DESCRIPTOR_TABLE;
            page_table[index] = pte;
            LTRACEF("pte %p[0x%lx] = 0x%llx\n", page_table, index, pte);
            *next = vaddr;
            return NO_ERROR;
        }
        case MMU_PTE_L012_DESCRIPTOR_TABLE:
            paddr = pte & MMU_PTE_OUTPUT_ADDR_MASK;
            LTRACEF("found page table 0x%lx\n", paddr);
            *next = paddr_to_kvaddr(paddr);
            return NO_ERROR;

        case MMU_PTE_L012_DESCRIPTOR_BLOCK:
            return ERR_ALREADY_EXISTS;

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

/* Unmap [vaddr, vaddr + size) below page_table, queueing an invalidate for each
 * entry and handing tables that end up empty to the batch. The caller has made
 * sure the range cuts no block; one that does is refused with ERR_NOT_SUPPORTED. */
static status_t arm64_mmu_unmap_pt(vaddr_t vaddr, vaddr_t vaddr_rel,
                                   size_t size,
                                   uint index_shift, uint page_size_shift,
                                   pte_t *page_table, struct arm64_tlb_batch *batch) {
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
                                              next_page_table, batch);
            if (err < 0) {
                return err;
            }
            if (chunk_size == block_size ||
                page_table_is_clear(next_page_table, page_size_shift)) {
                LTRACEF("pte %p[0x%lx] = 0 (was page table)\n", page_table, index);
                page_table[index] = MMU_PTE_DESCRIPTOR_INVALID;

                /* The walkers may hold this descriptor in their walk caches, and
                 * a walk through the table may be in flight: the intermediate
                 * entry is invalidated too, and the page is handed out again
                 * only once that has completed on every cpu. */
                arm64_tlb_batch_invalidate(batch, vaddr, false);
                arm64_tlb_batch_free_table(batch, page_table_paddr);
            }
        } else if (pte) {
            if (index_shift > page_size_shift && chunk_size != block_size) {
                DEBUG_ASSERT_MSG(false, "partial unmap of block descriptor at %p[0x%lx] (0x%llx)\n",
                                 page_table, index, pte);
                return ERR_NOT_SUPPORTED;
            }

            LTRACEF("pte %p[0x%lx] = 0\n", page_table, index);
            page_table[index] = MMU_PTE_DESCRIPTOR_INVALID;
            arm64_tlb_batch_invalidate(batch, vaddr, true);
        } else {
            LTRACEF("pte %p[0x%lx] already clear\n", page_table, index);
        }
        vaddr += chunk_size;
        vaddr_rel += chunk_size;
        size -= chunk_size;
    }

    return NO_ERROR;
}

/* Map [vaddr, vaddr + size) below page_table. An existing table is descended
 * into; a page or block in the way stops the walk with ERR_ALREADY_EXISTS, and
 * a table that cannot be allocated with ERR_NO_MEMORY, *fail_vaddr the address
 * it stopped at. Everything before that is mapped, nothing after it, and the
 * caller takes back what went in. */
static status_t arm64_mmu_map_pt(vaddr_t vaddr, vaddr_t vaddr_rel,
                                 paddr_t paddr,
                                 size_t size, pte_t attrs,
                                 uint index_shift, uint page_size_shift,
                                 pte_t *page_table, vaddr_t *fail_vaddr) {
    LTRACEF("vaddr 0x%lx, vaddr_rel 0x%lx, paddr 0x%lx, size 0x%lx, attrs 0x%llx, index shift %d, page_size_shift %d, page_table %p\n",
            vaddr, vaddr_rel, paddr, size, attrs,
            index_shift, page_size_shift, page_table);

    DEBUG_ASSERT(((vaddr_rel | paddr | size) & ((1UL << page_size_shift) - 1)) == 0);

    while (size) {
        const vaddr_t block_size = 1UL << index_shift;
        const vaddr_t block_mask = block_size - 1;
        const vaddr_t vaddr_rem = vaddr_rel & block_mask;
        const size_t chunk_size = MIN(size, block_size - vaddr_rem);
        const vaddr_t index = vaddr_rel >> index_shift;
        const pte_t pte = page_table[index];

        const bool page_level = index_shift == page_size_shift;
        const bool table_there = !page_level &&
                                 (pte & MMU_PTE_DESCRIPTOR_MASK) == MMU_PTE_L012_DESCRIPTOR_TABLE;
        const bool fits_block = ((vaddr_rel | paddr) & block_mask) == 0 && chunk_size == block_size &&
                                index_shift <= MMU_PTE_DESCRIPTOR_BLOCK_MAX_SHIFT;

        if (table_there || (!page_level && !fits_block && pte == MMU_PTE_DESCRIPTOR_INVALID)) {
            /* go down into the table there, or a fresh one */
            pte_t *next_page_table;
            status_t err = arm64_mmu_get_page_table(index, page_size_shift, page_table, &next_page_table);
            if (err != NO_ERROR) {
                *fail_vaddr = vaddr;
                return err;
            }
            err = arm64_mmu_map_pt(vaddr, vaddr_rem, paddr, chunk_size, attrs,
                                   index_shift - (page_size_shift - 3),
                                   page_size_shift, next_page_table, fail_vaddr);
            if (err != NO_ERROR) {
                return err;
            }
        } else if (pte != MMU_PTE_DESCRIPTOR_INVALID) {
            LTRACEF("page table entry already in use, index 0x%lx, 0x%llx\n", index, pte);
            *fail_vaddr = vaddr;
            return ERR_ALREADY_EXISTS;
        } else {
            const pte_t new_pte = paddr | attrs |
                                  (page_level ? MMU_PTE_L3_DESCRIPTOR_PAGE : MMU_PTE_L012_DESCRIPTOR_BLOCK);
            LTRACEF("pte %p[0x%lx] = 0x%llx\n", page_table, index, new_pte);
            page_table[index] = new_pte;
        }
        vaddr += chunk_size;
        vaddr_rel += chunk_size;
        paddr += chunk_size;
        size -= chunk_size;
    }

    return NO_ERROR;
}

/* the entry with its permissions replaced from flags: AP[2] makes it read only
 * without WRITE, and it executes only in its own mode, user (AP[1]) or kernel,
 * and only with EXECUTE */
static pte_t arm64_pte_with_perms(pte_t pte, uint flags) {
    pte &= ~(MMU_PTE_ATTR_AP_RO | MMU_PTE_ATTR_UXN | MMU_PTE_ATTR_PXN);
    if (!(flags & ARCH_MMU_FLAG_PERM_WRITE)) {
        pte |= MMU_PTE_ATTR_AP_RO;
    }
    if (pte & MMU_PTE_ATTR_AP_USER) {
        pte |= MMU_PTE_ATTR_PXN;
        if (!(flags & ARCH_MMU_FLAG_PERM_EXECUTE)) {
            pte |= MMU_PTE_ATTR_UXN;
        }
    } else {
        pte |= MMU_PTE_ATTR_UXN;
        if (!(flags & ARCH_MMU_FLAG_PERM_EXECUTE)) {
            pte |= MMU_PTE_ATTR_PXN;
        }
    }
    return pte;
}

/* Give every page and block in [vaddr, vaddr + size) below page_table the
 * permissions in flags, queueing an invalidate for each entry that changes. A
 * permission change needs no break-before-make. The caller has made sure the range cuts
 * no block. With check_only nothing is written, and the walk only reports
 * ERR_INVALID_ARGS if flags asks for EXECUTE on a page that is not normal
 * memory. */
static status_t arm64_mmu_protect_pt(vaddr_t vaddr, vaddr_t vaddr_rel, size_t size,
                                     uint index_shift, uint page_size_shift,
                                     pte_t *page_table, uint flags, struct arm64_tlb_batch *batch,
                                     bool check_only) {
    while (size) {
        const vaddr_t block_size = 1UL << index_shift;
        const vaddr_t vaddr_rem = vaddr_rel & (block_size - 1);
        const size_t chunk_size = MIN(size, block_size - vaddr_rem);
        const vaddr_t index = vaddr_rel >> index_shift;
        const pte_t pte = page_table[index];

        if (index_shift > page_size_shift &&
            (pte & MMU_PTE_DESCRIPTOR_MASK) == MMU_PTE_L012_DESCRIPTOR_TABLE) {
            status_t err = arm64_mmu_protect_pt(vaddr, vaddr_rem, chunk_size,
                                                index_shift - (page_size_shift - 3), page_size_shift,
                                                paddr_to_kvaddr(pte & MMU_PTE_OUTPUT_ADDR_MASK),
                                                flags, batch, check_only);
            if (err != NO_ERROR) {
                return err;
            }
        } else if (pte) {
            DEBUG_ASSERT(index_shift == page_size_shift || chunk_size == block_size);
            if (check_only) {
                if ((flags & ARCH_MMU_FLAG_PERM_EXECUTE) &&
                    (pte & MMU_PTE_ATTR_ATTR_INDEX_MASK) != MMU_PTE_ATTR_NORMAL_MEMORY) {
                    LTRACEF("execute asked for on uncached memory at 0x%lx\n", vaddr);
                    return ERR_INVALID_ARGS;
                }
            } else {
                const pte_t new_pte = arm64_pte_with_perms(pte, flags);
                if (new_pte != pte) {
                    LTRACEF("pte %p[0x%lx] = 0x%llx (was 0x%llx)\n", page_table, index, new_pte, pte);
                    page_table[index] = new_pte;
                    arm64_tlb_batch_invalidate(batch, vaddr, true);
                }
            }
        }
        vaddr += chunk_size;
        vaddr_rel += chunk_size;
        size -= chunk_size;
    }

    return NO_ERROR;
}

/* unmap [vaddr, vaddr + size), then wait for every invalidate to complete on every cpu */
static status_t arm64_mmu_unmap(const struct arm64_aspace_layout *l, vaddr_t vaddr, size_t size) {
    LTRACEF("vaddr 0x%lx, size 0x%lx, asid 0x%x\n", vaddr, size, l->asid);

    struct arm64_tlb_batch batch;
    arm64_tlb_batch_init(&batch, l->asid);
    status_t err = arm64_mmu_unmap_pt(vaddr, vaddr - l->base, size,
                                      l->top_index_shift, l->page_size_shift, l->top, &batch);
    arm64_tlb_batch_finish(&batch);
    return err;
}

/* Whether unmap or protect may change [vaddr, vaddr + count pages) without
 * splitting a block, decided before either touches anything. Only the two ends
 * of the range can cut one. */
static status_t arm64_mmu_check_range_ends(const arch_aspace_t *aspace, vaddr_t vaddr, uint count) {
    const vaddr_t last = vaddr + ((vaddr_t)count - 1) * PAGE_SIZE;
    const vaddr_t ends[] = { vaddr, last };
    for (uint i = 0; i < countof(ends); i++) {
        uint shift;
        const pte_t pte = arm64_mmu_leaf_at(aspace, ends[i], &shift);
        if ((pte & MMU_PTE_DESCRIPTOR_MASK) != MMU_PTE_L012_DESCRIPTOR_BLOCK) {
            continue;
        }
        const vaddr_t block = ROUNDDOWN(ends[i], 1UL << shift);
        const vaddr_t block_last = block + ((1UL << shift) - 1);
        if (block < vaddr || block_last > last + (PAGE_SIZE - 1)) {
            LTRACEF("range at 0x%lx covers part of a block at 0x%lx\n", vaddr, block);
            return ERR_NOT_SUPPORTED;
        }
    }
    return NO_ERROR;
}

status_t arch_mmu_map(arch_aspace_t *aspace, vaddr_t vaddr, paddr_t paddr, uint count, uint flags) {
    LTRACEF("vaddr 0x%lx paddr 0x%lx count %u flags 0x%x\n", vaddr, paddr, count, flags);

    DEBUG_ASSERT(aspace);
    DEBUG_ASSERT(aspace->tt_virt);

    /* the output address is bits [47:12] of a descriptor */
    if (!IS_PAGE_ALIGNED(vaddr) || !IS_PAGE_ALIGNED(paddr) ||
        !arch_mmu_paddr_range_valid(paddr, count, 48) || !arch_mmu_flags_valid(flags)) {
        return ERR_INVALID_ARGS;
    }

    if (!arch_mmu_range_in_aspace(aspace, vaddr, count)) {
        return ERR_OUT_OF_RANGE;
    }

    if (count == 0) {
        return NO_ERROR;
    }

    pte_t attrs;
    status_t err = mmu_flags_to_pte_attr(flags, &attrs);
    if (err < 0) {
        return err;
    }

    /* user entries are tagged with the aspace's asid rather than shared by all */
    const struct arm64_aspace_layout l = arm64_layout(aspace);
    if (!(aspace->flags & ARCH_ASPACE_FLAG_KERNEL)) {
        attrs |= MMU_PTE_ATTR_NON_GLOBAL;
    }

    vaddr_t fail_vaddr = vaddr;
    err = arm64_mmu_map_pt(vaddr, vaddr - l.base, paddr, count * PAGE_SIZE, attrs,
                           l.top_index_shift, l.page_size_shift, l.top, &fail_vaddr);
    if (err != NO_ERROR) {
        /* Take back what went in: the pages before a collision, or through the
         * page a table was missing for, so the empty tables linked for it are
         * reclaimed as well. */
        const size_t done = (fail_vaddr - vaddr) + (err == ERR_NO_MEMORY ? PAGE_SIZE : 0);
        if (done > 0) {
            __UNUSED status_t unmap_err = arm64_mmu_unmap(&l, vaddr, done);
            DEBUG_ASSERT(unmap_err == NO_ERROR);
        }
        return err;
    }

    /* The new descriptors must reach the walkers before the caller touches the
     * mapping. A kernel mapping may be used right away by this cpu, so also keep
     * it from running ahead on a translation speculated before the write; user
     * mappings get that from the exception return into user space. */
    arm64_tlb_sync_table_writes();
    if (aspace->flags & ARCH_ASPACE_FLAG_KERNEL) {
        ISB;
    }
    return NO_ERROR;
}

status_t arch_mmu_unmap(arch_aspace_t *aspace, vaddr_t vaddr, uint count) {
    LTRACEF("vaddr 0x%lx count %u\n", vaddr, count);

    DEBUG_ASSERT(aspace);
    DEBUG_ASSERT(aspace->tt_virt);

    if (!IS_PAGE_ALIGNED(vaddr)) {
        return ERR_INVALID_ARGS;
    }

    if (!arch_mmu_range_in_aspace(aspace, vaddr, count)) {
        return ERR_OUT_OF_RANGE;
    }

    if (count == 0) {
        return NO_ERROR;
    }

    status_t err = arm64_mmu_check_range_ends(aspace, vaddr, count);
    if (err != NO_ERROR) {
        return err;
    }

    const struct arm64_aspace_layout l = arm64_layout(aspace);
    return arm64_mmu_unmap(&l, vaddr, count * PAGE_SIZE);
}

status_t arch_mmu_protect(arch_aspace_t *aspace, vaddr_t vaddr, uint count, uint flags) {
    LTRACEF("vaddr 0x%lx count %u flags 0x%x\n", vaddr, count, flags);

    DEBUG_ASSERT(aspace);
    DEBUG_ASSERT(aspace->tt_virt);

    if (!IS_PAGE_ALIGNED(vaddr) || !arch_mmu_protect_flags_valid(flags)) {
        return ERR_INVALID_ARGS;
    }

    if (!arch_mmu_range_in_aspace(aspace, vaddr, count)) {
        return ERR_OUT_OF_RANGE;
    }

    if (count == 0) {
        return NO_ERROR;
    }

    status_t err = arm64_mmu_check_range_ends(aspace, vaddr, count);
    if (err != NO_ERROR) {
        return err;
    }

    const struct arm64_aspace_layout l = arm64_layout(aspace);
    const size_t size = count * PAGE_SIZE;

    struct arm64_tlb_batch batch;
    arm64_tlb_batch_init(&batch, l.asid);

    /* EXECUTE on device memory is refused before any entry changes */
    if (flags & ARCH_MMU_FLAG_PERM_EXECUTE) {
        err = arm64_mmu_protect_pt(vaddr, vaddr - l.base, size, l.top_index_shift, l.page_size_shift,
                                   l.top, flags, &batch, true);
        if (err != NO_ERROR) {
            return err;
        }
    }

    err = arm64_mmu_protect_pt(vaddr, vaddr - l.base, size, l.top_index_shift, l.page_size_shift,
                               l.top, flags, &batch, false);
    DEBUG_ASSERT(err == NO_ERROR);

    arm64_tlb_batch_finish(&batch);
    return err;
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
