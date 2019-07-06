/*
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/asm.h>

.section ".text.boot"
FUNCTION(_start)
    # set the default stack
    la      $sp, default_stack_top

    # zero out the bss section
    la      $t0, __bss_start
    la     $t1, __bss_end
0:
    sw      $zero, ($t0)
    addi    $t0, 4
    bne     $t0, $t1, 0b

    # args to main and call it
    li      $a0, 1
    li      $a1, 2
    li      $a2, 3
    li      $a3, 4
    jal     lk_main

    # should never return here
    b       .

.bss
.align 3
LOCAL_DATA(default_stack)
    .skip 4096
LOCAL_DATA(default_stack_top)

