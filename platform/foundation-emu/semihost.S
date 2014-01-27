#include <asm.h>

FUNCTION(semihost_call)
    hlt 0xf000
    ret

