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
    mov  $1, %esi
0:
    xor  %eax, %eax
    lock cmpxchg  %esi, (%rdi)
    jz 1f
    pause
    jmp 0b
1:
    ret
END_FUNCTION(arch_spin_lock)

// int arch_spin_trylock(spin_lock_t *lock);
FUNCTION(arch_spin_trylock)
    mov  $1, %eax

    lock xchg  %eax, (%rdi)

    ret
END_FUNCTION(arch_spin_trylock)

// void arch_spin_unlock(spin_lock_t *lock);
FUNCTION(arch_spin_unlock)
    movl  $0, (%rdi)
    ret
END_FUNCTION(arch_spin_unlock)

#endif // WITH_SMP