/*
 * Copyright (c) 2009 Corey Tabaka
 * Copyright (c) 2015 Intel Corporation
 * Copyright (c) 2016 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <arch.h>
#include <arch/arch_ops.h>
#include <arch/interrupts.h>
#include <arch/mmu.h>
#include <arch/x86.h>
#include <arch/x86/feature.h>
#include <arch/x86/mmu.h>
#include <assert.h>
#include <kernel/vm.h>
#include <lk/compiler.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/trace.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

// TODO:
// - synchronization of top level page tables for user space aspaces

#define LOCAL_TRACE          0
#define TRACE_CONTEXT_SWITCH 0

/* top level kernel page tables, initialized in start.S */
#if X86_LEGACY
/* enough page tables to map 16MB ram using 4K pages */
map_addr_t kernel_pt[NO_OF_PT_ENTRIES][4] __ALIGNED(PAGE_SIZE);
#endif
map_addr_t kernel_pd[NO_OF_PT_ENTRIES] __ALIGNED(PAGE_SIZE);
paddr_t kernel_pd_phys;

/* Levels count down from PD_L to PT_L, whose entries map 4K pages. */
static inline uint x86_index(vaddr_t vaddr, int level) {
    return (vaddr >> (PT_SHIFT + (level - PT_L) * ADDR_OFFSET)) & (NO_OF_PT_ENTRIES - 1);
}

/* the span an entry at level covers */
static inline vaddr_t x86_entry_span(int level) {
    return 1UL << (PT_SHIFT + (level - PT_L) * ADDR_OFFSET);
}

/* a page at PT_L, or a 4MB page (PS) at PD_L */
static inline bool x86_is_leaf(map_addr_t entry, int level) {
    return level == PT_L || (entry & X86_MMU_PG_PS);
}

/* the frame a leaf maps, or the table a directory entry points to */
static inline paddr_t x86_entry_paddr(map_addr_t entry, int level) {
    return entry & ~(map_addr_t)(x86_entry_span(level) - 1);
}

static inline map_addr_t *x86_next_table(map_addr_t entry) {
    return paddr_to_kvaddr(x86_entry_paddr(entry, PT_L));
}

/**
 * @brief Returning the x86 arch flags from generic mmu flags
 */
static arch_flags_t get_x86_arch_flags(uint flags) {
    arch_flags_t arch_flags = 0;

    if (flags & ARCH_MMU_FLAG_PERM_WRITE) {
        arch_flags |= X86_MMU_PG_RW;
    }

    if (flags & ARCH_MMU_FLAG_PERM_USER) {
        arch_flags |= X86_MMU_PG_U;
    }

    /* PAT indexes 2 and 3 of X86_PAT_DEFAULT */
    switch (flags & ARCH_MMU_FLAG_CACHE_MASK) {
        case ARCH_MMU_FLAG_UNCACHED:
            arch_flags |= X86_MMU_CACHE_DISABLE;
            break;
        case ARCH_MMU_FLAG_UNCACHED_DEVICE:
            arch_flags |= X86_MMU_CACHE_DISABLE | X86_MMU_PG_PWT;
            break;
    }

    return arch_flags;
}

/**
 * @brief Returning the generic mmu flags from x86 arch flags
 */
static uint get_arch_mmu_flags(arch_flags_t flags) {
    /* without NX every page can be executed */
    uint mmu_flags = ARCH_MMU_FLAG_PERM_READ | ARCH_MMU_FLAG_PERM_EXECUTE;

    if (flags & X86_MMU_PG_RW) {
        mmu_flags |= ARCH_MMU_FLAG_PERM_WRITE;
    }

    if (flags & X86_MMU_PG_U) {
        mmu_flags |= ARCH_MMU_FLAG_PERM_USER;
    }

    if (flags & X86_MMU_CACHE_DISABLE) {
        mmu_flags |= (flags & X86_MMU_PG_PWT) ? ARCH_MMU_FLAG_UNCACHED_DEVICE : ARCH_MMU_FLAG_UNCACHED;
    }

    return mmu_flags;
}

/* The entry that ends the walk for vaddr from the directory: a present leaf, or
 * the first entry that is not present. *level is the level of its table. */
