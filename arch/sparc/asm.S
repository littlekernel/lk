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
    // o6 = %sp old thread stack pointer
    // o7 = %return address to the caller - 8
    // i0-i7 are owned by the caller
    // l0-l7 are owned by the caller
    // g1-g4 are scratch registers
    save    %sp, -96, %sp; // create a new stack frame

    // current state: CWP-1
    // i0 = oldcs
    // i1 = newcs
    // i6 = %sp old thread stack pointer
    // i7 = %return address to the caller - 8
    // o6 = old thread stack pointer -96
    // g1-g4 are scratch registers
    // l0-l7 are owned by us
    // o0-o7 are owned by us

    // save the context we came from into the oldcs frame
    st      %i6, [%i0 + 0]; // save the old frame pointer
    st      %i7, [%i0 + 4]; // save from pc

    // force the overflow handler to spill out 6 frames (NWINDOWS - 2)
    set     0, %g1
0:
    save    %sp, 0, %sp
    add     %g1, 1, %g1
    cmp     %g1, 6
    bl      0b
    nop

# 0: b 0b; nop

0:
    // now unwind back to the original frame
    sub     %g1, 1, %g1
    restore
    cmp     %g1, 0
    bne      0b
    nop

    // now we're on the original frame, and it's guaranteed
    // that no other frames are active
    ld      [%i1 + 0], %i6; // restore frame pointer %fp (i6)
    ld      [%i1 + 4], %i7; // restore return address %i7

    // go back to the callers frame
    restore // should trigger an underflow exception

    // the i registers we set above before are now o register

# 0: b 0b; nop

    retl    // jumps to o7+8
    nop
END_FUNCTION(sparc_context_switch)
