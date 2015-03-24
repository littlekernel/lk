/*
 * Copyright (c) 2015 Stefan Kristiansson
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <asm.h>
#include <arch/ops.h>
#include <arch/or1k/mmu.h>
#include <kernel/vm.h>

#define RED_ZONE            128
#define EXCEPTION_FRAME     (128 + RED_ZONE)

/* clobbers r9 and rd, result will be in rd */
#define get_va_to_pa_offs(rd) \
    l.movhi rd, hi(.+12)    ;\
    l.jal   .+8             ;\
     l.ori  rd, rd, lo(.+4) ;\
    l.sub   rd, rd, r9

/* clobbers r9 and rd, result will be in rd */
#define to_phys(sym, rd) \
    get_va_to_pa_offs(rd)   ;\
    l.movhi r9, hi(sym)     ;\
    l.ori   r9, r9, lo(sym) ;\
    l.sub   rd, r9, rd

.macro exception_entry
#if WITH_KERNEL_VM
    l.sw    0(r0), r31
    l.sw    4(r0), r9
    get_va_to_pa_offs(r31)
    l.sub   r1, r1, r31
    l.lwz   r9, 4(r0)
#endif
    l.addi  r1, r1, -EXCEPTION_FRAME
    l.sw    0(r1), r2
    l.sw    4(r1), r3
    l.sw    8(r1), r4
    l.sw    12(r1), r5
    l.sw    16(r1), r6
    l.sw    20(r1), r7
    l.sw    24(r1), r8
    l.sw    28(r1), r9
    l.sw    32(r1), r10
    l.sw    36(r1), r11
    l.sw    40(r1), r12
    l.sw    44(r1), r13
    l.sw    48(r1), r14
    l.sw    52(r1), r15
    l.sw    56(r1), r16
    l.sw    60(r1), r17
    l.sw    64(r1), r18
    l.sw    68(r1), r19
    l.sw    72(r1), r20
    l.sw    76(r1), r21
    l.sw    80(r1), r22
    l.sw    84(r1), r23
    l.sw    88(r1), r24
    l.sw    92(r1), r25
    l.sw    96(r1), r26
    l.sw    100(r1), r27
    l.sw    104(r1), r28
    l.sw    108(r1), r29
    l.sw    112(r1), r30
    l.mfspr r3, r0, OR1K_SPR_SYS_EPCR_ADDR(0)
    l.sw    120(r1), r3
    l.mfspr r3, r0, OR1K_SPR_SYS_ESR_ADDR(0)
    l.sw    124(r1), r3
#if WITH_KERNEL_VM
    l.add   r1, r1, r31
    l.lwz   r31, 0(r0)

    /* enable dmmu and immu */
    l.mfspr r9, r0, OR1K_SPR_SYS_SR_ADDR
    l.ori   r9, r9, OR1K_SPR_SYS_SR_DME_MASK | OR1K_SPR_SYS_SR_IME_MASK
    l.mtspr r0, r9, OR1K_SPR_SYS_ESR_ADDR(0)

    l.movhi r9, hi(.+16)
    l.ori   r9, r9, lo(.+12)
    l.mtspr r0, r9, OR1K_SPR_SYS_EPCR_ADDR(0)
    l.rfe
#endif
    l.sw    116(r1), r31
.endm

.section ".vectors", "ax"
.org 0x100
.global _reset
_reset:
    l.jal   start
     l.nop

.org 0x200
bus_error_exception:
    exception_entry
    l.mfspr r4, r0, OR1K_SPR_SYS_EEAR_ADDR(0)
    l.jal   or1k_busfault_handler
     l.ori  r3, r1, 0
    l.j return_from_exception
     l.nop

.org 0x300
data_pagefault_exception:
    exception_entry
    l.mfspr r4, r0, OR1K_SPR_SYS_EEAR_ADDR(0)
    l.jal   or1k_data_pagefault_handler
     l.ori  r3, r1, 0
    l.j return_from_exception
     l.nop