static map_addr_t *x86_mmu_lookup(map_addr_t *pd, vaddr_t vaddr, int *level) {
    map_addr_t *entry = &pd[x86_index(vaddr, PD_L)];
    if (!(*entry & X86_MMU_PG_P) || (*entry & X86_MMU_PG_PS)) {
        *level = PD_L;
        return entry;
    }
    *level = PT_L;
    return &x86_next_table(*entry)[x86_index(vaddr, PT_L)];
}

/**
 * @brief Allocating a new page table
 */
static map_addr_t *alloc_page_table(paddr_t *pa_out) {
    if (!vm_ptable_budget_take()) {
        return NULL;
    }

    vm_page_t *page = pmm_alloc_page();
    if (!page) {
        return NULL;
    }

    paddr_t pa = vm_page_to_paddr(page);
    DEBUG_ASSERT(pa != (paddr_t)-1);

    map_addr_t *page_ptr = paddr_to_kvaddr(pa);
    DEBUG_ASSERT(page_ptr);

    memset(page_ptr, 0, PAGE_SIZE);
    /* the zeroes are stored before the entry that links the table; x86 keeps
     * stores in order, so only the compiler has to be told */
    __asm__ volatile("" ::: "memory");
    if (pa_out) {
        *pa_out = pa;
    }

    return page_ptr;
}

/* Map one page, linking a page table if it needs one. ERR_ALREADY_EXISTS if a
 * page or a 4MB page is in the way, ERR_NO_MEMORY if the table cannot be
 * allocated. A directory entry grants everything and leaves access to the
 * leaf, and in a user aspace that includes user access. A table linked under
 * a kernel directory entry reaches the user directories created from then on,
 * not the ones that already copied the kernel half. */
static status_t x86_mmu_map_page(map_addr_t *pd, vaddr_t vaddr, map_addr_t leaf, bool user) {
    map_addr_t *entry = &pd[x86_index(vaddr, PD_L)];
    map_addr_t e = *entry;
    if (!(e & X86_MMU_PG_P)) {
        paddr_t pa;
        if (!alloc_page_table(&pa)) {
            return ERR_NO_MEMORY;
        }
        e = pa | X86_MMU_PG_P | X86_MMU_PG_RW | (user ? X86_MMU_PG_U : 0);
        *entry = e;
        LTRACEF_LEVEL(2, "new page table for %#lx: %#x\n", vaddr, e);
    } else if (e & X86_MMU_PG_PS) {
        return ERR_ALREADY_EXISTS;
    }

    map_addr_t *pte = &x86_next_table(e)[x86_index(vaddr, PT_L)];
    if (*pte & X86_MMU_PG_P) {
        return ERR_ALREADY_EXISTS;
    }
    *pte = leaf;
    LTRACEF_LEVEL(2, "pte for %#lx: %#x\n", vaddr, leaf);
    return NO_ERROR;
}

/* Replace *entry with desired if it still holds *expected, else load what it
 * holds into *expected. The cpu sets A and D in entries on its own, so a plain
 * read and write could lose one. */
static bool x86_entry_replace(map_addr_t *entry, map_addr_t *expected, map_addr_t desired) {
#if X86_LEGACY
    /* A 386 has no cmpxchg, and no second cpu: with interrupts off nothing
     * touches the page between the read and the write. */
    struct arch_interrupt_saved_state state = arch_interrupt_save();
    const bool same = *entry == *expected;
    if (same) {
        *entry = desired;
    } else {
        *expected = *entry;
    }
    arch_interrupt_restore(state);
    return same;
#else
    return __atomic_compare_exchange_n(entry, expected, desired, false, __ATOMIC_RELAXED, __ATOMIC_RELAXED);
#endif
}

/* the pages from the first entry changed to the end of the last, for the shootdown */
struct x86_span {
    vaddr_t first;
    size_t pages;
};

static void x86_span_add(struct x86_span *span, vaddr_t vaddr, size_t pages) {
    if (span->pages == 0) {
        span->first = vaddr;
    }
    span->pages = (vaddr - span->first) / PAGE_SIZE + pages;
}

static bool x86_table_is_empty(const map_addr_t *table) {
    for (uint i = 0; i < NO_OF_PT_ENTRIES; i++) {
        if (table[i] & X86_MMU_PG_P) {
            return false;
        }
    }
    return true;
}

