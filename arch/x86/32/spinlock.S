/*
 * Copyright (c) 2025 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/asm.h>

#if WITH_SMP

// void arch_spin_lock(spin_lock_t *lock);
FUNCTION(arch_spin_lock)
    mov  4(%esp), %ecx

    mov  $1, %edx
0:
    xor  %eax, %eax
    lock cmpxchg  %edx, (%ecx)
    jz 1f
    pause
    jmp 0b
1:
    ret
END_FUNCTION(arch_spin_lock)

// int arch_spin_trylock(spin_lock_t *lock);
FUNCTION(arch_spin_trylock)
    mov  4(%esp), %ecx

    mov  $1, %eax
    lock xchg  %eax, (%ecx)

    ret
END_FUNCTION(arch_spin_trylock)

// void arch_spin_unlock(spin_lock_t *lock);
FUNCTION(arch_spin_unlock)
    mov   4(%esp), %ecx
    movl  $0, (%ecx)
    ret
END_FUNCTION(arch_spin_unlock)

#endif // WITH_SMP