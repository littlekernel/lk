/*
 * Copyright (c) 2008-2013 Travis Geiselbrecht
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
#include <arch/arm/cores.h>
#include <arch/arm/mmu.h>
#include <kernel/vm.h>

.section ".text.boot"
.globl _start
_start:
	b	platform_reset
	b	arm_undefined
	b	arm_syscall
	b	arm_prefetch_abort
	b	arm_data_abort
	b	arm_reserved
	b	arm_irq
	b	arm_fiq

.weak platform_reset
platform_reset:
	/* Fall through for the weak symbol */

.globl arm_reset
arm_reset:
	/* do some cpu setup */
#if ARM_WITH_CP15
	mrc		p15, 0, r0, c1, c0, 0
		/* XXX this is currently for arm926, revist with armv6 cores */
		/* new thumb behavior, low exception vectors, i/d cache disable, mmu disabled */
	bic		r0, r0, #(1<<15| 1<<13 | 1<<12)
	bic		r0, r0, #(1<<2 | 1<<1 | 1<<0)
#if ARM_ARCH_LEVEL < 6
		/* enable alignment faults on pre-ARMv6 hardware. On v6+,
		 * GCC is free to generate unaligned accesses.
		 */
	orr		r0, r0, #(1<<1)
#endif
	mcr		p15, 0, r0, c1, c0, 0
#endif

#if WITH_CPU_EARLY_INIT
	/* call platform/arch/etc specific init code */
	bl __cpu_early_init
#endif

#if WITH_KERNEL_VM
__relocate_start:
	/* see if we need to relocate to our proper location in physical memory */
	adr		r0, _start                           /* this emits sub r0, pc, #constant */
	ldr		r1, =(MEMBASE + KERNEL_LOAD_OFFSET)  /* calculate the binary's physical load address */
	subs	r12, r0, r1                          /* calculate the delta between where we're loaded and the proper spot */
	beq		.Lsetup_mmu

	/* we need to relocate ourselves to the proper spot */
	ldr		r2, =__data_end
	ldr		r3, =(KERNEL_BASE - MEMBASE)
	sub		r2, r3
	add		r2, r12

.Lrelocate_loop:
	ldr		r3, [r0], #4
	str		r3, [r1], #4
	cmp		r0, r2
	bne		.Lrelocate_loop

	/* we're relocated, jump to the right address */
	sub		pc, r12
	nop

__mmu_start:
.Lsetup_mmu:
	/* set up the mmu according to mmu_initial_mappings */

	/* calculate our physical to virtual offset */
	mov		r12, pc
	ldr		r1, =.Laddr1
.Laddr1:
	sub		r12, r1

	/* r12 now holds the offset from virtual to physical:
	 * virtual + r12 = physical */

	/* load the base of the translation table and clear the table */
	ldr		r0, =arm_kernel_translation_table
	add		r0, r12

	mov		r1, #0
	mov		r2, #0

	/* walk through all the entries in the translation table, setting them up */