/* Clear the entries covering [vaddr, last] (last inclusive) in table at level,
 * stepping over what is not present and adding what it clears to span. A page
 * table that ends up empty is unlinked and put on freed, unless it hangs off a
 * kernel directory entry: every user directory holds a copy of those, so such
 * a table stays for good, the static ones of a legacy build included. The
 * caller has made sure the range touches no 4MB page. */
static void x86_mmu_unmap_table(map_addr_t *table, int level, vaddr_t vaddr, vaddr_t last,
                                bool kernel, struct list_node *freed, struct x86_span *span) {
    const vaddr_t entry_span = x86_entry_span(level);
    for (;;) {
        map_addr_t *entry = &table[x86_index(vaddr, level)];
        const map_addr_t e = *entry;
        const vaddr_t entry_base = ROUNDDOWN(vaddr, entry_span);
        const vaddr_t entry_last = MIN(entry_base + (entry_span - 1), last);

        if (!(e & X86_MMU_PG_P)) {
            /* a hole: step over what the entry covers */
        } else if (x86_is_leaf(e, level)) {
            if (level > PT_L) {
                DEBUG_ASSERT_MSG(false, "unmap of a 4MB page at %#lx\n", entry_base);
            } else {
                *entry = 0;
                x86_span_add(span, entry_base, 1);
            }
        } else {
            map_addr_t *next = x86_next_table(e);
            x86_mmu_unmap_table(next, level - 1, vaddr, entry_last, kernel, freed, span);

            if (!kernel && x86_table_is_empty(next)) {
                const paddr_t pa = x86_entry_paddr(e, PT_L);
                LTRACEF_LEVEL(2, "unlinking empty page table %#lx\n", pa);
                *entry = 0;
                vm_page_t *page = paddr_to_vm_page(pa);
                DEBUG_ASSERT(page);
                list_add_tail(freed, &page->node);
            }
        }

        if (entry_last == last) {
            return;
        }
        vaddr = entry_last + 1;
    }
}

/* Unmap [vaddr, vaddr + count pages), then make sure no cpu uses what was
 * there or walks a table that was unlinked before handing those back. */
static void x86_mmu_unmap_range(arch_aspace_t *aspace, vaddr_t vaddr, uint count) {
    struct list_node freed = LIST_INITIAL_VALUE(freed);
    struct x86_span span = { 0, 0 };
    const bool kernel = aspace->flags & ARCH_ASPACE_FLAG_KERNEL;

    x86_mmu_unmap_table(aspace->cr3, PD_L, vaddr, vaddr + ((vaddr_t)count * PAGE_SIZE - 1),
                        kernel, &freed, &span);

    /* Any invlpg drops every cached directory entry, so the unlinked tables
     * are covered even if they held nothing to clear. */
    if (span.pages > 0) {
        x86_tlb_shootdown(span.first, span.pages);
    } else if (!list_is_empty(&freed)) {
        x86_tlb_shootdown(vaddr, 1);
    }
    pmm_free(&freed);
}

/* Whether unmap or protect may change [vaddr, vaddr + count pages), decided
 * before either touches anything. The kernel's 4MB physmap entries are fixed:
 * every user directory holds a copy of them, and map makes no large pages
 * anywhere else. */
static status_t x86_mmu_check_range(arch_aspace_t *aspace, vaddr_t vaddr, uint count) {
#if !X86_LEGACY
    if ((aspace->flags & ARCH_ASPACE_FLAG_KERNEL) && vaddr - KERNEL_ASPACE_BASE < PHYSMAP_SIZE) {
        TRACEF("range at %#lx touches the physmap\n", vaddr);
        return ERR_NOT_SUPPORTED;
    }
#endif
    return NO_ERROR;
}

/* Give every page covering [vaddr, last] in table the RW bit in bits, adding
 * the ones that change to span. With check_only nothing is written, and the
 * walk only reports ERR_INVALID_ARGS on an uncached page when execute is asked
 * for. */
