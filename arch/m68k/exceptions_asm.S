/*
 * Copyright (c) 2021 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/asm.h>

.text

.macro irq_vector n
m68k_irq_vector_\n:
    moveml  %d0-%d7/%a0-%a6, %sp@-

    pea     \n
    movel   %sp,%sp@-
    jsr     m68k_irq
    addaw   #8, %sp

    moveml  %sp@+, %d0-%d7/%a0-%a6
    rte
.endm

irq_vector 0
irq_vector 1
irq_vector 2
irq_vector 3
irq_vector 4
irq_vector 5
irq_vector 6
irq_vector 7

.macro exception_vector name n
\name:
    moveml  %d0-%d7/%a0-%a6, %sp@-

    pea     \n
    movel   %sp,%sp@-
    jsr     m68k_exception
    addaw   #8, %sp

    moveml  %sp@+, %d0-%d7/%a0-%a6
    rte
.endm

exception_vector m68k_access_fault 2
exception_vector m68k_address_error 3
exception_vector m68k_illegal_vector 4
exception_vector m68k_div_by_zero 5
exception_vector m68k_chk 6
exception_vector m68k_trap 7
exception_vector m68k_priv_violation 8
exception_vector m68k_trace 9
exception_vector m68k_unimp_aline 10
exception_vector m68k_unimp_fline 11

.section .text.vectab
.align 4
DATA(exc_vectors)
.org 0x8
    .long m68k_access_fault
    .long m68k_address_error
    .long m68k_illegal_vector
    .long m68k_div_by_zero
    .long m68k_chk
    .long m68k_trap
    .long m68k_priv_violation
    .long m68k_trace
    .long m68k_unimp_aline
    .long m68k_unimp_fline
.org 0x60
    .long m68k_irq_vector_0
    .long m68k_irq_vector_1
    .long m68k_irq_vector_2
    .long m68k_irq_vector_3
    .long m68k_irq_vector_4
    .long m68k_irq_vector_5
    .long m68k_irq_vector_6
    .long m68k_irq_vector_7
.org 4*256
END_DATA(exc_vectors)

