/*
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/asm.h>

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

.section ".text.boot"
FUNCTION(start)
    # set the default stack
    addik   r1, r0, default_stack_top

    # set up small data pointers
    addik   r2, r0, _SDATA2_START__
    addik   r13, r0, _SDATA_START__

    # set the processor mode to default
    mts     rmsr, r0

    # zero out bss sections
    addik   r5, r0, __bss_start
    addik   r6, r0, 0
    rsubik  r7, r5, __bss_end
    brlid   r15, memset
    nop

    # arguments to main
    addik   r5, r0, 1
    addik   r6, r0, 2
    addik   r7, r0, 3
    brlid   r15, lk_main
    addik   r8, r0, 4

    # shouldn't be here
    bri     .

FUNCTION(unhandled_exception)
    bri     .

.bss
.align 3
LOCAL_DATA(default_stack)
    .skip 4096
LOCAL_DATA(default_stack_top)
