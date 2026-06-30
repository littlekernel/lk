#include <lk/asm.h>

/* void sparc_context_switch(
    struct sparc_context_switch_frame *oldcs,
    struct sparc_context_switch_frame *newcs); */
FUNCTION(sparc_context_switch)
    /* Stub context switch: just return */
    retl
    nop
END_FUNCTION(sparc_context_switch)
