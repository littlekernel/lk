#include <lk/asm.h>


// int call_with_stack(void *stack, int (*fp)(), int param1, int param2);
FUNCTION(call_with_stack)
sub     sp,sp,16
stp     fp, lr, [sp]
mov     fp, sp

sub     x0,x0,16
mov     x6,sp
str     x6,[x0]
mov     sp,x0
mov     x5,x1
mov     x0,x2
mov     x1,x3
blr     x5
ldr     x6,[sp]
mov     sp,x6

ldp     fp, lr, [sp]
add     sp,sp,16
ret     lr
