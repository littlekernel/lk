/*
 * Copyright (c) 2014-2016 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#if ARCH_HAS_MMU

#include <arch.h>
#include <arch/defines.h>
#include <lk/compiler.h>
#include <stdbool.h>
#include <sys/types.h>

/* to bring in definition of arch_aspace */
#include <arch/aspace.h>

__BEGIN_CDECLS

/* flags to pass to the arch_mmu_map and arch_mmu_query routines */
#define ARCH_MMU_FLAG_CACHED            (0U<<0)
#define ARCH_MMU_FLAG_UNCACHED          (1U<<0)
#define ARCH_MMU_FLAG_UNCACHED_DEVICE   (2U<<0) /* only exists on some arches, otherwise UNCACHED */
#define ARCH_MMU_FLAG_CACHE_MASK        (3U<<0)

/* Permissions are granted, not taken away: a mapping has exactly the ones
 * named. READ is required, so the valid sets are R, RW, RX and RWX. Leaving
 * out EXECUTE is only enforced where arch_mmu_supports_nx_mappings() says so. */
#define ARCH_MMU_FLAG_PERM_USER         (1U<<2)
#define ARCH_MMU_FLAG_PERM_READ         (1U<<3)
#define ARCH_MMU_FLAG_PERM_WRITE        (1U<<4)
#define ARCH_MMU_FLAG_PERM_EXECUTE      (1U<<5)
#define ARCH_MMU_FLAG_PERM_RWX_MASK     (ARCH_MMU_FLAG_PERM_READ | ARCH_MMU_FLAG_PERM_WRITE | \
                                         ARCH_MMU_FLAG_PERM_EXECUTE)
#define ARCH_MMU_FLAG_NS                (1U<<6) /* supported on some arches */
#define ARCH_MMU_FLAG_INVALID           (1U<<7) /* indicates that flags are not specified */

/* arch level query of some features at the mapping/query level */
bool arch_mmu_supports_nx_mappings(void);
bool arch_mmu_supports_ns_mappings(void);
bool arch_mmu_supports_user_aspaces(void);

/* forward declare the per-address space arch-specific context object */
typedef struct arch_aspace arch_aspace_t;

#define ARCH_ASPACE_FLAG_KERNEL         (1U<<0)

/* Whether flags are a valid request for arch_mmu_map(): READ is present, the
 * cache policy is defined and not an uncached one with EXECUTE (arm forbids
 * fetching from Device memory), NS only where the arch supports it, and no
 * INVALID or undefined bit. */
static inline bool arch_mmu_flags_valid(uint flags) {
    const uint defined = ARCH_MMU_FLAG_CACHE_MASK | ARCH_MMU_FLAG_PERM_USER |
                         ARCH_MMU_FLAG_PERM_RWX_MASK | ARCH_MMU_FLAG_NS;
    if ((flags & ~defined) || !(flags & ARCH_MMU_FLAG_PERM_READ)) {
        return false;
    }
    switch (flags & ARCH_MMU_FLAG_CACHE_MASK) {
        case ARCH_MMU_FLAG_CACHED:
            break;
        case ARCH_MMU_FLAG_UNCACHED:
        case ARCH_MMU_FLAG_UNCACHED_DEVICE:
            if (flags & ARCH_MMU_FLAG_PERM_EXECUTE) {
                return false;
            }
            break;
        default:
            return false;
    }
    return !(flags & ARCH_MMU_FLAG_NS) || arch_mmu_supports_ns_mappings();
}

/* Whether flags are a valid permission set for arch_mmu_protect(): R/W/X bits
 * only, READ among them. */
static inline bool arch_mmu_protect_flags_valid(uint flags) {
    return !(flags & ~ARCH_MMU_FLAG_PERM_RWX_MASK) && (flags & ARCH_MMU_FLAG_PERM_READ);
}

/* Whether [paddr, paddr + count pages) neither wraps nor runs past a physical
 * address width of bits. */
static inline bool arch_mmu_paddr_range_valid(paddr_t paddr, uint count, uint bits) {
    uint64_t last = (paddr_t)-1;
    if (bits < 64 && last > ((uint64_t)1 << bits) - 1) {
        last = ((uint64_t)1 << bits) - 1;
    }
    const uint64_t size = (uint64_t)count << PAGE_SIZE_SHIFT;
    return paddr <= last && (size == 0 || size - 1 <= last - paddr);
}

/* [vaddr, vaddr + count pages) lies inside the aspace. Computed so an aspace
 * that runs to the top of the address space does not overflow. */
static inline bool arch_mmu_range_in_aspace(const arch_aspace_t *aspace, vaddr_t vaddr, uint count) {
    if (vaddr < aspace->base || vaddr > aspace->base + aspace->size - 1) {
        return false;
    }
    const size_t pages_left = (aspace->base + aspace->size - 1 - vaddr) / PAGE_SIZE + 1;
    return count <= pages_left;
}

/* Initialize per address space. Destroy frees the top level table; the aspace
 * must have no mappings left and be loaded on no cpu. Destroy asserts the
 * first; x86 and arm32 cannot check the second. */
