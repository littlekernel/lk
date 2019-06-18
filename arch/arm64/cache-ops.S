/*
 * Copyright (c) 2014, Google Inc. All rights reserved
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

#include <asm.h>
#include <arch/ops.h>
#include <arch/defines.h>
#include <arch/asm_macros.h>

.text

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

    /* void arch_flush_invalidate_cache_range(addr_t start, size_t len); */
FUNCTION(arch_clean_invalidate_cache_range)
    cache_range_op dc civac        // clean & invalidate dcache to PoC by MVA
    ret

    /* void arch_invalidate_cache_range(addr_t start, size_t len); */
FUNCTION(arch_invalidate_cache_range)
    cache_range_op dc ivac         // invalidate dcache to PoC by MVA
    ret

    /* void arch_sync_cache_range(addr_t start, size_t len); */
FUNCTION(arch_sync_cache_range)
    cache_range_op dc cvau         // clean dcache to PoU by MVA
    cache_range_op ic ivau         // invalidate icache to PoU by MVA
    ret

    /* void arch_disable_cache(uint flags); */
FUNCTION(arch_disable_cache)
    // LK runs in EL1

    /* Disable iCache and dCache */
.inEL1:
    mrs x4, sctlr_el1
    bic x4, x4, #(1 << 12) // Disable iCache
    bic x4, x4, #(1 << 2)  // Disable dCache
    msr sctlr_el1, x4
    isb

    /* Clean and Invalidate dCache */
    mrs x0, CLIDR_EL1
    and w3, w0, #0x07000000     // Bits: 26:24 Level of Coherence
    lsr w3, w3, #23             // Store 2 x LoC in W3
    cbz w3, Finished            // If 0, we are done
    mov w10, #0                 // store 2x cache level (since csselr starts at bit 1)
    mov w8, #1

Loop1:
    add w2, w10, w10, lsr #1    // Calculate 3x cache level a(w10 + 2w10 = 3w10)
    lsr w1, w0, w2              // read cType (cache type)
    and w1, w1, #0x7            // mask 3-bits
    cmp w1, #2                  // types >=2 include data cache
    b.lt Skip                   // skip if no data cache implemented
    msr csselr_el1, x10         // select the cache level
    isb                         // sync
    mrs x1, ccsidr_el1          // read ccsidr (current cache size id)
    and w2, w1, #0x7            // w2 = log2(linesize) - 4
    add w2, w2, #4              // w2 = log2(linesize)
    ubfx w4, w1, #3, #10        // w4 = way number (associativity)
    clz w5, w4                  // w5 = 32 - log2(ways), bit pos in dc operand
    lsl w9, w4, w5              // w9 = max way number, aligned to position in dc operand
    lsl w16, w8, w5             // w16 = amount to decrement way number per iteration

Loop2:
    ubfx w7, w1, #13, #15       // w7 = max set number
    lsl w7, w7, w2              // w7= max set number, aligned to position in dc operand
    lsl w17, w8, w2             // w17 = amount to decrement set number per iteration

Loop3:
    orr w11, w10, w9            // w11 = combine way number, cache number and set num for dc operand
    orr w11, w11, w7
    dc cisw, x11                 // perform clean by set and way
    subs w7, w7, w17            // decrement set number
    b.ge Loop3
    subs x9, x9, x16            // decrement way number
    b.ge Loop2

Skip:
    add w10, w10, #2
    cmp w3, w10
    dsb sy
    b.gt Loop1

Finished:
    /* invalidate iCache*/
    ic ialluis
    isb

    /* invalidate TLB */
    tlbi vmalle1
    dsb sy
    isb
    ret


