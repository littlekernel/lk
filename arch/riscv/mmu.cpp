/*
 * Copyright (c) 2020 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#if RISCV_MMU

#include "arch/riscv/mmu.h"

#include <assert.h>
#include <string.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/trace.h>
#include <arch/atomic.h>
#include <arch/ops.h>
#include <arch/mmu.h>
#include <arch/riscv.h>
#include <arch/riscv/csr.h>
#include <kernel/mp.h>
#include <kernel/vm.h>
#include <kernel/vm/asid.h>

#include "riscv_priv.h"

#define LOCAL_TRACE 0

#if __riscv_xlen == 32
#error "32 bit mmu not supported yet"
#endif

// global, generally referenced in start.S

// the one main kernel top page table, used by the kernel address space
// when no user space is active. bottom user space parts are empty.
riscv_pte_t kernel_pgtable[RISCV_MMU_PT_ENTRIES] __ALIGNED(PAGE_SIZE);
paddr_t kernel_pgtable_phys; // filled in by start.S

// trampoline top level page table is like the kernel page table but additionally
// holds an identity map of the bottom RISCV_MMU_PHYSMAP_SIZE bytes of ram.
// used at early bootup and when starting secondary processors.
riscv_pte_t trampoline_pgtable[RISCV_MMU_PT_ENTRIES] __ALIGNED(PAGE_SIZE);
paddr_t trampoline_pgtable_phys; // filled in by start.S

// pre-allocated second level page tables, one page sized table per kernel
// entry of the top level table. Every kernel top level entry is populated at
// boot, so user space top level tables stay in sync with the kernel's by
// simply taking a copy of those entries.
riscv_pte_t kernel_l2_pgtable[RISCV_MMU_KERNEL_PT_ENTRIES][RISCV_MMU_PT_ENTRIES] __ALIGNED(PAGE_SIZE);
static_assert(sizeof(kernel_l2_pgtable[0]) == PAGE_SIZE, "");
paddr_t kernel_l2_pgtable_phys; // filled in by start.S

// initial memory mappings. VM uses to construct mappings after the fact
struct mmu_initial_mapping mmu_initial_mappings[] = {
    // all of memory, mapped in start.S
    {
        .phys = 0,
        .virt = RISCV_MMU_PHYSMAP_BASE_VIRT,
        .size = RISCV_MMU_PHYSMAP_SIZE,
        .flags = 0,
        .name = "memory"
    },

    // null entry to terminate the list
    { }
};

namespace {

// local state
ulong riscv_asid_mask;
arch_aspace_t *kernel_aspace;

// User aspaces each get their own asid when the cpu implements all 16 bits of
// the satp field, so their TLB entries survive context switches. With fewer
// bits everything runs on asid 0 and each switch flushes the non-global entries
// on the local cpu. Decided once the asid width has been probed in
// riscv_early_mmu_init().
bool riscv_use_asids;
asid_allocator_t riscv_asid_allocator;

// the asid the kernel aspace runs under: ASID_KERNEL when asids are in use,
// else 0 like everyone else, which fits a cpu with no asid bits at all
uint16_t kernel_asid() {
    return riscv_use_asids ? ASID_KERNEL : 0;
}

// flush one asid on every cpu, from interrupt context on the remote ones
void flush_asid_task(void *arg) {
    riscv_tlb_flush_asid(*static_cast<const uint16_t *>(arg));
}

// given a va address and the level, compute the index in the current PT
constexpr uint vaddr_to_index(vaddr_t va, uint level) {
    // levels count down from PT_LEVELS - 1
    DEBUG_ASSERT(level < RISCV_MMU_PT_LEVELS);

    // canonicalize the address
    va &= RISCV_MMU_CANONICAL_MASK;

    uint index = ((va >> PAGE_SIZE_SHIFT) >> (level * RISCV_MMU_PT_SHIFT)) & (RISCV_MMU_PT_ENTRIES - 1);
    LTRACEF_LEVEL(3, "canonical va %#lx, level %u = index %#x\n", va, level, index);

    return index;
}

uintptr_t constexpr page_size_per_level(uint level) {
    // levels count down from PT_LEVELS - 1
    DEBUG_ASSERT(level < RISCV_MMU_PT_LEVELS);

    return 1UL << (PAGE_SIZE_SHIFT + level * RISCV_MMU_PT_SHIFT);
}

uintptr_t constexpr page_mask_per_level(uint level) {
    return page_size_per_level(level) - 1;
}

// compute the starting and stopping index of the kernel aspace
constexpr uint kernel_start_index = vaddr_to_index(KERNEL_ASPACE_BASE, RISCV_MMU_PT_LEVELS - 1);
constexpr uint kernel_end_index = vaddr_to_index(KERNEL_ASPACE_BASE + KERNEL_ASPACE_SIZE - 1UL, RISCV_MMU_PT_LEVELS - 1);

static_assert(kernel_end_index >= kernel_start_index && kernel_end_index < RISCV_MMU_PT_ENTRIES, "");
static_assert(kernel_end_index - kernel_start_index + 1 == RISCV_MMU_KERNEL_PT_ENTRIES, "");

// point this cpu's translation at a root table under an asid. Callers decide
// what, if anything, to flush around it.
void riscv_set_satp(uint asid, paddr_t pt) {
    ulong satp;

#if RISCV_MMU == 48
    satp = RISCV_SATP_MODE_SV48 << RISCV_SATP_MODE_SHIFT;
#elif RISCV_MMU == 39
    satp = RISCV_SATP_MODE_SV39 << RISCV_SATP_MODE_SHIFT;
#else
#error implement
#endif

    // make sure the asid is in range
    DEBUG_ASSERT_MSG((asid & riscv_asid_mask) == asid, "asid %#x mask %#lx\n", asid, riscv_asid_mask);
    satp |= (ulong)asid << RISCV_SATP_ASID_SHIFT;

    // make sure the page table is aligned
    DEBUG_ASSERT(IS_PAGE_ALIGNED(pt));
    satp |= pt >> PAGE_SIZE_SHIFT;

    riscv_csr_write(RISCV_CSR_SATP, satp);
}

// TLB shootdown of a range of pages in one aspace, run on every cpu. Kernel
// entries are global, so they are matched under every asid; user entries under
// the aspace's own. A run longer than this many pages, or one that freed a page
// table (whose cached intermediate entries a per page fence need not cover), is
// flushed as a whole instead.
constexpr size_t tlb_shootdown_max_pages = 16;

struct tlb_shootdown_args {
    vaddr_t base;
    size_t count;
    uint16_t asid;
    bool global;
    bool full;
};

void tlb_shootdown_task(void *arg) {
    const auto *args = static_cast<const tlb_shootdown_args *>(arg);

    if (args->full || args->count > tlb_shootdown_max_pages) {
        if (args->global) {
            riscv_tlb_flush_all();
        } else {
            riscv_tlb_flush_asid(args->asid);
        }
        return;
    }

    for (size_t i = 0; i < args->count; i++) {
        const vaddr_t va = args->base + i * PAGE_SIZE;
        if (args->global) {
            riscv_tlb_flush_va_all_asids(va);
        } else {
            riscv_tlb_flush_va_asid(va, args->asid);
        }
    }
}

// Make every cpu drop what it may have cached for [base, base + count pages) of
// this aspace. Interrupts must be enabled when other cpus are up; with only this
// cpu active, as during early boot, the fence simply runs here.
void riscv_tlb_shootdown(const arch_aspace_t *aspace, vaddr_t base, size_t count, bool tables_freed) {
    tlb_shootdown_args args = {
        .base = base,
        .count = count,
        .asid = aspace->asid,
        .global = (aspace->flags & ARCH_ASPACE_FLAG_KERNEL) != 0,
        .full = tables_freed,
    };

    LTRACEF("base %#lx count %zu asid %#x global %d full %d\n", base, count, args.asid, args.global, args.full);

    // the page table stores must be visible to the other harts before their fences run
    smp_mb();
    mp_sync_exec(MP_IPI_TARGET_ALL, 0, tlb_shootdown_task, &args);
}

#if LK_DEBUGLEVEL > 0
// how many more tables alloc_ptable may hand out, -1 for no limit; tests use
// it to drive the out of memory paths
int ptable_alloc_budget = -1;
#endif

volatile riscv_pte_t *alloc_ptable(arch_aspace_t *aspace, addr_t *pa) {
#if LK_DEBUGLEVEL > 0
    if (ptable_alloc_budget == 0) {
        return NULL;
    }
    if (ptable_alloc_budget > 0) {
        ptable_alloc_budget--;
    }
#endif

    // grab a page from the pmm
    vm_page_t *p = pmm_alloc_page();
    if (!p) {
        return NULL;
    }

    // get the physical and virtual mappings of the page
    *pa = vm_page_to_paddr(p);
    riscv_pte_t *pte = (riscv_pte_t *)paddr_to_kvaddr(*pa);

    // zero it out
    memset(pte, 0, PAGE_SIZE);

    smp_wmb();

    // add it to the aspace list
    list_add_head(&aspace->pt_list, &p->node);

    LTRACEF_LEVEL(3, "returning pa %#lx, va %p\n", *pa, pte);
    return pte;
}

riscv_pte_t mmu_flags_to_pte(uint flags) {
    riscv_pte_t pte = 0;

    pte |= (flags & ARCH_MMU_FLAG_PERM_USER) ? RISCV_PTE_U : 0;
    pte |= (flags & ARCH_MMU_FLAG_PERM_RO) ? RISCV_PTE_R : (RISCV_PTE_R | RISCV_PTE_W);
    pte |= (flags & ARCH_MMU_FLAG_PERM_NO_EXECUTE) ? 0 : RISCV_PTE_X;

    return pte;
}

uint pte_flags_to_mmu_flags(riscv_pte_t pte) {
    uint f = 0;
    if ((pte & (RISCV_PTE_R | RISCV_PTE_W)) == RISCV_PTE_R) {
        f |= ARCH_MMU_FLAG_PERM_RO;
    }
    f |= (pte & RISCV_PTE_X) ? 0 : ARCH_MMU_FLAG_PERM_NO_EXECUTE;
    f |= (pte & RISCV_PTE_U) ? ARCH_MMU_FLAG_PERM_USER : 0;
    return f;
}

} // namespace

// public api

// initialize per address space
status_t arch_mmu_init_aspace(arch_aspace_t *aspace, vaddr_t base, size_t size, uint flags) {
    LTRACEF("aspace %p, base %#lx, size %#zx, flags %#x\n", aspace, base, size, flags);

    DEBUG_ASSERT(aspace);

    // validate that the base + size is sane and doesn't wrap
    DEBUG_ASSERT(size > PAGE_SIZE);
    DEBUG_ASSERT(base + size - 1 > base);

    aspace->magic = RISCV_ASPACE_MAGIC;
    aspace->flags = flags;
    aspace->active_cpus = 0;
    list_initialize(&aspace->pt_list);
    if (flags & ARCH_ASPACE_FLAG_KERNEL) {
        // kernel aspace is special and should be constructed once
        DEBUG_ASSERT(base == KERNEL_ASPACE_BASE);
        DEBUG_ASSERT(size == KERNEL_ASPACE_SIZE);
        DEBUG_ASSERT(!kernel_aspace);

        aspace->base = base;
        aspace->size = size;
        aspace->pt_virt = kernel_pgtable;
        aspace->pt_phys = kernel_pgtable_phys;
        aspace->asid = kernel_asid();
        kernel_aspace = aspace;

        // the kernel aspace comes first, before any user aspace can ask for an asid
        asid_allocator_init(&riscv_asid_allocator, RISCV_SATP_ASID_MASK);

        // TODO: allocate and attach kernel page tables here instead of prealloced
    } else {
        // at the moment can only deal with user aspaces that perfectly
        // cover the predefined range
        DEBUG_ASSERT(base == USER_ASPACE_BASE);
        DEBUG_ASSERT(size == USER_ASPACE_SIZE);
        DEBUG_ASSERT(kernel_aspace);

        aspace->base = base;
        aspace->size = size;

        if (riscv_use_asids) {
            status_t err = asid_alloc(&riscv_asid_allocator, &aspace->asid);
            if (err < 0) {
                aspace->magic = 0;
                return err;
            }
        } else {
            aspace->asid = 0;
        }

        // allocate a top level page table
        aspace->pt_virt = alloc_ptable(aspace, &aspace->pt_phys);
        if (!aspace->pt_virt) {
            if (riscv_use_asids) {
                asid_free(&riscv_asid_allocator, aspace->asid);
            }
            aspace->magic = 0; // not a properly constructed aspace
            return ERR_NO_MEMORY;
        }

        // copy the top part of the top page table from the kernel's
        for (auto i = kernel_start_index; i <= kernel_end_index; i++) {
            aspace->pt_virt[i] = kernel_pgtable[i];
        }
        smp_wmb();
    }

    LTRACEF("pt phys %#lx, pt virt %p, asid %#x\n", aspace->pt_phys, aspace->pt_virt, aspace->asid);

    return NO_ERROR;
}

status_t arch_mmu_destroy_aspace(arch_aspace_t *aspace) {
    LTRACEF("aspace %p\n", aspace);

    DEBUG_ASSERT(aspace);
    DEBUG_ASSERT(aspace->magic == RISCV_ASPACE_MAGIC);

    if (aspace->flags & ARCH_ASPACE_FLAG_KERNEL) {
        panic("trying to destroy kernel aspace\n");
    } else {
        // no cpu may still be running on these tables, and every mapping must
        // already be gone: unmapping is what reclaims the lower tables, so only
        // the root should be left
        DEBUG_ASSERT(__atomic_load_n(&aspace->active_cpus, __ATOMIC_RELAXED) == 0);
        for (uint i = 0; i < kernel_start_index; i++) {
            DEBUG_ASSERT_MSG(aspace->pt_virt[i] == 0, "user root entry %u still %#lx\n", i, aspace->pt_virt[i]);
        }
        DEBUG_ASSERT(list_length(&aspace->pt_list) == 1);

        if (riscv_use_asids) {
            // Drop whatever every cpu still holds under this asid before it can
            // be handed to another aspace. Without per aspace asids nothing is
            // needed: every switch flushes asid 0 on the cpu doing it, and no cpu
            // has this aspace loaded any more.
            uint16_t asid = aspace->asid;
            mp_sync_exec(MP_IPI_TARGET_ALL, 0, flush_asid_task, &asid);
            asid_free(&riscv_asid_allocator, asid);
        }
        aspace->asid = 0;

        // mass free all of the page tables in the aspace
        DEBUG_ASSERT(!list_is_empty(&aspace->pt_list)); // should be at least one page
        LTRACEF("freeing %zu page tables\n", list_length(&aspace->pt_list));
        pmm_free(&aspace->pt_list);

        // free the top level page table
        aspace->pt_virt = nullptr;
        aspace->pt_phys = 0;
    }

    aspace->magic = 0;

    return NO_ERROR;
}

namespace {

enum class walk_action {
    HALT,        // stop the walk and return err
    NEXT,        // move past this entry
    COMMIT_NEXT, // store new_pte over this entry, then move past it
    ALLOC_PT,    // hang a fresh table off this empty entry and descend into it
};

// what a walk callback asks the walker to do with the entry it was shown
struct walk_cb_ret {
    static walk_cb_ret Halt(int err) { return { walk_action::HALT, err, 0 }; }
    static walk_cb_ret Next() { return { walk_action::NEXT, NO_ERROR, 0 }; }
    static walk_cb_ret CommitNext(riscv_pte_t pte) { return { walk_action::COMMIT_NEXT, NO_ERROR, pte }; }
    static walk_cb_ret AllocPT() { return { walk_action::ALLOC_PT, NO_ERROR, 0 }; }

    walk_action action;
    int err;             // HALT
    riscv_pte_t new_pte; // COMMIT_NEXT; 0 clears the entry
};

// the page table an entry lives in; tables are page sized and page aligned
inline volatile riscv_pte_t *table_of(volatile riscv_pte_t *ptep) {
    return reinterpret_cast<volatile riscv_pte_t *>(reinterpret_cast<uintptr_t>(ptep) & ~(PAGE_SIZE - 1));
}

bool table_is_empty(volatile riscv_pte_t *table) {
    for (uint i = 0; i < RISCV_MMU_PT_ENTRIES; i++) {
        if (table[i] != 0) {
            return false;
        }
    }
    return true;
}

// Called as a walk leaves the table at level. If the walk cleared entries in
// it (its dirty bit) and it is now empty, unlink it from the parent entry and
// collect its page on freed_tables, which marks the parent's table dirty in
// turn. Levels above highest are never unlinked, nor is anything without a
// list to collect on.
void leave_table(volatile riscv_pte_t *const ptep_at_level[], uint level, uint highest,
                 uint *dirty, struct list_node *freed_tables) {
    if (!(*dirty & (1u << level))) {
        return;
    }
    *dirty &= ~(1u << level);
    if (!freed_tables || level > highest) {
        return;
    }

    volatile riscv_pte_t *table = table_of(ptep_at_level[level]);
    if (!table_is_empty(table)) {
        return;
    }

    // the table's address comes from the link about to be cut
    volatile riscv_pte_t *link = ptep_at_level[level + 1];
    const paddr_t pa = RISCV_PTE_PPN(*link);
    LTRACEF_LEVEL(2, "level %u table %p (pa %#lx) empty, unlinking from %p\n", level, table, pa, link);
    *link = 0;
    *dirty |= 1u << (level + 1);

    vm_page_t *page = paddr_to_vm_page(pa);
    DEBUG_ASSERT(page);
    DEBUG_ASSERT(list_in_list(&page->node));
    list_delete(&page->node);
    list_add_tail(freed_tables, &page->node);
}

// One walk over [vaddr, vaddr + count pages) of an aspace, which must already
// have been range checked. Descends to the first entry that is not a table
// link, shows it to the callback as (vaddr, level, pte), and moves past
// whatever that entry covers, resuming in the current table rather than from
// the root. ptep_at_level[l] is the entry walked through at level l and is
// valid at and above the current level.
//
// Tables emptied along the way are unlinked once the walk leaves them, or
// when it ends, and collected on freed_tables for the caller to release after
// the TLB shootdown. Without a list nothing is reclaimed. The top level table
// is never unlinked, and neither is the kernel aspace's level below it: those
// are the static kernel_l2_pgtable pages every user root shares.
//
// Returns the callback's HALT error, ERR_NO_MEMORY if a table could not be
// allocated (the empty tables linked for it are unlinked again), or NO_ERROR
// once the range is exhausted. walked, when given, receives the number of
// pages stepped past on every exit path.
template <typename F>
int riscv_pt_walk(arch_aspace_t *aspace, vaddr_t vaddr, size_t count, F callback,
                  struct list_node *freed_tables, size_t *walked) {
    LTRACEF("vaddr %#lx count %zu\n", vaddr, count);

    DEBUG_ASSERT(aspace);

    constexpr uint top = RISCV_MMU_PT_LEVELS - 1;
    const uint highest = (aspace->flags & ARCH_ASPACE_FLAG_KERNEL) ? top - 2 : top - 1;

    volatile riscv_pte_t *ptep_at_level[RISCV_MMU_PT_LEVELS];
    uint dirty = 0; // bit per level: an entry in that level's table was cleared
    size_t remaining = count;
    int err = NO_ERROR;

    uint level = top;
    ptep_at_level[top] = aspace->pt_virt + vaddr_to_index(vaddr, top);

    while (remaining > 0) {
        volatile riscv_pte_t *ptep = ptep_at_level[level];
        const riscv_pte_t pte = *ptep;

        LTRACEF_LEVEL(2, "level %u, pte %p (%#lx) va %#lx remaining %zu\n",
                      level, ptep, pte, vaddr, remaining);

        if ((pte & RISCV_PTE_V) && !(pte & RISCV_PTE_PERM_MASK)) {
            // link to the next level table (RWX == 0)
            DEBUG_ASSERT(level > 0);
            const paddr_t ptp = RISCV_PTE_PPN(pte);
            volatile riscv_pte_t *table = (riscv_pte_t *)paddr_to_kvaddr(ptp);
            LTRACEF_LEVEL(2, "next level page table at %p, pa %#lx\n", table, ptp);

            level--;
            ptep_at_level[level] = table + vaddr_to_index(vaddr, level);
            continue;
        }

        // an empty entry or a leaf: the callback decides
        const walk_cb_ret ret = callback(vaddr, level, pte);
        if (ret.action == walk_action::HALT) {
            err = ret.err;
            break;
        }

        if (ret.action == walk_action::ALLOC_PT) {
            DEBUG_ASSERT(level > 0);
            DEBUG_ASSERT((pte & RISCV_PTE_V) == 0);

            paddr_t ptp;
            volatile riscv_pte_t *table = alloc_ptable(aspace, &ptp);
            if (!table) {
                // the table this entry is in may have been linked by this
                // walk and be empty; the exit path takes it out again
                dirty |= 1u << level;
                err = ERR_NO_MEMORY;
                break;
            }
            LTRACEF_LEVEL(2, "new page table %p, pa %#lx\n", table, ptp);

            // link it in. RWX == 0 marks a table
            *ptep = RISCV_PTE_PPN_TO_PTE(ptp) | RISCV_PTE_V;

            level--;
            ptep_at_level[level] = table + vaddr_to_index(vaddr, level);
            continue;
        }

        if (ret.action == walk_action::COMMIT_NEXT) {
            *ptep = ret.new_pte;
            if (ret.new_pte == 0) {
                dirty |= 1u << level;
            }
        }

        // step past what this entry covers, or to the end of the range
        const size_t block_pages = (size_t)1 << (level * RISCV_MMU_PT_SHIFT);
        const size_t page_in_block = (vaddr >> PAGE_SIZE_SHIFT) & (block_pages - 1);
        const size_t step = MIN(block_pages - page_in_block, remaining);
        vaddr += step * PAGE_SIZE; // wraps to 0 past the kernel's last page, with nothing remaining
        remaining -= step;
        if (remaining == 0) {
            break;
        }

        // vaddr is the next entry at this level. Leave every table whose
        // entries ran out and resume in the first that has more.
        while (vaddr_to_index(vaddr, level) == 0) {
            DEBUG_ASSERT(level < top); // the range check keeps the top level from wrapping
            leave_table(ptep_at_level, level, highest, &dirty, freed_tables);
            level++;
        }
        ptep_at_level[level] = table_of(ptep_at_level[level]) + vaddr_to_index(vaddr, level);
    }

    // the walk stopped inside a chain of tables; each that lost an entry gets its check
    if (dirty) {
        for (uint l = 0; l <= highest; l++) {
            leave_table(ptep_at_level, l, highest, &dirty, freed_tables);
        }
    }

    if (walked) {
        *walked = count - remaining;
    }
    return err;
}

} // namespace

#if LK_DEBUGLEVEL > 0
extern "C"
void riscv_mmu_set_ptable_alloc_budget(int count) {
    ptable_alloc_budget = count;
}
#endif

// routines to map/unmap/query mappings per address space
int arch_mmu_map(arch_aspace_t *aspace, const vaddr_t _vaddr, const paddr_t _paddr, const uint count, const uint flags) {
    LTRACEF("vaddr %#lx paddr %#lx count %u flags %#x\n", _vaddr, _paddr, count, flags);

    DEBUG_ASSERT(aspace);
    DEBUG_ASSERT(aspace->magic == RISCV_ASPACE_MAGIC);

    if (flags & ARCH_MMU_FLAG_NS) {
        return ERR_INVALID_ARGS;
    }

    if (!IS_PAGE_ALIGNED(_vaddr) || !IS_PAGE_ALIGNED(_paddr)) {
        return ERR_INVALID_ARGS;
    }

    if (!arch_mmu_range_in_aspace(aspace, _vaddr, count)) {
        return ERR_OUT_OF_RANGE;
    }

    if (count == 0) {
        return NO_ERROR;
    }

    const riscv_pte_t leaf_bits = mmu_flags_to_pte(flags) | RISCV_PTE_A | RISCV_PTE_D | RISCV_PTE_V |
                                  ((aspace->flags & ARCH_ASPACE_FLAG_KERNEL) ? RISCV_PTE_G : 0);

    // An empty entry gets a leaf when the address, its backing and what is
    // left of the range all line up with the size of page that level holds,
    // else a table to go further down. The top level never gets a leaf: user
    // roots copy the kernel's top level entries once, when they are created.
    auto map_cb = [_vaddr, _paddr, count, leaf_bits](vaddr_t vaddr, uint level, riscv_pte_t pte) -> walk_cb_ret {
        LTRACEF("vaddr %#lx level %u pte %#lx\n", vaddr, level, pte);

        if (pte & RISCV_PTE_V) {
            // a page or large page already covers this address
            DEBUG_ASSERT(pte & RISCV_PTE_PERM_MASK);
            TRACEF("mapping already exists at %#lx (level %u pte %#lx)\n", vaddr, level, pte);
            return walk_cb_ret::Halt(ERR_ALREADY_EXISTS);
        }

        const paddr_t paddr = _paddr + (vaddr - _vaddr);
        if (level > 0) {
            const size_t block = page_size_per_level(level);
            const size_t pages_left = count - (vaddr - _vaddr) / PAGE_SIZE;
            if (level == RISCV_MMU_PT_LEVELS - 1 || !IS_ALIGNED(vaddr, block) || !IS_ALIGNED(paddr, block) ||
                pages_left < block / PAGE_SIZE) {
                return walk_cb_ret::AllocPT();
            }
        }

        const riscv_pte_t new_pte = RISCV_PTE_PPN_TO_PTE(paddr) | leaf_bits;
        LTRACEF_LEVEL(2, "new leaf at level %u: pte %#lx\n", level, new_pte);
        return walk_cb_ret::CommitNext(new_pte);
    };

    struct list_node freed_tables = LIST_INITIAL_VALUE(freed_tables);
    size_t mapped = 0;
    int ret = riscv_pt_walk(aspace, _vaddr, count, map_cb, &freed_tables, &mapped);
    if (ret < 0) {
        // leave nothing behind: the pages that did go in, and any table that
        // was linked for one that did not
        if (mapped > 0) {
            arch_mmu_unmap(aspace, _vaddr, mapped);
        }
        if (!list_is_empty(&freed_tables)) {
            // a cpu may have cached the link to an orphaned table
            riscv_tlb_shootdown(aspace, _vaddr, 0, true);
            pmm_free(&freed_tables);
        }
        return ret;
    }
    DEBUG_ASSERT(list_is_empty(&freed_tables));

    // A cpu may hold a cached translation for a page that was invalid when it
    // last looked, so the new entries need a fence before they are usable.
    // Every cpu gets one: a stale cached entry elsewhere would fault, and there
    // is no fault handler to fence and retry. Svvptc cpus would not need this.
    riscv_tlb_shootdown(aspace, _vaddr, mapped, false);

    return NO_ERROR;
}

status_t arch_mmu_query(arch_aspace_t *aspace, const vaddr_t _vaddr, paddr_t *paddr, uint *flags) {
    LTRACEF("aspace %p, vaddr %#lx\n", aspace, _vaddr);

    DEBUG_ASSERT(aspace);
    DEBUG_ASSERT(aspace->magic == RISCV_ASPACE_MAGIC);

    if (!arch_mmu_range_in_aspace(aspace, _vaddr, 1)) {
        return ERR_OUT_OF_RANGE;
    }

    // a single entry walk: report the leaf covering the address, or its absence
    auto query_cb = [paddr, flags](vaddr_t vaddr, uint level, riscv_pte_t pte) -> walk_cb_ret {
        LTRACEF("vaddr %#lx level %u pte %#lx\n", vaddr, level, pte);

        if (!(pte & RISCV_PTE_V)) {
            return walk_cb_ret::Halt(ERR_NOT_FOUND);
        }
        DEBUG_ASSERT(pte & RISCV_PTE_PERM_MASK);

        if (paddr) {
            // the ppn plus the offset within a page of this level's size
            const paddr_t pa = RISCV_PTE_PPN(pte);
            const uintptr_t page_mask = page_mask_per_level(level);
            *paddr = pa | (vaddr & page_mask);
            LTRACEF_LEVEL(3, "raw pa %#lx, page_mask %#lx, final pa %#lx\n", pa, page_mask, *paddr);
        }
        if (flags) {
            *flags = pte_flags_to_mmu_flags(pte);
            LTRACEF_LEVEL(3, "computed flags %#x\n", *flags);
        }
        return walk_cb_ret::Halt(NO_ERROR);
    };

    return riscv_pt_walk(aspace, _vaddr, 1, query_cb, nullptr, nullptr);
}

int arch_mmu_unmap(arch_aspace_t *aspace, const vaddr_t _vaddr, const uint count) {
    LTRACEF("vaddr %#lx count %u\n", _vaddr, count);

    DEBUG_ASSERT(aspace);
    DEBUG_ASSERT(aspace->magic == RISCV_ASPACE_MAGIC);

    if (!IS_PAGE_ALIGNED(_vaddr)) {
        return ERR_INVALID_ARGS;
    }

    if (!arch_mmu_range_in_aspace(aspace, _vaddr, count)) {
        return ERR_OUT_OF_RANGE;
    }

    if (count == 0) {
        return NO_ERROR;
    }

    // Pages are cleared and holes stepped over. A large page goes as a whole
    // when the range covers it and is refused otherwise: splitting one needs
    // a table in its place, and what preceded it is already gone by then. The
    // span from the first to the last page cleared is what the shootdown must
    // cover.
    struct {
        vaddr_t first;
        size_t pages;
    } cleared = { 0, 0 };
    auto unmap_cb = [_vaddr, count, &cleared](vaddr_t vaddr, uint level, riscv_pte_t pte) -> walk_cb_ret {
        LTRACEF("vaddr %#lx level %u pte %#lx\n", vaddr, level, pte);

        if (!(pte & RISCV_PTE_V)) {
            return walk_cb_ret::Next();
        }
        DEBUG_ASSERT(pte & RISCV_PTE_PERM_MASK);

        const size_t block_pages = page_size_per_level(level) / PAGE_SIZE;
        if (level > 0) {
            const size_t pages_left = count - (vaddr - _vaddr) / PAGE_SIZE;
            if (!IS_ALIGNED(vaddr, page_size_per_level(level)) || pages_left < block_pages) {
                TRACEF("partial unmap of large page at %#lx (level %u) not supported\n", vaddr, level);
                return walk_cb_ret::Halt(ERR_NOT_SUPPORTED);
            }
        }

        if (cleared.pages == 0) {
            cleared.first = vaddr;
        }
        cleared.pages = (vaddr - cleared.first) / PAGE_SIZE + block_pages;
        return walk_cb_ret::CommitNext(0);
    };

    struct list_node freed_tables = LIST_INITIAL_VALUE(freed_tables);
    int ret = riscv_pt_walk(aspace, _vaddr, count, unmap_cb, &freed_tables, nullptr);

    // Shoot down the span that was cleared, then and only then hand back any
    // tables that emptied: a cpu may still be walking them until its fence
    // has run.
    const bool tables_freed = !list_is_empty(&freed_tables);
    if (cleared.pages > 0 || tables_freed) {
        riscv_tlb_shootdown(aspace, cleared.first, cleared.pages, tables_freed);
    }
    if (tables_freed) {
        LTRACEF("freeing %zu emptied page tables\n", list_length(&freed_tables));
        pmm_free(&freed_tables);
    }

    return ret;
}

// load a new user address space context.
// aspace argument NULL should load kernel-only context
void arch_mmu_context_switch(arch_aspace_t *old_aspace, arch_aspace_t *aspace) {
    LTRACEF("old aspace %p, aspace %p\n", old_aspace, aspace);

    DEBUG_ASSERT(!old_aspace || old_aspace->magic == RISCV_ASPACE_MAGIC);
    DEBUG_ASSERT(!aspace || aspace->magic == RISCV_ASPACE_MAGIC);

    if (aspace) {
        DEBUG_ASSERT((aspace->flags & ARCH_ASPACE_FLAG_KERNEL) == 0);
        riscv_set_satp(aspace->asid, aspace->pt_phys);
    } else {
        // kernel only: the kernel root has no user half
        riscv_set_satp(kernel_asid(), kernel_aspace->pt_phys);
    }

    // Every user aspace shares asid 0 when asids are not in use, so the
    // outgoing aspace's entries must not survive the switch. Global kernel
    // entries are untouched by the asid form.
    if (!riscv_use_asids && old_aspace != aspace) {
        riscv_tlb_flush_asid(0);
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

bool arch_mmu_supports_nx_mappings(void) { return true; }
bool arch_mmu_supports_ns_mappings(void) { return false; }
bool arch_mmu_supports_user_aspaces(void) { return true; }

extern "C"
void riscv_mmu_init_secondaries() {
    // switch to the proper kernel pgtable, with the trampoline parts unmapped.
    // The trampoline's identity map is global, so everything must go.
    riscv_set_satp(kernel_asid(), kernel_pgtable_phys);
    riscv_tlb_flush_all();

    // set the SUM bit so we can access user space directly (for now)
    riscv_csr_set(RISCV_CSR_XSTATUS, RISCV_CSR_XSTATUS_SUM);
}

// called once on the boot cpu during very early (single threaded) init
extern "C"
void riscv_early_mmu_init() {
    // figure out the number of support ASID bits by writing all 1s to
    // the asid field in satp and seeing which ones 'stick'
    auto satp_orig = riscv_csr_read(satp);
    auto satp = satp_orig | (RISCV_SATP_ASID_MASK << RISCV_SATP_ASID_SHIFT);
    riscv_csr_write(satp, satp);
    riscv_asid_mask = (riscv_csr_read(satp) >> RISCV_SATP_ASID_SHIFT) & RISCV_SATP_ASID_MASK;
    riscv_csr_write(satp, satp_orig);

    // only a full width field is worth allocating from
    riscv_use_asids = !RISCV_ASID_FALLBACK && (riscv_asid_mask == RISCV_SATP_ASID_MASK);

    // install zeroed page tables to the unused portions of the kernel page tables
    for (auto i = kernel_start_index; i <= kernel_end_index; i++) {
        if ((trampoline_pgtable[i] & RISCV_PTE_V) == 0) {
            trampoline_pgtable[i] = RISCV_PTE_PPN_TO_PTE(kernel_l2_pgtable_phys + (i - kernel_start_index) * PAGE_SIZE) | RISCV_PTE_V;
        }
    }

    // copy the top parts of the kernel page table from the trampoline page table
    for (auto i = kernel_start_index; i <= kernel_end_index; i++) {
        kernel_pgtable[i] = trampoline_pgtable[i];
    }
    smp_wmb();

    // switch to the new kernel pagetable
    riscv_mmu_init_secondaries();
}

// called a bit later once on the boot cpu
extern "C"
void riscv_mmu_init() {
    dprintf(INFO, "RISCV: MMU ASID mask %#lx, %s\n", riscv_asid_mask,
            riscv_use_asids ? "one asid per user aspace" : "user aspaces share asid 0 and flush on switch");
}

#endif
