#include <asm.h>

tmp                     .req x9
tmp2                    .req x10

.section .text.boot
FUNCTION(_start)
    ldr tmp, =__stack_end
    mov sp, tmp

    /* clear bss */
.L__do_bss:
    /* clear out the bss */
    /* NOTE: relies on __bss_start and __bss_end being 8 byte aligned */
    ldr     tmp, =__bss_start
    ldr     tmp2, =__bss_end
    sub     tmp2, tmp2, tmp
    cbz     tmp2, .L__bss_loop_done
.L__bss_loop:
    sub     tmp2, tmp2, #8
    str     xzr, [tmp], #8
    cbnz    tmp2, .L__bss_loop
.L__bss_loop_done:

    bl  lk_main
    b   .

.ltorg

.section .bss.prebss.stack
    .align 4
DATA(__stack)
    .skip 0x2000
DATA(__stack_end)

