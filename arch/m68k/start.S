/*
 * Copyright (c) 2021 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/asm.h>

.section .text.boot
FUNCTION(_start)
    // clear bss
    lea     __bss_start,%a0
    cmpal   #__bss_end,%a0
    beqs    1f
0:
    clrb    %a0@+
    cmpal   #__bss_end,%a0
    bnes    0b
1:

    movel   #_default_stack_top,%sp

    jsr     lk_main
    jmp     .

END_FUNCTION(_start)

.bss
.align 8
_default_stack_base:
    .skip 4096
_default_stack_top:

