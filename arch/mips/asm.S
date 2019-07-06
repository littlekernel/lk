/*
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/asm.h>

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

