/*
 * Copyright (c) 2008-2014 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#ifndef ASSEMBLY

#include <lk/compiler.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

__BEGIN_CDECLS

/* Fast routines that all arches must implement inline in arch_ops.h. */
static inline ulong arch_cycle_count(void);
static inline uint arch_curr_cpu_num(void);

/* Use to align structures on cache lines to avoid cpu aliasing. */
#define __CPU_ALIGN __ALIGNED(CACHE_LINE)

/* Optional: an arch that keeps a per-cpu pointer in a register (or segment)
 * can define ARCH_HAS_KERNEL_PERCPU_PTR to 1 and provide
 *
 *   static inline void *arch_get_kernel_percpu(void);
 *
 * returning the local cpu's struct percpu (kernel/percpu.h), which the arch
 * stores alongside its own per-cpu data when it sets that up. It lets
 * percpu_local() be a single load instead of a cpu number lookup and an index
 * computation, and it is on the path of every spin_lock() in a debug build.
 * Without it the kernel indexes percpu_array by arch_curr_cpu_num(). */

void arch_disable_cache(uint flags);
void arch_enable_cache(uint flags);

void arch_clean_cache_range(addr_t start, size_t len);
void arch_clean_invalidate_cache_range(addr_t start, size_t len);
void arch_invalidate_cache_range(addr_t start, size_t len);
void arch_sync_cache_range(addr_t start, size_t len);

void arch_idle(void);

__END_CDECLS

#endif // !ASSEMBLY

/* for the above arch enable/disable routines */
#define ARCH_CACHE_FLAG_ICACHE 1
#define ARCH_CACHE_FLAG_DCACHE 2
#define ARCH_CACHE_FLAG_UCACHE (ARCH_CACHE_FLAG_ICACHE | ARCH_CACHE_FLAG_DCACHE)

/* include the arch specific implementations */
#include <arch/arch_ops.h>
