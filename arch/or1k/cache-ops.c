/*
 * Copyright (c) 2015 Stefan Kristiansson
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

#include <arch/ops.h>
#include <arch/or1k.h>

static inline uint32_t dc_block_size(void)
{
    uint32_t dccfgr = mfspr(OR1K_SPR_SYS_DCCFGR_ADDR);

    return dccfgr & OR1K_SPR_SYS_DCCFGR_CBS_MASK ? 32 : 16;
}

static inline uint32_t dc_sets(void)
{
    uint32_t dccfgr = mfspr(OR1K_SPR_SYS_DCCFGR_ADDR);

    return 1 << OR1K_SPR_SYS_DCCFGR_NCS_GET(dccfgr);
}

static inline uint32_t ic_block_size(void)
{
    uint32_t iccfgr = mfspr(OR1K_SPR_SYS_ICCFGR_ADDR);

    return iccfgr & OR1K_SPR_SYS_ICCFGR_CBS_MASK ? 32 : 16;
}

static inline uint32_t ic_sets(void)
{
    uint32_t iccfgr = mfspr(OR1K_SPR_SYS_ICCFGR_ADDR);

    return 1 << OR1K_SPR_SYS_ICCFGR_NCS_GET(iccfgr);
}

void arch_invalidate_cache_all(void)
{
    uint32_t i;
    uint32_t cache_size;
    uint32_t block_size;

    block_size = ic_block_size();
    cache_size = block_size * ic_sets();
    for (i = 0; i < cache_size; i += block_size)
        mtspr(OR1K_SPR_ICACHE_ICBIR_ADDR, i);

    block_size = dc_block_size();
    cache_size = block_size * dc_sets();
    for (i = 0; i < cache_size; i += block_size)
        mtspr(OR1K_SPR_DCACHE_DCBIR_ADDR, i);
}

void arch_disable_cache(uint flags)
{
    uint32_t sr = mfspr(OR1K_SPR_SYS_SR_ADDR);

    if (flags & ICACHE)
        sr &= ~OR1K_SPR_SYS_SR_ICE_MASK;
    if (flags & DCACHE)
        sr &= ~OR1K_SPR_SYS_SR_DCE_MASK;

    mtspr(OR1K_SPR_SYS_SR_ADDR, sr);
}

void arch_enable_cache(uint flags)
{
    uint32_t sr = mfspr(OR1K_SPR_SYS_SR_ADDR);

    if (flags & ICACHE)
        sr |= OR1K_SPR_SYS_SR_ICE_MASK;
    if (flags & DCACHE)
        sr |= OR1K_SPR_SYS_SR_DCE_MASK;

    mtspr(OR1K_SPR_SYS_SR_ADDR, sr);
}

/* flush dcache */
void arch_clean_cache_range(addr_t start, size_t len)
{
    addr_t addr;
    uint32_t block_size = dc_block_size();

    for (addr = start; addr < start + len; addr += block_size)
        mtspr(OR1K_SPR_DCACHE_DCBFR_ADDR, addr);
}

/* invalidate dcache */
void arch_invalidate_cache_range(addr_t start, size_t len)
{
    addr_t addr;
    uint32_t block_size = dc_block_size();

    for (addr = start; addr < start + len; addr += block_size)
        mtspr(OR1K_SPR_DCACHE_DCBIR_ADDR, addr);
}

/* flush + invalidate dcache */
void arch_clean_invalidate_cache_range(addr_t start, size_t len)
{
    /* invalidate is implied by flush on or1k */
    arch_clean_cache_range(start, len);
}

/* flush dcache + invalidate icache */
void arch_sync_cache_range(addr_t start, size_t len)
{
    addr_t addr;
    uint32_t block_size = ic_block_size();

    arch_clean_cache_range(start, len);
    for (addr = start; addr < start + len; addr += block_size)
        mtspr(OR1K_SPR_ICACHE_ICBIR_ADDR, addr);
}
