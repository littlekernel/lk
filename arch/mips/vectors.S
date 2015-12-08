/*
 * Copyright (c) 2015 Travis Geiselbrecht
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
#include <arch/mips.h>

.section ".text.vectab"
FUNCTION(vectab)
.org 0
_tlb_refill:
    b       .

.macro iframe_save
    .set    push
    .set    noat
    addiu   $sp, -88

    /* save all the non temporary registers */
    sw      $at, 0($sp)
    sw      $v0, 4($sp)
    sw      $v1, 8($sp)
    sw      $a0, 12($sp)
    sw      $a1, 16($sp)
    sw      $a2, 20($sp)
    sw      $a3, 24($sp)
    sw      $t0, 28($sp)
    sw      $t1, 32($sp)
    sw      $t2, 36($sp)
    sw      $t3, 40($sp)
    sw      $t4, 44($sp)
    sw      $t5, 48($sp)
    sw      $t6, 52($sp)
    sw      $t7, 56($sp)
    sw      $t8, 60($sp)
    sw      $t9, 64($sp)
    sw      $gp, 68($sp)
    sw      $ra, 72($sp)

    /* save the control registers */
    mfc0    $at, $12 /* status */
    sw      $at, 76($sp)
    mfc0    $at, $13 /* cause */
    sw      $at, 80($sp)
    mfc0    $at, $14 /* epc */
    sw      $at, 84($sp)

    .set    pop
.endm

.macro iframe_restore
    .set    push
    .set    noat

    /* restore the temporary registers */
    lw      $at, 0($sp)
    lw      $v0, 4($sp)
    lw      $v1, 8($sp)
    lw      $a0, 12($sp)
    lw      $a1, 16($sp)
    lw      $a2, 20($sp)
    lw      $a3, 24($sp)
    lw      $t0, 28($sp)
    lw      $t1, 32($sp)
    lw      $t2, 36($sp)
    lw      $t3, 40($sp)
    lw      $t4, 44($sp)
    lw      $t5, 48($sp)
    lw      $t6, 52($sp)
    lw      $t7, 56($sp)
    lw      $t8, 60($sp)
    lw      $t9, 64($sp)
    lw      $gp, 68($sp)
    lw      $ra, 72($sp)

    /* restore the control registers */
    lw      $k0, 76($sp)
    mtc0    $k0, $12 /* status */
    lw      $k0, 80($sp)
    mtc0    $k0, $13 /* cause */
    lw      $k0, 84($sp)
    mtc0    $k0, $14 /* epc */

    addiu   $sp, 88
    .set    pop
.endm

/* compatibility mode irq/syscall/general exception */
.org 0x180
_irq:
    la      $k0, mips_gen_exception
    li      $k1, 0
    b       shared_irq_save_return

/* vectored base */
.macro vectored_irq, num
.org 0x200 + VECTORED_OFFSET_SHIFT * \num
_vectored_irq\num:
    la      $k0, mips_irq
    li      $k1, \num
    b       shared_irq_save_return
    b       .
.endm

vectored_irq 0
vectored_irq 1
vectored_irq 2
vectored_irq 3
vectored_irq 4
vectored_irq 5
vectored_irq 6
vectored_irq 7
vectored_irq 8
vectored_irq 9

/* branched to from above, k0 holds address to call, k1 holds arg to function */
shared_irq_save_return:
    iframe_save

    move    $a0, $sp
    move    $a1, $k1
    jal     $k0

    iframe_restore

    eret

