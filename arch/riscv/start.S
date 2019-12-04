/*
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/asm.h>
#include <arch/defines.h>
#include "config.h"

.section ".text.boot"
FUNCTION(_start)
.option push
.option norelax
    // set the global pointer
    la      gp, __global_pointer$
.option pop

    // if our hart isnt 0, trap the cpu
    csrr    t0, mhartid

    // set the default stack
    la      sp, default_stack_top
    // default stack locations for each hart:
    // LOW ------------ HIGH
    // [hart2][hart1][hart0]
    li      t1, ARCH_DEFAULT_STACK_SIZE
    mul     t1, t1, a0
    sub     sp, sp, t1

    // everyone stores zero in _boot_status
    la      t5, _boot_status
    sw      zero, (t5)

    bnez    t0, .Lsecondary_trap

#if ARCH_RISCV_TWOSEGMENT
    // copy preinitialized data from flash to memory
    la      t0, __data_start_rom
    la      t1, __data_start
    la      t2, __data_end
    beq     t0, t1, 1f

0:
    lw      t3, (t0)
    sw      t3, (t1)
    add     t0, t0, 4
    add     t1, t1, 4
    bne     t1, t2, 0b
#endif

    // zero bss
1:
    la      t0, __bss_start
    la      t1, __bss_end
0:
    sw      x0, (t0)
    add     t0, t0, 4
    bne     t0, t1, 0b

    // Release any other harts into riscv_secondary_entry
    fence   w, w
    add     t0, zero, 1
    sw      t0, (t5)

    // call main
    jal     lk_main

    // should never return here
    j       .

.Lsecondary_trap:
#if WITH_SMP
    // wait for _boot_status to be nonzero, then go into riscv_secondary_entry
    lw      t0, (t5)
    beqz    t0, .Lsecondary_trap
    jal     riscv_secondary_entry
#else
    wfi
    j       .
#endif

.bss
.align 4
LOCAL_DATA(default_stack)
    .skip ARCH_DEFAULT_STACK_SIZE * SMP_MAX_CPUS
LOCAL_DATA(default_stack_top)
LOCAL_DATA(_boot_status)
    .dword
