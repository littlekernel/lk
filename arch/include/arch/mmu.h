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

#define ARCH_MMU_FLAG_PERM_USER         (1U<<2)
#define ARCH_MMU_FLAG_PERM_RO           (1U<<3)
#define ARCH_MMU_FLAG_PERM_NO_EXECUTE   (1U<<4) /* supported on most, but not all arches */
#define ARCH_MMU_FLAG_NS                (1U<<5) /* supported on some arches */
#define ARCH_MMU_FLAG_INVALID           (1U<<6) /* indicates that flags are not specified */

/* arch level query of some features at the mapping/query level */
bool arch_mmu_supports_nx_mappings(void);
bool arch_mmu_supports_ns_mappings(void);
bool arch_mmu_supports_user_aspaces(void);

/* forward declare the per-address space arch-specific context object */
typedef struct arch_aspace arch_aspace_t;

#define ARCH_ASPACE_FLAG_KERNEL         (1U<<0)

/* [vaddr, vaddr + count pages) lies inside the aspace. Computed so an aspace
 * that runs to the top of the address space does not overflow. Map, unmap and
 * query return ERR_OUT_OF_RANGE for anything outside. */
static inline bool arch_mmu_range_in_aspace(const arch_aspace_t *aspace, vaddr_t vaddr, uint count) {
    if (vaddr < aspace->base || vaddr > aspace->base + aspace->size - 1) {
        return false;
    }
    const size_t pages_left = (aspace->base + aspace->size - 1 - vaddr) / PAGE_SIZE + 1;
    return count <= pages_left;
}

/* initialize per address space */
status_t arch_mmu_init_aspace(arch_aspace_t *aspace, vaddr_t base, size_t size, uint flags) __NONNULL((1));
status_t arch_mmu_destroy_aspace(arch_aspace_t *aspace) __NONNULL((1));

/* Routines to map/unmap/query mappings per address space. A map that fails
 * leaves nothing of its range behind. An unmap may refuse part of its range
 * with ERR_NOT_SUPPORTED where the arch would have to split a large page or
 * write a top level entry it keeps fixed; what came before that point in the
 * range is already unmapped by then. The vmm only ever unmaps exactly the
 * ranges it mapped. */
int arch_mmu_map(arch_aspace_t *aspace, vaddr_t vaddr, paddr_t paddr, uint count, uint flags) __NONNULL((1));
int arch_mmu_unmap(arch_aspace_t *aspace, vaddr_t vaddr, uint count) __NONNULL((1));
status_t arch_mmu_query(arch_aspace_t *aspace, vaddr_t vaddr, paddr_t *paddr, uint *flags) __NONNULL((1));

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

