//
// Copyright (c) 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
//
#include <lk/asm.h>

/* void sparc_context_switch(
    struct sparc_context_switch_frame *oldcs,
    struct sparc_context_switch_frame *newcs); */
FUNCTION(sparc_context_switch)
    // state at the start of the function:
    // o0 = oldcs
    // o1 = newcs
    save    %sp, -96, %sp; // create a new stack frame

    // save the current context into oldcs
    st      %sp, [%i0 + 0]; // save sp
    st      %i7, [%i0 + 4]; // save from pc

    // force the overflow handler to spill out 6 frames (NWINDOWS - 2)
    set     0, %g1
0:
    save    %sp, 0, %sp
    add     %g1, 1, %g1
    cmp     %g1, 6
    bl      0b
    nop

0:
    // now unwind back to the original frame
    sub     %g1, 1, %g1
    restore
    cmp     %g1, 0
    bg      0b
    nop

    // now we're on the original frame, and it's guaranteed
    // that no other frames are active
    ld      [%i1 + 0], %sp; // restore sp
    ld      [%i1 + 4], %i7; // restore pc

    // go back to the callers frame
    restore

    retl
    nop
END_FUNCTION(sparc_context_switch)