.org 0x400
instruction_pagefault_exception:
    exception_entry
    l.mfspr r4, r0, OR1K_SPR_SYS_EEAR_ADDR(0)
    l.jal   or1k_instruction_pagefault_handler
     l.ori  r3, r1, 0
    l.j return_from_exception
     l.nop

.org 0x500
tick_timer_exception:
    exception_entry
    l.jal   or1k_tick
     l.nop
    l.j return_from_exception
     l.nop

.org 0x600
alignment_exception:
    exception_entry
    l.mfspr r4, r0, OR1K_SPR_SYS_EEAR_ADDR(0)
    l.jal   or1k_alignment_handler
     l.ori  r3, r1, 0
    l.j return_from_exception
     l.nop

.org 0x700
illegal_instruction_exception:
    exception_entry
    l.mfspr r4, r0, OR1K_SPR_SYS_EEAR_ADDR(0)
    l.jal   or1k_illegal_instruction_handler
     l.ori  r3, r1, 0
    l.j return_from_exception
     l.nop

.org 0x800
external_interrupt_exception:
    exception_entry
    l.jal   or1k_irq
     l.nop
    l.j return_from_exception
     l.nop

.org 0x900
dtlb_miss_exception:
#if WITH_KERNEL_VM
    l.sw    0(r0), r3
    l.sw    4(r0), r4
    l.sw    8(r0), r9

    to_phys(or1k_kernel_translation_table, r3)
    l.mfspr r4, r0, OR1K_SPR_SYS_EEAR_ADDR(0)
    /* l1 index */
    l.srli  r9, r4, 24
    l.slli  r9, r9, 2

    l.add   r3, r3, r9

    l.lwz   r3, 0(r3) /* l1 entry */
    l.andi  r9, r3, OR1K_MMU_PG_PRESENT
    l.sfnei r9, OR1K_MMU_PG_PRESENT
    l.bf    dtlb_miss_fault
     l.andi r9, r3, OR1K_MMU_PG_L

    l.sfeqi r9, OR1K_MMU_PG_L
    /* l2_index */
    l.srli  r4, r4, 13
    l.bf    1f
     l.andi r4, r4, 0x7ff
    l.slli  r4, r4, 2
    l.addi  r9, r0, 0xffffe000 /* PAGE_SIZE-1 */
    l.and   r9, r3, r9
    l.add   r9, r9, r4
    l.j 2f
     l.lwz  r9, 0(r9) /* l2 entry */

/* use bits [23:13] from EEAR */
1:  l.slli  r4, r4, 13
    l.or    r9, r3, r4

2:  l.ori   r3,r0,0xf351    /* sw emulation of dmmupr */
    l.srli  r4,r9,4         /* get PP Index * 4 */
    l.andi  r4,r4,0xc       /* mask everything but PPI (without X) (& 0b01100)*/
    l.srl   r3,r3,r4        /* get protection bits from "dmmupr" */
    /*
    * The protection bits are unconvienently the "wrong" way in DMMUPR
    * compared to DTLBR (UWE|URE|SWE|SRE vs SWE|SRE|UWE|URE), so we have
    * to swap their places...
    */
    l.andi  r4,r3,0x3       /* SWE|SRE */
    l.slli  r4,r4,8         /* 1:0 -> 9:8 */
    l.andi  r3,r3,0xc       /* UWE|URE */
    l.slli  r3,r3,4         /* 3:2 -> 7:6 */
    l.or    r3,r3,r4

    l.addi  r4,r0,0xffffe03f /* protection bit mask */
    l.and   r4,r9,r4        /* apply the mask */
    l.or    r9,r4,r3        /* apply protection bits */

    l.mfspr r3, r0, OR1K_SPR_SYS_DMMUCFGR_ADDR
    l.slli  r3, r3, 31-OR1K_SPR_SYS_DMMUCFGR_NTS_MSB
    l.srli  r3, r3, 31-OR1K_SPR_SYS_DMMUCFGR_NTS_LSB
    l.ori   r4, r0, 1
    l.sll   r3, r4, r3
    l.addi  r3, r3, -1
    l.mfspr r4, r0, OR1K_SPR_SYS_EEAR_ADDR(0)
    l.srli  r4, r4, 13
    l.and   r3, r4, r3
    l.mtspr r3, r9, OR1K_SPR_DMMU_DTLBW_TR_ADDR(0,0)
    l.slli  r4, r4, 13
    l.ori   r4, r4, OR1K_SPR_DMMU_DTLBW_MR_V_MASK
    l.mtspr r3, r4, OR1K_SPR_DMMU_DTLBW_MR_ADDR(0,0)

    l.lwz   r3, 0(r0)
    l.lwz   r4, 4(r0)
    l.lwz   r9, 8(r0)
    l.rfe