status_t arch_mmu_init_aspace(arch_aspace_t *aspace, vaddr_t base, size_t size, uint flags) __NONNULL((1));
status_t arch_mmu_destroy_aspace(arch_aspace_t *aspace) __NONNULL((1));

/*
 * Map, unmap, query and protect, the same on every arch.
 *
 * Arguments are checked first, count == 0 included; only then does a count
 * of 0 return NO_ERROR, having touched nothing.
 *  - vaddr must be page aligned (query takes any vaddr). For map so must paddr
 *    be, and [paddr, paddr + count pages) may neither wrap nor run past the
 *    arch's physical address width. Else ERR_INVALID_ARGS.
 *  - [vaddr, vaddr + count pages) must lie inside the aspace, else
 *    ERR_OUT_OF_RANGE.
 *  - Map's flags must pass arch_mmu_flags_valid(), protect's
 *    arch_mmu_protect_flags_valid(), else ERR_INVALID_ARGS.
 *
 * Each returns NO_ERROR or a negative error, never a count, and one that fails
 * leaves the mappings as they were (a table already empty may be freed on the
 * way). On return no cpu uses a translation that unmap or protect removed or
 * changed, the pages map added are usable from every cpu, and the tables an
 * unmap emptied are freed, after the shootdown.
 *
 * The caller serializes the calls that change an aspace, and keeps interrupts
 * enabled while other cpus are up: riscv and x86 shoot down through
 * mp_sync_exec.
 *
 * map: every page in the range must be unmapped, else ERR_ALREADY_EXISTS; a
 * page under a large page counts as mapped, an existing lower level table does
 * not. A page table allocation failure is ERR_NO_MEMORY. Either way the pages
 * the call mapped and the tables it linked are removed again. The arch may use
 * large pages where vaddr, paddr and count line up.
 *
 * unmap: holes are skipped, so a range with nothing mapped succeeds. A range
 * that covers part of a large page (only its two ends can) or touches an entry
 * the arch keeps fixed (the riscv physmap leaves, the x86-32 kernel physmap) is
 * refused with ERR_NOT_SUPPORTED before anything changes. Emptied tables are
 * freed, except the ones the arch keeps for good: static boot tables, riscv's
 * static kernel tables, every x86-32 table under a kernel directory entry and
 * x86-64's kernel_pml4[511] table.
 *
 * query: ERR_NOT_FOUND if vaddr is unmapped; either out parameter may be NULL.
 * paddr includes vaddr's offset in its page or large page. flags report what
 * the entry encodes: UNCACHED_DEVICE may read back as UNCACHED, riscv always
 * reads back CACHED, and where arch_mmu_supports_nx_mappings() is false every
 * page reads back EXECUTE.
 *
 * protect: replaces the R/W/X permissions of every mapped page in the range
 * with flags, keeping its frame, cache policy, USER, NS and arch private bits.
 * Holes are skipped; it never maps anything and never allocates. EXECUTE on an
 * uncached page is ERR_INVALID_ARGS, and a partly covered large page or a fixed
 * entry ERR_NOT_SUPPORTED, both decided before anything changes. Every changed
 * page is invalidated on every cpu, upgrades included.
 *
 * Replacing the page behind an address is an unmap and a map under the
 * caller's lock. Taking every permission away is an unmap, never a protect.
 */
status_t arch_mmu_map(arch_aspace_t *aspace, vaddr_t vaddr, paddr_t paddr, uint count, uint flags) __NONNULL((1));
status_t arch_mmu_unmap(arch_aspace_t *aspace, vaddr_t vaddr, uint count) __NONNULL((1));
status_t arch_mmu_query(arch_aspace_t *aspace, vaddr_t vaddr, paddr_t *paddr, uint *flags) __NONNULL((1));
status_t arch_mmu_protect(arch_aspace_t *aspace, vaddr_t vaddr, uint count, uint flags) __NONNULL((1));

vaddr_t arch_mmu_pick_spot(arch_aspace_t *aspace,
                           vaddr_t base, uint prev_region_arch_mmu_flags,
                           vaddr_t end,  uint next_region_arch_mmu_flags,
                           vaddr_t align, size_t size, uint arch_mmu_flags) __NONNULL((1));

/*
 * Load a new user address space context on the current cpu.
 * new_aspace NULL unloads user space, leaving only the kernel mapped.
 * old_aspace is the user aspace the cpu had loaded before the call (NULL if
 * none) and is only there for bookkeeping; the arch may ignore it.
 * The scheduler calls this with interrupts disabled.
 */
void arch_mmu_context_switch(arch_aspace_t *old_aspace, arch_aspace_t *new_aspace);

/*
 * Each arch/aspace.h also defines
 *   static inline int arch_aspace_active_cpus(const struct arch_aspace *aspace);
 * returning how many cpus currently have the user aspace loaded, as counted by
 * arch_mmu_context_switch(). An arch that does not keep the count returns 0.
 */

__END_CDECLS

#endif

