#include <asm.h>

.section .text.boot
FUNCTION(_start)
    ldr x0, =__stack_end
    mov sp, x0

    /* clear bss */
.L__do_bss:
    /* clear out the bss */
    /* NOTE: relies on __bss_start and __bss_end being 8 byte aligned */
    ldr     x0, =__bss_start
    ldr     x1, =__bss_end
    mov     x2, #0
    sub     x1, x1, x0
    cbz     x1, .L__bss_loop_done
.L__bss_loop:
    sub     x1, x1, #8
    str     x2, [x0], #8
    cbnz    x1, .L__bss_loop
.L__bss_loop_done:

    bl  lk_main
    b   .

.ltorg

.section .bss.prebss.stack
    .align 4
DATA(__stack)
    .skip 0x2000
DATA(__stack_end)

