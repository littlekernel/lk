/*
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/debug.h>
#include <arch.h>
#include <arch/ops.h>
#include <arch/arm.h>
#include <kernel/thread.h>
#include <kernel/debug.h>
#include <platform.h>
#include <arch/arm/cm.h>

#if ARM_WITH_CACHE

/* cache flushing routines for cortex-m cores that support it */

void arch_disable_cache(uint flags) {
    if (flags & ARCH_CACHE_FLAG_DCACHE)
        SCB_DisableDCache();

    if (flags & ARCH_CACHE_FLAG_ICACHE)
        SCB_DisableICache();
}

void arch_enable_cache(uint flags) {
    if (flags & ARCH_CACHE_FLAG_DCACHE)
        SCB_EnableDCache();

    if (flags & ARCH_CACHE_FLAG_ICACHE)
        SCB_EnableICache();
}

/* clean (writeback) data in the data cache on the range */
void arch_clean_cache_range(addr_t start, size_t len) {
    addr_t end = start + len;

    /* align the start address on CACHE_LINE boundary */
    start &= ~(CACHE_LINE - 1);

    SCB_CleanDCache_by_Addr((uint32_t *)start, end - start);
}

/* clean (writeback) and then evict data from the data cache on the range */
void arch_clean_invalidate_cache_range(addr_t start, size_t len) {
    addr_t end = start + len;

    /* align the start address on CACHE_LINE boundary */
    start &= ~(CACHE_LINE - 1);

    SCB_CleanInvalidateDCache_by_Addr((uint32_t *)start, end - start);
}

/* evict data from the data cache on the range */
void arch_invalidate_cache_range(addr_t start, size_t len) {
    addr_t end = start + len;

    /* align the start address on CACHE_LINE boundary */
    start &= ~(CACHE_LINE - 1);

    SCB_InvalidateDCache_by_Addr((uint32_t *)start, end - start);
}

/*
 * clean (writeback) data on the range and then throw away the instruction cache,
 * ensuring that new instructions fetched from the range are not stale.
 */
void arch_sync_cache_range(addr_t start, size_t len) {
    /* flush the dcache and invalidate the icache, ensuring fresh instructions */
    arch_clean_cache_range(start, len);
    SCB_InvalidateICache();
}

#else

/* doesn't support cache flush, just nop */

void arch_disable_cache(uint flags) {
}

void arch_enable_cache(uint flags) {
}

/* clean (writeback) data in the data cache on the range */
void arch_clean_cache_range(addr_t start, size_t len) {
}

/* clean (writeback) and then evict data from the data cache on the range */
void arch_clean_invalidate_cache_range(addr_t start, size_t len) {
}

/* evict data from the data cache on the range */
void arch_invalidate_cache_range(addr_t start, size_t len) {
}

/*
 * clean (writeback) data on the range and then throw away the instruction cache,
 * ensuring that new instructions fetched from the range are not stale.
 */
void arch_sync_cache_range(addr_t start, size_t len) {
}

#endif // !ARM_WITH_CACHE

