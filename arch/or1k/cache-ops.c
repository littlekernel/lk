/*
 * Copyright (c) 2015 Stefan Kristiansson
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <arch/ops.h>
#include <arch/or1k.h>

static inline uint32_t dc_block_size(void) {
    uint32_t dccfgr = mfspr(OR1K_SPR_SYS_DCCFGR_ADDR);

    return dccfgr & OR1K_SPR_SYS_DCCFGR_CBS_MASK ? 32 : 16;
}

static inline uint32_t dc_sets(void) {
    uint32_t dccfgr = mfspr(OR1K_SPR_SYS_DCCFGR_ADDR);

    return 1 << OR1K_SPR_SYS_DCCFGR_NCS_GET(dccfgr);
}

static inline uint32_t ic_block_size(void) {
    uint32_t iccfgr = mfspr(OR1K_SPR_SYS_ICCFGR_ADDR);

    return iccfgr & OR1K_SPR_SYS_ICCFGR_CBS_MASK ? 32 : 16;
}

static inline uint32_t ic_sets(void) {
    uint32_t iccfgr = mfspr(OR1K_SPR_SYS_ICCFGR_ADDR);

    return 1 << OR1K_SPR_SYS_ICCFGR_NCS_GET(iccfgr);
}

void arch_invalidate_cache_all(void) {
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

void arch_disable_cache(uint flags) {
    uint32_t sr = mfspr(OR1K_SPR_SYS_SR_ADDR);

    if (flags & ARCH_CACHE_FLAG_ICACHE)
        sr &= ~OR1K_SPR_SYS_SR_ICE_MASK;
    if (flags & ARCH_CACHE_FLAG_DCACHE)
        sr &= ~OR1K_SPR_SYS_SR_DCE_MASK;

    mtspr(OR1K_SPR_SYS_SR_ADDR, sr);
}

void arch_enable_cache(uint flags) {
    uint32_t sr = mfspr(OR1K_SPR_SYS_SR_ADDR);

    if (flags & ARCH_CACHE_FLAG_ICACHE)
        sr |= OR1K_SPR_SYS_SR_ICE_MASK;
    if (flags & ARCH_CACHE_FLAG_DCACHE)
        sr |= OR1K_SPR_SYS_SR_DCE_MASK;

    mtspr(OR1K_SPR_SYS_SR_ADDR, sr);
}

/* flush dcache */
void arch_clean_cache_range(addr_t start, size_t len) {
    addr_t addr;
    uint32_t block_size = dc_block_size();

    for (addr = start; addr < start + len; addr += block_size)
        mtspr(OR1K_SPR_DCACHE_DCBFR_ADDR, addr);
}

/* invalidate dcache */
void arch_invalidate_cache_range(addr_t start, size_t len) {
    addr_t addr;
    uint32_t block_size = dc_block_size();

    for (addr = start; addr < start + len; addr += block_size)
        mtspr(OR1K_SPR_DCACHE_DCBIR_ADDR, addr);
}

/* flush + invalidate dcache */
void arch_clean_invalidate_cache_range(addr_t start, size_t len) {
    /* invalidate is implied by flush on or1k */
    arch_clean_cache_range(start, len);
}

/* flush dcache + invalidate icache */
void arch_sync_cache_range(addr_t start, size_t len) {
    addr_t addr;
    uint32_t block_size = ic_block_size();

    arch_clean_cache_range(start, len);
    for (addr = start; addr < start + len; addr += block_size)
        mtspr(OR1K_SPR_ICACHE_ICBIR_ADDR, addr);
}
