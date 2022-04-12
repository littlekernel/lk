/*
 * Copyright (c) 2021 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/asm.h>

.text

#if M68K_CPU >= 68010
// For 68010s and above the exception frame already has a code pushed on the stack,
// so it's easy to just vector most of the handlers into one of a few classes of handlers
// and decode the vector in C.

.align 4
_m68k_irq_vector:
    // TODO: save less state for IRQs
    moveml  %d0-%d7/%a0-%a6, %sp@-

    movel   %sp,%sp@-
    jsr     m68k_irq
    add     #4, %sp

    moveml  %sp@+, %d0-%d7/%a0-%a6
    rte

.macro exception_vector name func
.align 4
\name:
    moveml  %d0-%d7/%a0-%a6, %sp@-

    movel   %sp,%sp@-
    jsr     \func
    add     #4, %sp

    moveml  %sp@+, %d0-%d7/%a0-%a6
    rte
.endm

exception_vector _m68k_general_exception m68k_exception
exception_vector _m68k_trap_exception m68k_trap_exception

.section .text.vectab
.align 16
DATA(exc_vectors)
    // first two entries are the reset vector
    .long 0
    .long 0
    // general exceptions
    .rept (15 - 2)
    .long _m68k_general_exception
    .endr
.org (15*4)
    // uninitialized irq vector
    .long _m68k_irq_vector
.org (16*4)
    .rept (24 - 16)
    .long _m68k_general_exception
    .endr
.org (24 * 4) // offset 0x60
    // start of autovectored interrupts
    .rept (32 - 24)
    .long _m68k_irq_vector
    .endr
.org (32 * 4) // offset 0x60
    // index 32, offset 0x80
    .rept (48 - 32)
    .long _m68k_trap_exception
    .endr
.org (48 * 4)
    // start of FPU, MMU vectors
    .rept (64 - 48)
    .long _m68k_general_exception
    .endr
.org (64 * 4) // offset 0x100
    // index 64, offset 0x100, end of reserved vectors
    // start of user vectors
    .rept (256 - 64)
    .long _m68k_irq_vector
    .endr
.org 4*256
END_DATA(exc_vectors)

#else
#error add support for pre 68010 exceptions
#endif
