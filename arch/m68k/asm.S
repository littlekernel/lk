/*
 * Copyright (c) 2021 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/asm.h>

// void m68k_context_switch(struct m68k_context_switch_frame *oldcs, struct m68k_context_switch_frame *newcs);
FUNCTION(m68k_context_switch)
    movel   %sp@+,%d0           // pop PC off the stack
    movel   %sp@,%a0            // oldcs
    movel   %sp@(4),%a1         // newcs

    // save old state
    movel   %sp, %a0@(0)             // oldcs.sp
    movel   %d0, %a0@(4)             // oldcs.pc
    moveml  %d2-%d7/%a2-%a6, %a0@(8) // oldcs.<base of callee saved list>

    // load new state
    movel   %a1@(0), %sp             // newcs.sp
    movel   %a1@(4), %sp@-           // newcs.pc -> stack
    moveml  %a1@(8), %d2-%d7/%a2-%a6 // newcs.<base of callee saved list>

    // return to new PC
    rts
END_FUNCTION(m68k_context_switch)

