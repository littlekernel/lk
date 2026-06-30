#include <lk/asm.h>

.section ".text.boot"
FUNCTION(_start)

    /* Setup a minimal stack pointer.
     * SPARC ABI requires at least 96 bytes of stack frame for window save area.
     */
    set     default_stack_top - 96, %sp
    set     0, %fp

    /* Clear BSS */
    set     __bss_start, %g1
    set     __bss_end, %g2
.Lclear_bss:
    cmp     %g1, %g2
    bge,a   .Lcall_main
    nop
    st      %g0, [%g1]
    add     %g1, 4, %g1
    ba      .Lclear_bss
    nop

.Lcall_main:
    call    lk_main
    nop

    /* Loop forever if we return */
.Lloop:
    ba      .Lloop
    nop
END_FUNCTION(_start)

.bss
.balign 8
LOCAL_DATA(default_stack)
    .skip 8192
LOCAL_DATA(default_stack_top)
