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
    // leaf function, operating directly in arch_context_switch's window.
    // o0 = oldcs
    // o1 = newcs
    // o6 = %sp (old thread stack pointer)
    // o7 = return address to arch_context_switch's caller

    // save the context we came from into the oldcs frame
    st      %o6, [%o0 + 0] // save the old stack
    st      %o7, [%o0 + 4] // save from pc

    // save the current WIM to %g3
    rd      %wim, %g3

    // get the starting CWP from %psr
    rd      %psr, %g1
    and     %g1, 0x1f, %g1

    // keep count of how many restores we did in %g2
    mov     0, %g2

.Lspill_loop:
    // compute next_cwp = (CWP + 1) % 8
    add     %g1, 1, %g1
    and     %g1, 7, %g1

    // check if 1 << next_cwp matches WIM
    mov     1, %g4
    sll     %g4, %g1, %g4
    cmp     %g4, %g3
    beq     .Lspill_done
    nop

    // do restore to enter the next window (our caller)
    restore
    std     %l0, [%sp + 0]
    std     %l2, [%sp + 8]
    std     %l4, [%sp + 16]
    std     %l6, [%sp + 24]
    std     %i0, [%sp + 32]
    std     %i2, [%sp + 40]
    std     %i4, [%sp + 48]
    std     %i6, [%sp + 56]

    b       .Lspill_loop
    add     %g2, 1, %g2

.Lspill_done:
    // return CWP to the starting window by doing save %g2 times
    cmp     %g2, 0
    beq     .Lout
    nop

.Lrestore_loop:
    save
    subcc   %g2, 1, %g2
    bne     .Lrestore_loop
    nop

.Lout:

    // save the local and input registers of the old thread to its context switch frame
    std     %l0, [%o0 + 8]
    std     %l2, [%o0 + 16]
    std     %l4, [%o0 + 24]
    std     %l6, [%o0 + 32]
    std     %i0, [%o0 + 40]
    std     %i2, [%o0 + 48]
    std     %i4, [%o0 + 56]
    std     %i6, [%o0 + 64]

    // switch stack pointer and return address (PC) to the new thread
    ld      [%o1 + 0], %o6
    ld      [%o1 + 4], %o7

    // restore the local and input registers of the new thread from its context switch frame
    ldd     [%o1 + 8], %l0
    ldd     [%o1 + 16], %l2
    ldd     [%o1 + 24], %l4
    ldd     [%o1 + 32], %l6
    ldd     [%o1 + 40], %i0
    ldd     [%o1 + 48], %i2
    ldd     [%o1 + 56], %i4
    ldd     [%o1 + 64], %i6

    // invalidate all other windows so that the new thread is forced
    // to reload its older frames from the stack on every restore.
    // We do this by setting the firewall to the next restore target window (CWP + 1)
    rd      %psr, %g1
    and     %g1, 0x1f, %g1
    add     %g1, 1, %g1
    and     %g1, 7, %g1
    mov     1, %g2
    sll     %g2, %g1, %g2
    wr      %g2, 0, %wim
    nop; nop; nop

    // go back to the caller's frame
    retl
    nop
END_FUNCTION(sparc_context_switch)
