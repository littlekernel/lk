#include <asm.h>

.section .text.boot
FUNCTION(_start)
    ldr x0, =__stack_end
    mov sp, x0

    bl  lk_main
    b   .

.section .bss.prebss.stack
    .align 4
DATA(__stack)
    .skip 0x2000
DATA(__stack_end)

