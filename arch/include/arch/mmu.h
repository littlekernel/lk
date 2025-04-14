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

/* initialize per address space */
status_t arch_mmu_init_aspace(arch_aspace_t *aspace, vaddr_t base, size_t size, uint flags) __NONNULL((1));
status_t arch_mmu_destroy_aspace(arch_aspace_t *aspace) __NONNULL((1));

/* routines to map/unmap/query mappings per address space */
int arch_mmu_map(arch_aspace_t *aspace, vaddr_t vaddr, paddr_t paddr, uint count, uint flags) __NONNULL((1));
int arch_mmu_unmap(arch_aspace_t *aspace, vaddr_t vaddr, uint count) __NONNULL((1));
status_t arch_mmu_query(arch_aspace_t *aspace, vaddr_t vaddr, paddr_t *paddr, uint *flags) __NONNULL((1));

vaddr_t arch_mmu_pick_spot(arch_aspace_t *aspace,
                           vaddr_t base, uint prev_region_arch_mmu_flags,
                           vaddr_t end,  uint next_region_arch_mmu_flags,
                           vaddr_t align, size_t size, uint arch_mmu_flags) __NONNULL((1));

/*
 * load a new user address space context.
 * aspace argument NULL should unload user space.
 */
void arch_mmu_context_switch(arch_aspace_t *aspace);

__END_CDECLS

#endif