#endif /* WITH_KERNEL_VM */

dtlb_miss_fault:
    l.lwz   r3, 0(r0)
    l.lwz   r4, 4(r0)
    l.j     data_pagefault_exception
     l.lwz   r9, 8(r0)

.org 0xa00
itlb_miss_exception:
#if WITH_KERNEL_VM
    l.sw    0(r0), r3
    l.sw    4(r0), r4
    l.sw    8(r0), r9

    to_phys(or1k_kernel_translation_table, r3)
    l.mfspr r4, r0, OR1K_SPR_SYS_EEAR_ADDR(0)
    /* l1 index */
    l.srli  r9, r4, 24
    l.slli  r9, r9, 2

    l.add   r3, r3, r9
    l.lwz   r3, 0(r3) /* l1 entry */

    l.andi  r9, r3, OR1K_MMU_PG_PRESENT
    l.sfnei r9, OR1K_MMU_PG_PRESENT
    l.bf    itlb_miss_fault
     l.andi r9, r3, OR1K_MMU_PG_L
    l.sfeqi r9, OR1K_MMU_PG_L
    /* l2 index */
    l.srli  r4, r4, 13
    l.bf    1f
     l.andi r4, r4, 0x7ff

    l.slli  r4, r4, 2
    l.addi  r9, r0, 0xffffe000 /* PAGE_SIZE-1 */
    l.and   r9, r3, r9
    l.add   r9, r9, r4
    l.j 2f
     l.lwz  r9, 0(r9) /* l2 entry */

    /* use bits [23:13] from EEAR */
1:  l.slli  r4, r4, 13
    l.or    r9, r3, r4

2:  l.ori   r3, r0, 0xd00   /* sw emulation of immupr */
    l.srli  r4, r9, 5       /* get PP Index * 2 */
    l.andi  r4, r4, 0xa     /* mask everything but PPI (without W) (& 0b1010)*/
    l.srl   r3, r3, r4      /* get protection bits from "immupr" */
    l.andi  r3, r3, 0x3     /* mask everything else out */
    l.slli  r3, r3, 6       /* and put them in their spot */
    l.addi  r4, r0, 0xffffe03f /* protection bit mask */
    l.and   r4, r9, r4      /* apply the mask */
    l.or    r9, r4, r3      /* apply protection bits */

    l.mfspr r3, r0, OR1K_SPR_SYS_IMMUCFGR_ADDR
    l.slli  r3, r3, 31-OR1K_SPR_SYS_IMMUCFGR_NTS_MSB
    l.srli  r3, r3, 31-OR1K_SPR_SYS_IMMUCFGR_NTS_LSB
    l.ori   r4, r0, 1
    l.sll   r3, r4, r3
    l.addi  r3, r3, -1
    l.mfspr r4, r0, OR1K_SPR_SYS_EEAR_ADDR(0)
    l.srli  r4, r4, 13
    l.and   r3, r4, r3
    l.mtspr r3, r9, OR1K_SPR_IMMU_ITLBW_TR_ADDR(0,0)

    l.slli  r4, r4, 13
    l.ori   r4, r4, OR1K_SPR_IMMU_ITLBW_MR_V_MASK
    l.mtspr r3, r4, OR1K_SPR_IMMU_ITLBW_MR_ADDR(0,0)

    l.lwz   r3, 0(r0)
    l.lwz   r4, 4(r0)
    l.lwz   r9, 8(r0)
    l.rfe
