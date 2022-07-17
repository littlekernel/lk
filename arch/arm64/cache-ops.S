/*
 * Copyright (c) 2014, Google Inc. All rights reserved
 * Copyright 2016 The Fuchsia Authors
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <lk/asm.h>
#include <arch/ops.h>
#include <arch/defines.h>
#include <arch/arm64/cache_loop.h>

.text

// Routines to flush the cache by address on the local cpu.
// NOTE: the following routines do not touch the stack and only use x0-x3
// so can be safely called from assembly that is aware of this.

.macro cache_range_op, cache op
    add     x2, x0, x1                  // calculate the end address
    bic     x3, x0, #(CACHE_LINE-1)     // align the start with a cache line
.Lcache_range_op_loop\@:
    \cache  \op, x3
    add     x3, x3, #CACHE_LINE
    cmp     x3, x2
    blo     .Lcache_range_op_loop\@
    dsb     sy
.endm

    /* void arch_flush_cache_range(addr_t start, size_t len); */
FUNCTION(arch_clean_cache_range)
    cache_range_op dc cvac         // clean cache to PoC by MVA
    ret
END_FUNCTION(arch_clean_cache_range)

    /* void arch_flush_invalidate_cache_range(addr_t start, size_t len); */
FUNCTION(arch_clean_invalidate_cache_range)
    cache_range_op dc civac        // clean & invalidate dcache to PoC by MVA
    ret
END_FUNCTION(arch_clean_invalidate_cache_range)

    /* void arch_invalidate_cache_range(addr_t start, size_t len); */
FUNCTION(arch_invalidate_cache_range)
    cache_range_op dc ivac         // invalidate dcache to PoC by MVA
    ret
END_FUNCTION(arch_invalidate_cache_range)

    /* void arch_sync_cache_range(addr_t start, size_t len); */
FUNCTION(arch_sync_cache_range)
    cache_range_op dc cvau         // clean dcache to PoU by MVA
    cache_range_op ic ivau         // invalidate icache to PoU by MVA
    ret
END_FUNCTION(arch_sync_cache_range)

// Below are 3 variants of cache flushing routines by way/set for
// an individual cpu.
// NOTE: does not touch the stack but trashes most of the temporary
// registers.

    // void arm64_local_invalidate_cache_all()
FUNCTION(arm64_local_invalidate_cache_all)
    cache_way_set_op isw, invalidate

    // dump the instruction cache as well
    ic      iallu
    isb

    ret
END_FUNCTION(arm64_local_invalidate_cache_all)

    // void arm64_local_clean_cache_all()
FUNCTION(arm64_local_clean_cache_all)
    cache_way_set_op csw, clean

    // dump the instruction cache as well
    ic      iallu
    isb

    ret
END_FUNCTION(arm64_local_clean_cache_all)

    // void arm64_local_clean_invalidate_cache_all()
FUNCTION(arm64_local_clean_invalidate_cache_all)
    cache_way_set_op cisw, clean_invalidate

    // dump the instruction cache as well
    ic      iallu
    isb

    ret
END_FUNCTION(arm64_local_clean_invalidate_cache_all)

