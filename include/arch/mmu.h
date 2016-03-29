/*
 * Copyright (c) 2014-2016 Travis Geiselbrecht
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
#pragma once

#include <arch.h>
#include <sys/types.h>
#include <compiler.h>

/* to bring in definition of arch_aspace */
#include <arch/aspace.h>

__BEGIN_CDECLS

#define ARCH_MMU_FLAG_CACHED            (0<<0)
#define ARCH_MMU_FLAG_UNCACHED          (1<<0)
#define ARCH_MMU_FLAG_UNCACHED_DEVICE   (2<<0) /* only exists on some arches, otherwise UNCACHED */
#define ARCH_MMU_FLAG_CACHE_MASK        (3<<0)

#define ARCH_MMU_FLAG_PERM_USER         (1<<2)
#define ARCH_MMU_FLAG_PERM_RO           (1<<3)
#define ARCH_MMU_FLAG_PERM_NO_EXECUTE   (1<<4)
#define ARCH_MMU_FLAG_NS                (1<<5) /* NON-SECURE */
#define ARCH_MMU_FLAG_INVALID           (1<<7) /* indicates that flags are not specified */

/* forward declare the per-address space arch-specific context object */
typedef struct arch_aspace arch_aspace_t;

#define ARCH_ASPACE_FLAG_KERNEL         (1<<0)

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

/* load a new user address space context.
 * aspace argument NULL should unload user space.
 */
void arch_mmu_context_switch(arch_aspace_t *aspace);

void arch_disable_mmu(void);

__END_CDECLS