#endif /* WITH_KERNEL_VM */

itlb_miss_fault:
    l.lwz   r3, 0(r0)
    l.lwz   r4, 4(r0)
    l.j     instruction_pagefault_exception
     l.lwz   r9, 8(r0)

.org 0xb00
range_exception:
    exception_entry
    l.ori   r4, r0, 0xb00
    l.jal   or1k_unhandled_exception
     l.ori  r3, r1, 0
    l.j return_from_exception
     l.nop

.org 0xc00
syscall_exception:
    exception_entry
    l.jal   or1k_syscall_handler
     l.ori  r3, r1, 0
    l.j return_from_exception
     l.nop

.org 0xd00
fpu_exception:
    exception_entry
    l.ori   r4, r0, 0xd00
    l.jal   or1k_unhandled_exception
     l.ori  r3, r1, 0
    l.j return_from_exception
     l.nop

.org 0xe00
trap_exception:
    exception_entry
    l.ori   r4, r0, 0xe00
    l.jal   or1k_unhandled_exception
     l.ori  r3, r1, 0
    l.j return_from_exception
     l.nop

.section ".text.boot"
FUNCTION(start)
    /* set stack pointer to point at top of default stack */
    l.movhi r1, hi(default_stack_top)
    l.ori   r1, r1, lo(default_stack_top)

#if WITH_KERNEL_VM
    /* invalidate tlbs */
    l.ori   r3, r0, OR1K_SPR_DMMU_DTLBW_MR_ADDR(0, 0)
    l.ori   r4, r0, OR1K_SPR_IMMU_ITLBW_MR_ADDR(0, 0)
    l.addi  r6, r0, 3 /* Maximum number of ways - 1 */

1:  l.addi  r5, r0, 127 /* Maximum number of sets - 1 */
2:  l.mtspr r3, r0, 0x0
    l.mtspr r4, r0, 0x0

    l.addi  r3, r3, 1
    l.addi  r4, r4, 1
    l.sfeq  r5, r0
    l.bnf   2b
     l.addi r5, r5, -1

    l.addi  r3, r3, 128
    l.addi  r4, r4, 128

    l.sfeq  r6, r0
    l.bnf   1b
     l.addi r6, r6, -1

    /* setup initial mappings */
    get_va_to_pa_offs(r3)
    l.movhi r4, hi(or1k_kernel_translation_table)
    l.ori   r4, r4, lo(or1k_kernel_translation_table)
    l.sub   r4, r4, r3 /* to phys */
    l.movhi r5, hi(mmu_initial_mappings)
    l.ori   r5, r5, lo(mmu_initial_mappings)
    l.sub   r5, r5, r3 /* to phys */

    /* clear the translation table */
    l.addi  r3, r4, 255*4
0:  l.sw    0(r3), r0
    l.sfeq  r3, r4
    l.bnf   0b
     l.addi r3, r3, -4

1:  l.lwz   r6, 0(r5) /* phys */
    l.lwz   r7, 4(r5) /* virt */
    l.lwz   r8, 8(r5) /* size */
    l.lwz   r9, 12(r5) /* flags */
    l.lwz   r10, 16(r5) /* name */
    l.addi  r5, r5, 20

    /* divide with 16MB */
    l.srli  r6, r6, 24
    l.srli  r7, r7, 24
    l.srli  r8, r8, 24

    l.sfeqi r8, 0
    l.bf    .Linitial_mapping_done
     l.nop