0:
	str		r1, [r0, r2, lsl #2]
	add		r2, #1
	cmp		r2, #4096
	bne		0b

	/* load the address of the mmu_initial_mappings table and start processing */
	ldr		r1, =mmu_initial_mappings
	add		r1, r12

.Linitial_mapping_loop:
	ldmia	r1!, { r2-r6 }
		/* r2 = phys, r3 = virt, r4 = size, r5 = flags, r6 = name */

	/* mask all the addresses and sizes to 1MB boundaries */
	lsr		r2, #20  /* r2 = physical address / 1MB */
	lsr		r3, #20  /* r3 = virtual address / 1MB */
	lsr		r4, #20  /* r4 = size in 1MB chunks */

	/* if size == 0, end of list */
	cmp		r4, #0
	beq		.Linitial_mapping_done

	/* set up the flags */
	ldr		r6, =MMU_KERNEL_L1_PTE_FLAGS
	teq		r5, #MMU_INITIAL_MAPPING_FLAG_UNCACHED
	ldreq	r6, =MMU_INITIAL_MAP_STRONGLY_ORDERED
	beq		0f
	teq		r5, #MMU_INITIAL_MAPPING_FLAG_DEVICE
	ldreq	r6, =MMU_INITIAL_MAP_DEVICE
		/* r6 = mmu entry flags */

0:
	orr		r7, r6, r2, lsl #20
		/* r7 = phys addr | flags */

	/* store into appropriate translation table entry */
	str		r7, [r0, r3, lsl #2]

	/* loop until we're done */
	add		r2, #1
	add		r3, #1
	subs	r4, #1
	bne		0b

	b		.Linitial_mapping_loop

.Linitial_mapping_done:

	/* set up the mmu */

	/* Invalidate TLB */
	mov		r3, #0
	mcr		p15, 0, r3, c8, c7, 0
	isb

	/* Write 0 to TTBCR */
	mcr		p15, 0, r3, c2, c0, 2
	isb

	/* set cacheable attributes on translation walk */
	/* (SMP extensions) non-shareable, inner write-back write-allocate */
	orr		r0, #(1<<6 | 0<<1)
	/* outer write-back write-allocate */
	orr		r0, #(1<<3)

	/* Write ttbr with phys addr of the translation table */
	mcr		p15, 0, r0, c2, c0, 0
	isb

	/* Write DACR */
	mov		r3, #0x1
	mcr		p15, 0, r3, c3, c0, 0
	isb

	/* Read SCTLR into r3 */
	mrc		p15, 0, r3, c1, c0, 0

	/* Disable TRE/AFE */
	bic		r3, r3, #(1<<29 | 1<<28)

	/* Turn on the MMU */
	orr		r3, r3, #0x1

	/* Write back SCTLR */
	mcr		p15, 0, r3, c1, c0, 0
	isb

	/* Jump to virtual code address */
	ldr		pc, =1f
1:

	/* Invalidate TLB */
	mov		r3, #0
	mcr		p15, 0, r3, c8, c7, 0
	isb

#else
	/* see if we need to relocate */
	mov		r0, pc
	sub		r0, r0, #(.Laddr - _start)
.Laddr:
	ldr		r1, =_start
	cmp		r0, r1
	beq		.Lstack_setup

	/* we need to relocate ourselves to the proper spot */
	ldr		r2, =__data_end

.Lrelocate_loop:
	ldr		r3, [r0], #4
	str		r3, [r1], #4
	cmp		r1, r2
	bne		.Lrelocate_loop

	/* we're relocated, jump to the right address */
	ldr		r0, =.Lstack_setup
	bx		r0
#endif

	/* at this point we're running at our final location in virtual memory (if enabled) */
.Lstack_setup:
	/* set up the stack for irq, fiq, abort, undefined, system/user, and lastly supervisor mode */
	ldr		r2, =abort_stack_top

	cpsid	i,#0x12       /* irq */
	mov		sp, r2

	cpsid	i,#0x11       /* fiq */
	mov		sp, r2

	cpsid	i,#0x17       /* abort */
	mov		sp, r2

	cpsid	i,#0x1b       /* undefined */
	mov		sp, r2

	cpsid	i,#0x1f       /* system */
	mov		sp, r2

	cpsid	i,#0x13       /* supervisor */
	mov		sp, r2

	/* stay in supervisor mode from now on out */

	/* copy the initialized data segment out of rom if necessary */
	ldr		r0, =__data_start_rom
	ldr		r1, =__data_start
	ldr		r2, =__data_end

	cmp		r0, r1
	beq		.L__do_bss

.L__copy_loop:
	cmp		r1, r2
	ldrlt	r3, [r0], #4
	strlt	r3, [r1], #4
	blt		.L__copy_loop

.L__do_bss:
	/* clear out the bss */
	ldr		r0, =__bss_start
	ldr		r1, =_end
	mov		r2, #0
.L__bss_loop:
	cmp		r0, r1
	strlt	r2, [r0], #4
	blt		.L__bss_loop

	bl		lk_main
	b		.

.ltorg

.bss
.align 3
	/* the abort stack is for unrecoverable errors.
	 * also note the initial working stack is set to here.
	 * when the threading system starts up it'll switch to a new
	 * dynamically allocated stack, so we don't need it for very long
	 */
LOCAL_DATA(abort_stack)
	.skip 4096
LOCAL_DATA(abort_stack_top)

.data
.align 2

/* vim: set ts=4 sw=4 noexpandtab: */
