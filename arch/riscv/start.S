/*
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/asm.h>
#include <arch/defines.h>
#include <arch/riscv/asm.h>
#include <arch/riscv/mmu.h>
#include "config.h"

.section ".text.boot"
FUNCTION(_start)
.option push
.option norelax
    // set the global pointer
    lla     gp, __global_pointer$
.option pop

#if RISCV_M_MODE
    // copy the hart id into a0 which we'll use later
    // supervisor mode should already have hart id in a0
    csrr    a0, mhartid
#endif

    // cpu lottery: whoever sets this first gets to be cpu 0
    lla     t0, _boot_lottery
    li      t1, 1
    amoadd.w a2, t1, (t0)

    // a2 now holds the logical cpu number. a2 is used because it is
    // the first unused argument register on SBI based systems,
    // which seem to use a0 and a1.

    // if this cpu is out of range, trap it
    li      t0, SMP_MAX_CPUS
    ble     t0, a2, hart_trap

    // set the default stack per cpu
    lla     sp, default_stack_top
    // default stack locations for each cpu:
    // LOW ------------ HIGH
    // [cpu2][cpu1][cpu0]
    li      t1, ARCH_DEFAULT_STACK_SIZE
    mul     t1, t1, a2
    sub     sp, sp, t1

    // if we aren't cpu 0, go hang out in secondary cpu purgatory for now
    bne     a2, zero, secondary_trap

#if ARCH_RISCV_TWOSEGMENT
    // copy preinitialized data from flash to memory
    lla     t0, __data_start_rom
    lla     t1, __data_start
    lla     t2, __data_end
    beq     t0, t1, 1f

0:
    LDR     t3, (t0)
    STR     t3, (t1)
    add     t0, t0, RISCV_XLEN_BYTES
    add     t1, t1, RISCV_XLEN_BYTES
    bne     t1, t2, 0b
#endif

    // zero bss
1:
    lla     t0, __bss_start
    lla     t1, __bss_end
    beq     t0, t1, 1f
0:
    STR     zero, (t0)
    add     t0, t0, RISCV_XLEN_BYTES
    bne     t0, t1, 0b
1:

#if WITH_SMP
    // Save a copy of _start in physical space. This is later used
    // as the entry point for secondary cpus.
    lla     t0, _start
    STR     t0, (_start_physical), t1
#endif

#if RISCV_MMU
    call    _mmu_init
#endif

#if WITH_SMP
    // Release any other harts into riscv_secondary_entry
    fence   w, w
    li      t0, 1
    sb      t0, (_boot_status), t1
    fence
#endif

    // call into early C code to set up the percpu structure
    mv      s0, a0
    mv      s1, a1
    mv      s2, a2
    mv      s3, a3
    call    riscv_configure_percpu_early
    mv      a0, s0
    mv      a1, s1
    mv      a2, s2
    mv      a3, s3

    // call main
    call    lk_main

    // should never return here
    j       .
END_FUNCTION(_start)

LOCAL_FUNCTION(secondary_trap)
#if WITH_SMP
    // wait for _boot_status to be nonzero, then go into riscv_secondary_entry
    lb      t0, (_boot_status)
    beqz    t0, secondary_trap

    // we've been released by the main cpu and/or we've been booted after the
    // system has been running a while.

#if RISCV_MMU
    // enable the mmu on this core
    call    .Lenable_mmu
#endif

    // a0 == hart id
    // a2 == assigned cpu id (may not be the same)

    // set the per cpu structure before getting into the secondary boot path
    call    riscv_configure_percpu_early

    // bootstrap the secondary cpus
    call    riscv_secondary_entry
#endif
    // fallthrough if either no SMP or riscv_secondary_entry returns
END_FUNCTION(secondary_trap)

LOCAL_FUNCTION(hart_trap)
    // cpus with too high of a hart id go here and spin forever
    wfi
    j       hart_trap
END_FUNCTION(hart_trap)

#if RISCV_MMU
    // initialize the kernel page tables
    // for all MMU versions, identity map some amount of memory near 0 and
    // the same amount at the bottom of the kernel's address space
LOCAL_FUNCTION(_mmu_init)
    lla     t0, trampoline_pgtable

    // store the physical address of the pgtable for future use
    sd      t0, (trampoline_pgtable_phys), t1

    // do the same for the main kernel pgtable
    lla     t2, kernel_pgtable
    sd      t2, (kernel_pgtable_phys), t1

    // and the 2nd level tables
    lla     t2, kernel_l2_pgtable
    sd      t2, (kernel_l2_pgtable_phys), t1

    // compute kernel pgtable pointer (index 256)
    addi    t1, t0, (8 * 128)
    addi    t1, t1, (8 * 128)

    // page table entry: address 0, A, D, G, XWR, V
    li      t2, (0 | (1<<7) | (1<<6) | (1<<5) | (1<<3) | (1<<2) | (1<<1) | (1<<0))

    // num interations and increment count
#if RISCV_MMU == 48 || RISCV_MMU == 39
    // RV48: map the first 512GB of the physical address space at the
    // bottom of the kernel address space using a single terapage
    // RV39: map the first 64GB of the physical address space at the
    // bottom of the kernel address space using 64 1GB gigapages
    li      t3, RISCV_MMU_PHYSMAP_PAGE_COUNT
    li      t4, (RISCV_MMU_PHYSMAP_PAGE_SIZE >> 2)
#else
#error implement sv32
#endif

    // loop, writing t3 entries out and incrementing by t4 address.
    // write both to t0 (index 0 of the kernel page table) and
    // t1 (starting index of kernel space)
0:
    sd      t2, (t1)
    sd      t2, (t0)
    add     t2, t2, t4
    addi    t0, t0, 8
    addi    t1, t1, 8
    addi    t3, t3, -1
    bnez    t3, 0b

    // ensure it's written out
    fence   w,w

.Lenable_mmu:
    // set the satp register and enable the mmu
    // ASID 0, trampoline_pgtable address
    lla     t0, trampoline_pgtable
    srli    t1, t0, 12
#if RISCV_MMU == 48
    li      t2, (RISCV_SATP_MODE_SV48 << RISCV_SATP_MODE_SHIFT)
#elif RISCV_MMU == 39
    li      t2, (RISCV_SATP_MODE_SV39 << RISCV_SATP_MODE_SHIFT)
#else
#error implement
#endif
    or      t1, t1, t2
    csrw    satp, t1

    // global tlb fence
    sfence.vma  zero, zero

    // mmu is initialized and we're running out of an identity physical map

    // save the physical address of .Lhigh
    lla     t1, .Lhigh

    // bounce to the high address
    ld      t0, (.Lhigh_addr)
    jr      t0

    // the full virtual address of the .Lhigh label
.Lhigh_addr:
    .quad   .Lhigh
.Lhigh:

    // we're now running at the high virtual address
    // compute the delta between the old physical and newer high addresses
    sub     t0, t0, t1

    // fix up the gp, stack pointer, and return address
    add     gp, gp, t0
    add     sp, sp, t0
    add     ra, ra, t0
    ret
END_FUNCTION(_mmu_init)
#endif // RISCV_MMU

.bss
.align 4
LOCAL_DATA(default_stack)
    .skip ARCH_DEFAULT_STACK_SIZE * SMP_MAX_CPUS
LOCAL_DATA(default_stack_top)

// put boot status in .data so it doesn't get paved over during BSS initialization
.data
LOCAL_DATA(_boot_status)
    .byte  0

.align 2
LOCAL_DATA(_boot_lottery)
    .word  0