2:  l.slli  r3, r7, 2
    l.add   r3, r4, r3
    l.slli  r10, r6, 24
    l.ori   r10, r10, OR1K_MMU_PG_PRESENT | OR1K_MMU_PG_X | OR1K_MMU_PG_W | OR1K_MMU_PG_L
    l.sfeqi r9, MMU_INITIAL_MAPPING_FLAG_UNCACHED
    l.bf    3f
     l.sfeqi r9, MMU_INITIAL_MAPPING_FLAG_DEVICE
    l.bnf   4f
     l.nop
3:  l.ori   r10, r10, OR1K_MMU_PG_CI
4:  l.sw    0(r3), r10
    l.addi  r6, r6, 1
    l.addi  r8, r8, -1
    l.sfeqi r8, 0
    l.bnf   2b
     l.addi r7, r7, 1

    l.j 1b
     l.nop

.Linitial_mapping_done:
    /* enable mmu */
    l.mfspr r3, r0, OR1K_SPR_SYS_SR_ADDR
    l.ori   r3, r3, OR1K_SPR_SYS_SR_DME_MASK | OR1K_SPR_SYS_SR_IME_MASK
    l.mtspr r0, r3, OR1K_SPR_SYS_ESR_ADDR(0)
    /* setup pc to use virtual addresses */
    l.movhi r3, hi(.+16)
    l.ori   r3, r3, lo(.+12)
    l.mtspr r0, r3, OR1K_SPR_SYS_EPCR_ADDR(0)
    l.rfe
#endif

    /* invalidate and enable caches */
    l.jal   arch_invalidate_cache_all
    l.nop
    l.jal   arch_enable_cache
    l.ori  r3, r0, UCACHE

    /* clear bss */
    l.movhi r3, hi(__bss_start)
    l.ori   r3, r3, lo(__bss_start)
    l.movhi r4, hi(__bss_end)
    l.ori   r4, r4, lo(__bss_end)
1:  l.sw    0(r3), r0
    l.sfltu r3, r4
    l.bf    1b
     l.addi r3, r3, 4

    /* arguments to main */
    l.ori   r3, r0, 1
    l.ori   r4, r0, 2
    l.ori   r5, r0, 3
    l.jal   lk_main
     l.ori  r6, r0, 4

    /* shouldn't happen, but loop if it does */
    l.j 0
     l.nop

FUNCTION(return_from_exception)
    l.lwz   r3, 120(r1)
    l.mtspr r0, r3, OR1K_SPR_SYS_EPCR_BASE
    l.lwz   r3, 124(r1)
    l.mtspr r0, r3, OR1K_SPR_SYS_ESR_BASE
    l.lwz   r2, 0(r1)
    l.lwz   r3, 4(r1)
    l.lwz   r4, 8(r1)
    l.lwz   r5, 12(r1)
    l.lwz   r6, 16(r1)
    l.lwz   r7, 20(r1)
    l.lwz   r8, 24(r1)
    l.lwz   r9, 28(r1)
    l.lwz   r10, 32(r1)
    l.lwz   r11, 36(r1)
    l.lwz   r12, 40(r1)
    l.lwz   r13, 44(r1)
    l.lwz   r14, 48(r1)
    l.lwz   r15, 52(r1)
    l.lwz   r16, 56(r1)
    l.lwz   r17, 60(r1)
    l.lwz   r18, 64(r1)
    l.lwz   r19, 68(r1)
    l.lwz   r20, 72(r1)
    l.lwz   r21, 76(r1)
    l.lwz   r22, 80(r1)
    l.lwz   r23, 84(r1)
    l.lwz   r24, 88(r1)
    l.lwz   r25, 92(r1)
    l.lwz   r26, 96(r1)
    l.lwz   r27, 100(r1)
    l.lwz   r28, 104(r1)
    l.lwz   r29, 108(r1)
    l.lwz   r30, 112(r1)
    l.lwz   r31, 116(r1)
    l.addi  r1, r1, EXCEPTION_FRAME
    l.rfe

.section ".bss"
.align 8
LOCAL_DATA(default_stack)
.skip 8192
LOCAL_DATA(default_stack_top)
