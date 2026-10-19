/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

/*
 * Position independent user mode stubs, copied into a user page by
 * arch/test/uspace.cpp. Each one runs into an exception that the test's hooks
 * catch, and spins if it ever comes back; the spin stub only spins, until a
 * kill ends it.
 */

#if ARCH_ARM64 || (ARCH_RISCV && RISCV_S_MODE && RISCV_MMU)

.section .rodata
.balign 16

/* make a syscall with a recognizable argument */
.global uspace_stub_syscall_start
uspace_stub_syscall_start:
#if ARCH_ARM64
    movz    x0, #0x1234
    svc     #0
1:
    b       1b
#else
.option push
.option norelax
    li      a0, 0x1234
    ecall
1:
    j       1b
.option pop
#endif
.global uspace_stub_syscall_end
uspace_stub_syscall_end:

.balign 16

/* read from the bottom of the kernel's address space */
.global uspace_stub_fault_start
uspace_stub_fault_start:
#if ARCH_ARM64
    ldr     x0, 2f
    ldr     x0, [x0]
1:
    b       1b
    .balign 8
2:
    .quad   KERNEL_ASPACE_BASE
#else
.option push
.option norelax
    lla     t0, 2f      /* auipc + addi, never gp relative */
    ld      t0, (t0)
    ld      a0, (t0)
1:
    j       1b
    .balign 8
2:
    .dword  KERNEL_ASPACE_BASE
.option pop
#endif
.global uspace_stub_fault_end
uspace_stub_fault_end:

.balign 16

/* store to the address the test patches into the literal, then make the
 * syscall, so a store that goes through ends in the syscall hook */
.global uspace_stub_store_start
uspace_stub_store_start:
#if ARCH_ARM64
    ldr     x0, uspace_stub_store_target
    movz    x1, #0x5678
    str     x1, [x0]
    movz    x0, #0x1234
    svc     #0
1:
    b       1b
    .balign 8
#else
.option push
.option norelax
    lla     t0, uspace_stub_store_target
    ld      t0, (t0)
    li      t1, 0x5678
    sd      t1, (t0)
    li      a0, 0x1234
    ecall
1:
    j       1b
    .balign 8
.option pop
#endif
.global uspace_stub_store_target
uspace_stub_store_target:
    .quad   0
.global uspace_stub_store_end
uspace_stub_store_end:

.balign 16

/* jump to the address the test patches into the literal */
.global uspace_stub_jump_start
uspace_stub_jump_start:
#if ARCH_ARM64
    ldr     x0, uspace_stub_jump_target
    br      x0
1:
    b       1b
    .balign 8
#else
.option push
.option norelax
    lla     t0, uspace_stub_jump_target
    ld      t0, (t0)
    jr      t0
1:
    j       1b
    .balign 8
.option pop
#endif
.global uspace_stub_jump_target
uspace_stub_jump_target:
    .quad   0
.global uspace_stub_jump_end
uspace_stub_jump_end:

.balign 16

/* spin with no trap of its own: only an interrupt brings it back to the kernel */
.global uspace_stub_spin_start
uspace_stub_spin_start:
#if ARCH_ARM64
1:
    b       1b
#else
1:
    j       1b
#endif
.global uspace_stub_spin_end
uspace_stub_spin_end:

#endif
