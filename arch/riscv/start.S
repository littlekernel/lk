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

#if RISCV_M_MODE
    // copy the hart id into a0 which we'll use later
    // supervisor mode should already have hart id in a0
    csrr    a0, mhartid
#endif

    // if the hart is too high, trap it
    li      t0, RISCV_MAX_HARTS
    ble     t0, a0, .Lhart_trap

    // set the default stack per cpu
    la      sp, default_stack_top
    // default stack locations for each hart:
    // LOW ------------ HIGH
    // [hart2][hart1][hart0]
    li      t1, ARCH_DEFAULT_STACK_SIZE
    mul     t1, t1, a0
    sub     sp, sp, t1

    // if our hart isnt RISCV_BOOT_HART, trap the cpu
    li      t2, RISCV_BOOT_HART
    bne     t2, a0, .Lsecondary_trap

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
    sw      zero, (t0)
    add     t0, t0, 4
    bne     t0, t1, 0b

#if WITH_SMP
    // Release any other harts into riscv_secondary_entry
    fence   w, w
    la      t1, _boot_status
    li      t0, 1
    sb      t0, (t1)
#endif

    // call into early C code to set up the percpu structure
    mv      s0, a0
    mv      s1, a1
    mv      s2, a2
    mv      s3, a3
    jal     riscv_configure_percpu_early
    mv      a0, s0
    mv      a1, s1
    mv      a2, s2
    mv      a3, s3

    // call main
    jal     lk_main

    // should never return here
    j       .

.Lsecondary_trap:
#if WITH_SMP
    // wait for _boot_status to be nonzero, then go into riscv_secondary_entry
    la      t5, _boot_status
    lb      t0, (t5)
    beqz    t0, .Lsecondary_trap

    // set the per cpu structure before getting into the secondary boot path
    jal     riscv_configure_percpu_early

    // bootstrap the secondary cpus
    jal     riscv_secondary_entry
#else
    wfi
    j       .
#endif

.Lhart_trap:
    // cpus with too high of a hart id go here and spin forever
    wfi
    j       .

.bss
.align 4
LOCAL_DATA(default_stack)
    .skip ARCH_DEFAULT_STACK_SIZE * RISCV_MAX_HARTS;
LOCAL_DATA(default_stack_top)

// put boot status in .data so it doesn't get paved over during BSS initialization
.data
LOCAL_DATA(_boot_status)
    .byte  0
