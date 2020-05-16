/*
 * Copyright (c) 2009 Corey Tabaka
 * Copyright (c) 2020 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/asm.h>

.text

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

#if X86_LEGACY
/* In legacy mode (compiled for i386) the use of builtin atomics in the compiler
 * ends up emitting calls to various routines that are expected to be provided
 * by an atomic library. Implement them here.
 *
 * Function signatures from https://gcc.gnu.org/wiki/Atomic/GCCMM/LIbrary
 */

/*
 * I4  __atomic_fetch_add_4  (I4 *mem, I4 val, int model)
 */
FUNCTION(__atomic_fetch_add_4)
    // save old irq disable state and disable irqs
    pushf
    cli

    mov 8(%esp), %edx       // mem
    mov 12(%esp), %ecx      // val
    mov (%edx),%eax         // edx = *mem
    add %eax,%ecx           // ecx = edx + val
    mov %ecx,(%edx)         // *mem = ecx
    // eax holds the old value

    // restore old irq state
    popf
    ret

#endif

