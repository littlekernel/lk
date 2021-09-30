// Copyright 2020 The Fuchsia Authors
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#pragma once

// clang-format off

#ifdef __ASSEMBLER__

// Routine to iterate over all ways/sets across all levels of data caches
// from level 0 to the point of coherence.
//
// Adapted from example code in the ARM Architecture Reference Manual ARMv8.
.macro cache_way_set_op, op name
    mrs     x0, clidr_el1
    and     w3, w0, #0x07000000     // get 2x level of coherence
    lsr     w3, w3, #23
    cbz     w3, .Lfinished_\name
    mov     w10, #0                 // w10 = 2x cache level
    mov     w8, #1                  // w8 = constant 1
.Lloop1_\name:
    add     w2, w10, w10, lsr #1    // calculate 3x cache level
    lsr     w1, w0, w2              // extract 3 bit cache type for this level
    and     w1, w1, #0x7
    cmp     w1, #2
    b.lt    .Lskip_\name            // no data or unified cache at this level
    msr     csselr_el1, x10         // select this cache level
    isb                             // synchronize change to csselr
    mrs     x1, ccsidr_el1          // w1 = ccsidr
    and     w2, w1, #7              // w2 = log2(line len) - 4
    add     w2, w2, #4              // w2 = log2(line len)
    ubfx    w4, w1, #3, #10         // w4 = max way number, right aligned
    clz     w5, w4                  // w5 = 32 - log2(ways), bit position of way in DC operand
    lsl     w9, w4, w5              // w9 = max way number, aligned to position in DC operand
    lsl     w12, w8, w5             // w12 = amount to decrement way number per iteration

.Lloop2_\name:
    ubfx    w7, w1, #13, #15        // w7 = max set number, right aligned
    lsl     w7, w7, w2              // w7 = max set number, aligned to position in DC operand
    lsl     w13, w8, w2             // w13 = amount to decrement set number per iteration
.Lloop3_\name:
    orr     w11, w10, w9            // w11 = combine way number and cache number
    orr     w11, w11, w7            //       and set number for DC operand
    dc      \op, x11                // data cache op
    subs    w7, w7, w13             // decrement set number
    b.ge    .Lloop3_\name

    subs    x9, x9, x12             // decrement way number
    b.ge    .Lloop2_\name
.Lskip_\name:
    add     w10, w10, #2            // increment 2x cache level
    cmp     w3, w10
    dsb     sy                      // ensure completetion of previous cache maintainance instructions
    b.gt    .Lloop1_\name
.Lfinished_\name:
.endm

#endif