static status_t x86_mmu_protect_table(map_addr_t *table, int level, vaddr_t vaddr, vaddr_t last,
                                      map_addr_t bits, bool execute, bool check_only,
                                      struct x86_span *span) {
    const vaddr_t entry_span = x86_entry_span(level);
    for (;;) {
        map_addr_t *entry = &table[x86_index(vaddr, level)];
        map_addr_t e = *entry;
        const vaddr_t entry_base = ROUNDDOWN(vaddr, entry_span);
        const vaddr_t entry_last = MIN(entry_base + (entry_span - 1), last);

        if (!(e & X86_MMU_PG_P)) {
            /* a hole */
        } else if (x86_is_leaf(e, level)) {
            DEBUG_ASSERT(level == PT_L);
            if (check_only) {
                if (execute && (e & X86_MMU_CACHE_DISABLE)) {
                    LTRACEF("execute asked for on uncached memory at %#lx\n", vaddr);
                    return ERR_INVALID_ARGS;
                }
            } else {
                for (;;) {
                    const map_addr_t new_e = (e & ~X86_MMU_PG_RW) | bits;
                    if (new_e == e) {
                        break;
                    }
                    if (x86_entry_replace(entry, &e, new_e)) {
                        x86_span_add(span, entry_base, 1);
                        break;
                    }
                }
            }
        } else {
            status_t err = x86_mmu_protect_table(x86_next_table(e), level - 1, vaddr, entry_last, bits,
                                                 execute, check_only, span);
            if (err != NO_ERROR) {
                return err;
            }
        }

        if (entry_last == last) {
            return NO_ERROR;
        }
        vaddr = entry_last + 1;
    }
}

status_t arch_mmu_unmap(arch_aspace_t *const aspace, const vaddr_t vaddr, const uint count) {
    LTRACEF("aspace %p, vaddr %#lx, count %u\n", aspace, vaddr, count);

    DEBUG_ASSERT(aspace);

    if (!IS_ALIGNED(vaddr, PAGE_SIZE)) {
        return ERR_INVALID_ARGS;
    }

    if (!arch_mmu_range_in_aspace(aspace, vaddr, count)) {
        return ERR_OUT_OF_RANGE;
    }

    if (count == 0) {
        return NO_ERROR;
    }

    status_t err = x86_mmu_check_range(aspace, vaddr, count);
    if (err != NO_ERROR) {
        return err;
    }

    x86_mmu_unmap_range(aspace, vaddr, count);
    return NO_ERROR;
}

status_t arch_mmu_protect(arch_aspace_t *const aspace, const vaddr_t vaddr, const uint count,
                          const uint flags) {
    LTRACEF("aspace %p, vaddr %#lx, count %u, flags %#x\n", aspace, vaddr, count, flags);

    DEBUG_ASSERT(aspace);

    if (!IS_ALIGNED(vaddr, PAGE_SIZE) || !arch_mmu_protect_flags_valid(flags)) {
        return ERR_INVALID_ARGS;
    }

    if (!arch_mmu_range_in_aspace(aspace, vaddr, count)) {
        return ERR_OUT_OF_RANGE;
    }

    if (count == 0) {
        return NO_ERROR;
    }

    status_t err = x86_mmu_check_range(aspace, vaddr, count);
    if (err != NO_ERROR) {
        return err;
    }

    /* without NX, EXECUTE changes nothing but is still refused on uncached memory */
    const bool execute = flags & ARCH_MMU_FLAG_PERM_EXECUTE;
    const map_addr_t bits = get_x86_arch_flags(flags) & X86_MMU_PG_RW;
    const vaddr_t last = vaddr + ((vaddr_t)count * PAGE_SIZE - 1);
    struct x86_span span = { 0, 0 };

    if (execute) {
        err = x86_mmu_protect_table(aspace->cr3, PD_L, vaddr, last, bits, true, true, &span);
        if (err != NO_ERROR) {
            return err;
        }
    }

    err = x86_mmu_protect_table(aspace->cr3, PD_L, vaddr, last, bits, execute, false, &span);
    DEBUG_ASSERT(err == NO_ERROR);

    /* upgrades included: a cpu may otherwise fault on a stale entry */
    if (span.pages > 0) {
        x86_tlb_shootdown(span.first, span.pages);
    }
    return err;
}

