/*
 * Copyright (c) 2009 Corey Tabaka
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/asm.h>

.text

/* int _atomic_and(int *ptr, int val); */
FUNCTION(_atomic_and)
    movl 4(%esp), %edx
    movl (%edx), %eax
0:
    movl %eax, %ecx
    andl 8(%esp), %ecx
    lock
    cmpxchgl %ecx, (%edx)
    jnz 1f                  /* static prediction: branch forward not taken */
    ret
1:
    jmp 0b


/* int _atomic_or(int *ptr, int val); */
FUNCTION(_atomic_or)
movl 4(%esp), %edx
    movl (%edx), %eax
0:
    movl %eax, %ecx
    orl 8(%esp), %ecx
    lock
    cmpxchgl %ecx, (%edx)
    jnz 1f                  /* static prediction: branch forward not taken */
    ret
1:
    jmp 0b

/* void arch_idle(); */
FUNCTION(arch_idle)
    pushf
    popl %eax
    andl $0x200, %eax
    test %eax, %eax
    je 1f                   /* don't halt if local interrupts are disabled */
    hlt
1:
    ret

