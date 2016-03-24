/*
 * Copyright (c) 2015 Travis Geiselbrecht
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
#include <debug.h>
#include <arch.h>
#include <arch/ops.h>
#include <arch/arm.h>
#include <kernel/thread.h>
#include <kernel/debug.h>
#include <platform.h>
#include <arch/arm/cm.h>

#if ARM_WITH_CACHE

/* cache flushing routines for cortex-m cores that support it */

void arch_disable_cache(uint flags)
{
    if (flags & DCACHE)
        SCB_DisableDCache();

    if (flags & ICACHE)
        SCB_DisableICache();
}

void arch_enable_cache(uint flags)
{
    if (flags & DCACHE)
        SCB_EnableDCache();

    if (flags & ICACHE)
        SCB_EnableICache();
}

/* clean (writeback) data in the data cache on the range */
void arch_clean_cache_range(addr_t start, size_t len)
{
    addr_t end = start + len;

    /* align the start address on CACHE_LINE boundary */
    start &= ~(CACHE_LINE - 1);

    SCB_CleanDCache_by_Addr((uint32_t *)start, end - start);
}

/* clean (writeback) and then evict data from the data cache on the range */
void arch_clean_invalidate_cache_range(addr_t start, size_t len)
{
    addr_t end = start + len;

    /* align the start address on CACHE_LINE boundary */
    start &= ~(CACHE_LINE - 1);

    SCB_CleanInvalidateDCache_by_Addr((uint32_t *)start, end - start);
}

/* evict data from the data cache on the range */
void arch_invalidate_cache_range(addr_t start, size_t len)
{
    addr_t end = start + len;

    /* align the start address on CACHE_LINE boundary */
    start &= ~(CACHE_LINE - 1);

    SCB_InvalidateDCache_by_Addr((uint32_t *)start, end - start);
}

/*
 * clean (writeback) data on the range and then throw away the instruction cache,
 * ensuring that new instructions fetched from the range are not stale.
 */
void arch_sync_cache_range(addr_t start, size_t len)
{
    /* flush the dcache and invalidate the icache, ensuring fresh instructions */
    arch_clean_cache_range(start, len);
    SCB_InvalidateICache();
}

#else

/* doesn't support cache flush, just nop */

void arch_disable_cache(uint flags)
{
}

void arch_enable_cache(uint flags)
{
}

/* clean (writeback) data in the data cache on the range */
void arch_clean_cache_range(addr_t start, size_t len)
{
}

/* clean (writeback) and then evict data from the data cache on the range */
void arch_clean_invalidate_cache_range(addr_t start, size_t len)
{
}

/* evict data from the data cache on the range */
void arch_invalidate_cache_range(addr_t start, size_t len)
{
}

/*
 * clean (writeback) data on the range and then throw away the instruction cache,
 * ensuring that new instructions fetched from the range are not stale.
 */
void arch_sync_cache_range(addr_t start, size_t len)
{
}

#endif // !ARM_WITH_CACHE