status_t arch_mmu_query(arch_aspace_t *const aspace, const vaddr_t vaddr, paddr_t *const paddr,
                        uint *const flags) {
    LTRACEF("aspace %p, vaddr 0x%lx, paddr %p, flags %p\n", aspace, vaddr, paddr, flags);

    DEBUG_ASSERT(aspace);

    if (!arch_mmu_range_in_aspace(aspace, vaddr, 1)) {
        return ERR_OUT_OF_RANGE;
    }

    int level;
    const map_addr_t e = *x86_mmu_lookup(aspace->cr3, vaddr, &level);
    if (!(e & X86_MMU_PG_P)) {
        return ERR_NOT_FOUND;
    }

    if (paddr) {
        *paddr = x86_entry_paddr(e, level) | (vaddr & (x86_entry_span(level) - 1));
    }
    if (flags) {
        *flags = get_arch_mmu_flags(e);
    }
    LTRACEF("entry %#x at level %d\n", e, level);

    return NO_ERROR;
}

status_t arch_mmu_map(arch_aspace_t *const aspace, const vaddr_t vaddr, const paddr_t paddr,
                      const uint count, const uint flags) {
    LTRACEF("aspace %p, vaddr %#lx, paddr %#lx, count %u, flags %#x\n", aspace, vaddr, paddr, count,
            flags);

    DEBUG_ASSERT(aspace);

    /* Leaving out EXECUTE is accepted, but without NX the page stays executable. */
    if (!IS_ALIGNED(vaddr, PAGE_SIZE) || !IS_ALIGNED(paddr, PAGE_SIZE) ||
        !arch_mmu_paddr_range_valid(paddr, count, 32) || !arch_mmu_flags_valid(flags)) {
        return ERR_INVALID_ARGS;
    }

    if (!arch_mmu_range_in_aspace(aspace, vaddr, count)) {
        return ERR_OUT_OF_RANGE;
    }

    if (count == 0) {
        return NO_ERROR;
    }

    /* A leaf is global exactly when it belongs to the kernel aspace: a global
     * entry survives the cr3 load of a context switch, so one in a user aspace
     * would leak into the next. Before PGE the bit was reserved. */
    const bool kernel = aspace->flags & ARCH_ASPACE_FLAG_KERNEL;
    const map_addr_t global = (kernel && x86_feature_test(X86_FEATURE_PGE)) ? X86_MMU_PG_G : 0;
    const map_addr_t leaf_flags = get_x86_arch_flags(flags) | X86_MMU_PG_P | global;

    for (uint i = 0; i < count; i++) {
        const vaddr_t offset = (vaddr_t)i * PAGE_SIZE;
        status_t err = x86_mmu_map_page(aspace->cr3, vaddr + offset, (paddr + offset) | leaf_flags, !kernel);
        if (err != NO_ERROR) {
            /* Take back what went in: the pages before a collision, or through
             * the page a table was missing for, so an empty table linked for it
             * is reclaimed as well where that is allowed. */
            const uint done = i + (err == ERR_NO_MEMORY ? 1 : 0);
            if (done > 0) {
                x86_mmu_unmap_range(aspace, vaddr, done);
            }
            return err;
        }
    }

    /* A present bit going from 0 to 1 needs no invalidate: no cpu caches an
     * entry that is not present, at either level. */
    return NO_ERROR;
}

bool arch_mmu_supports_nx_mappings(void) {
    return false;
}
bool arch_mmu_supports_ns_mappings(void) {
    return false;
}
bool arch_mmu_supports_user_aspaces(void) {
    return true;
}

/* called once per cpu as it is brought up */
void x86_mmu_early_init_percpu(void) {
    /* Set WP bit in CR0 */
    uint32_t cr0 = x86_get_cr0();
    cr0 |= X86_CR0_WP;
    x86_set_cr0(cr0);

    /* Set some mmu control bits in CR4 */
    uint32_t bits = 0;
    bits |= x86_feature_test(X86_FEATURE_PGE) ? X86_CR4_PGE : 0;
    bits |= x86_feature_test(X86_FEATURE_PSE) ? X86_CR4_PSE : 0;
    bits |= x86_feature_test(X86_FEATURE_SMEP) ? X86_CR4_SMEP : 0;
    /* for now, we dont support SMAP due to some tests that assume they can access user space */
    // bits |= x86_feature_test(X86_FEATURE_SMAP) ? X86_CR4_SMAP : 0;
    if (bits) {
        /* don't touch cr4 unless we need to, early cpus will fault if its not implemented */
        uint32_t cr4 = x86_get_cr4();
        cr4 |= bits;
        x86_set_cr4(cr4);
    }

    /* The encoding of the cache policies relies on the default PAT. Nothing is
     * mapped through an entry other than index 0 (write back in both) yet, so
     * it can be rewritten in place. */
    if (x86_feature_test(X86_FEATURE_PAT) && read_msr(X86_MSR_IA32_PAT) != X86_PAT_DEFAULT) {
        write_msr(X86_MSR_IA32_PAT, X86_PAT_DEFAULT);
    }
}

