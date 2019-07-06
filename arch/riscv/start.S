/*
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/asm.h>

#if 0
.section ".vectors", "ax"
.globl _vectab
_vectab:
/* vector table here */
    # start vector
    brai start
    # user exception
    brai unhandled_exception
    # interrupt
    brai microblaze_irq
    # break
    brai unhandled_exception
    # hardware exception
    brai unhandled_exception

    # reserved for future
.fill (0x50 - 0x28)
#endif

.section ".text.boot"
FUNCTION(_start)
.option push
.option norelax
    # set the global pointer
    la      gp, __global_pointer$
.option pop

    # set the default stack
    la      sp, default_stack_top

#if 1
    # copy preinitialized data from flash to memory
    la      t0, __data_start_rom
    la      t1, __data_start
    la      t2, __data_end
    beq     t0, t1, 1f

0:
    lw      t3, (t0)
    sw      t3, (t1)
    add     t0, t0, 4
    add     t1, t1, 4
    bne     t1, t2, 0b
#endif

    # zero bss
1:
    la      t0, __bss_start
    la      t1, __bss_end
0:
    sw      x0, (t0)
    add     t0, t0, 4
    bne     t0, t1, 0b

    # call main
    jal     lk_main

    # should never return here
    j       .

.bss
.align 4
LOCAL_DATA(default_stack)
    .skip 1024
LOCAL_DATA(default_stack_top)
