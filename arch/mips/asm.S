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

/* void mips_context_switch(
    struct mips_context_switch_frame *oldcs,
    struct mips_context_switch_frame *newcs); */
FUNCTION(mips_context_switch)
    # a0 = oldcs
    # a1 = newcs

    # save old state
    sw      $s0, 0($a0)
    sw      $s1, 4($a0)
    sw      $s2, 8($a0)
    sw      $s3, 12($a0)
    sw      $s4, 16($a0)
    sw      $s5, 20($a0)
    sw      $s6, 24($a0)
    sw      $s7, 28($a0)
    sw      $s8, 32($a0)
    sw      $ra, 36($a0)
    sw      $sp, 40($a0)

    # load new state
    lw      $s0, 0($a1)
    lw      $s1, 4($a1)
    lw      $s2, 8($a1)
    lw      $s3, 12($a1)
    lw      $s4, 16($a1)
    lw      $s5, 20($a1)
    lw      $s6, 24($a1)
    lw      $s7, 28($a1)
    lw      $s8, 32($a1)
    lw      $ra, 36($a1)
    lw      $sp, 40($a1)

    jr      $ra
    nop