void x86_mmu_early_init(void) {
    /* unmap the lower identity mapping */
    for (uint i = 0; i < (1024 * 1024 * 1024) / (4 * 1024 * 1024); i++) {
        kernel_pd[i] = 0;
    }

    /* tlb flush */
    x86_set_cr3(x86_get_cr3());
}

void x86_mmu_init(void) {}

/*
 * x86 does not support multiple address spaces at the moment, so fail if these apis
 * are used for it.
 */
status_t arch_mmu_init_aspace(arch_aspace_t *const aspace, const vaddr_t base, const size_t size,
                              const uint flags) {
    DEBUG_ASSERT(aspace);

    LTRACEF("aspace %p, base %#lx, size %#zx, flags %#x\n", aspace, base, size, flags);

    /* validate that the base + size is sane and doesn't wrap */
    DEBUG_ASSERT(size > PAGE_SIZE);
    DEBUG_ASSERT(base + size - 1 > base);

    aspace->flags = flags;
    if (flags & ARCH_ASPACE_FLAG_KERNEL) {
        /* at the moment we can only deal with address spaces as globally defined */
        DEBUG_ASSERT(base == KERNEL_ASPACE_BASE);
        DEBUG_ASSERT(size == KERNEL_ASPACE_SIZE);

        aspace->base = base;
        aspace->size = size;
        aspace->cr3 = kernel_pd;
        aspace->cr3_phys = kernel_pd_phys;
    } else {
        DEBUG_ASSERT(base == USER_ASPACE_BASE);
        DEBUG_ASSERT(size == USER_ASPACE_SIZE);

        aspace->base = base;
        aspace->size = size;

        map_addr_t *va = pmm_alloc_kpages(1, NULL);
        if (!va) {
            return ERR_NO_MEMORY;
        }

        aspace->cr3 = va;
        aspace->cr3_phys = vaddr_to_paddr(aspace->cr3);

        /* copy the top entries from the kernel top table */
        memcpy(aspace->cr3 + NO_OF_PT_ENTRIES / 2, kernel_pd + NO_OF_PT_ENTRIES / 2, PAGE_SIZE / 2);

        /* zero out the rest */
        memset(aspace->cr3, 0, PAGE_SIZE / 2);
    }

    return NO_ERROR;
}

status_t arch_mmu_destroy_aspace(arch_aspace_t *const aspace) {
    if (aspace->flags & ARCH_ASPACE_FLAG_KERNEL) {
        // can't destroy the kernel aspace
        panic("attempt to destroy kernel aspace\n");
        return ERR_NOT_ALLOWED;
    }

    // Every mapping must already be gone: unmapping is what frees the page
    // tables, so only the directory is left. Whether a cpu still has the
    // aspace loaded cannot be checked here, since x86 does not count them.
    for (uint i = 0; i < NO_OF_PT_ENTRIES / 2; i++) {
        DEBUG_ASSERT_MSG(aspace->cr3[i] == 0, "user directory entry %u still %#x\n", i, aspace->cr3[i]);
    }

    // free the page table
    pmm_free_kpages(aspace->cr3, 1);

    return NO_ERROR;
}

void arch_mmu_context_switch(arch_aspace_t *old_aspace, arch_aspace_t *const aspace) {
    if (TRACE_CONTEXT_SWITCH) {
        TRACEF("aspace %p\n", aspace);
    }

    uint64_t cr3;
    if (aspace) {
        DEBUG_ASSERT((aspace->flags & ARCH_ASPACE_FLAG_KERNEL) == 0);

        cr3 = aspace->cr3_phys;
    } else {
        cr3 = kernel_pd_phys;
    }
    if (TRACE_CONTEXT_SWITCH) {
        TRACEF("cr3 %#llx\n", cr3);
    }

    x86_set_cr3(cr3);
}
