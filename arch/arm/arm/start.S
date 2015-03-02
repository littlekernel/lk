/*
 * Copyright (c) 2008-2014 Travis Geiselbrecht
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

/* the delta of the virtual address space to physical address space */
/* ie: virtual = 0xc0000000, physical = 0x0, delta = 0x40000000 */
/* virtual + KERNEL_PHYS_DELTA = phys */
#define KERNEL_PHYS_DELTA (MEMBASE - KERNEL_BASE)

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
	/* do some early cpu setup */
	mrc		p15, 0, r12, c1, c0, 0
	/* i/d cache disable, mmu disabled */
	bic		r12, #(1<<12)
	bic		r12, #(1<<2 | 1<<0)
	mcr		p15, 0, r12, c1, c0, 0

#if WITH_SMP
	/* figure out our cpu number */
	mrc     p15, 0, r12, c0, c0, 5 /* read MPIDR */

#if 0
	// XXX handle machines with weird cluster numbers
	/* mask off the bottom 12 bits to test cluster number:cpu number */
	ubfx    r12, r12, #0, #12
#else
	/* mask off the bottom 8 bits to test cpu number */
	ubfx    r12, r12, #0, #8
#endif

	/* if we're not cpu 0:0, fall into a trap and wait */
	teq     r12, #0
	movne	r0, r12
	bne     arm_secondary_setup
#endif

#if WITH_CPU_EARLY_INIT
	/* call platform/arch/etc specific init code */
	bl __cpu_early_init
#endif

#if WITH_KERNEL_VM
#if WITH_NO_PHYS_RELOCATION
	/* assume that image is properly loaded in physical memory */
#else
	/* see if we need to relocate to our proper location in physical memory */
	adr		r4, _start                           /* this emits sub r4, pc, #constant */
	ldr		r5, =(MEMBASE + KERNEL_LOAD_OFFSET)  /* calculate the binary's physical load address */
	subs	r12, r4, r5                          /* calculate the delta between where we're loaded and the proper spot */
	beq		.Lsetup_mmu

	/* we need to relocate ourselves to the proper spot */
	ldr		r6, =__data_end
	ldr		r7, =(KERNEL_BASE - MEMBASE)
	sub		r6, r7
	add		r6, r12

.Lrelocate_loop:
	ldr		r7, [r4], #4
	str		r7, [r5], #4
	cmp		r4, r6
	bne		.Lrelocate_loop

	/* we're relocated, jump to the right address */
	sub		pc, r12
	nop
#endif

.Lsetup_mmu:
	/* set up the mmu according to mmu_initial_mappings */

	/* load the base of the translation table and clear the table */
	ldr		r4, =(KERNEL_PHYS_DELTA + arm_kernel_translation_table)
		/* r4 = physical address of translation table */

	mov		r5, #0
	mov		r6, #0

	/* walk through all the entries in the translation table, setting them up */
0:
	str		r5, [r4, r6, lsl #2]
	add		r6, #1
	cmp		r6, #4096
	bne		0b

	/* load the address of the mmu_initial_mappings table and start processing */
	ldr		r5, =(KERNEL_PHYS_DELTA + mmu_initial_mappings)
		/* r5 = physical address of mmu initial mapping table */

.Linitial_mapping_loop:
	ldmia	r5!, { r6-r10 }
		/* r6 = phys, r7 = virt, r8 = size, r9 = flags, r10 = name */

	/* mask all the addresses and sizes to 1MB boundaries */
	lsr		r6, #20  /* r6 = physical address / 1MB */
	lsr		r7, #20  /* r7 = virtual address / 1MB */
	lsr		r8, #20  /* r8 = size in 1MB chunks */

	/* if size == 0, end of list */
	cmp		r8, #0
	beq		.Linitial_mapping_done

	/* set up the flags */
	ldr		r10, =MMU_KERNEL_L1_PTE_FLAGS
	teq		r9, #MMU_INITIAL_MAPPING_FLAG_UNCACHED
	ldreq	r10, =MMU_INITIAL_MAP_STRONGLY_ORDERED
	beq		0f
	teq		r9, #MMU_INITIAL_MAPPING_FLAG_DEVICE
	ldreq	r10, =MMU_INITIAL_MAP_DEVICE
		/* r10 = mmu entry flags */

0:
	orr		r11, r10, r6, lsl #20
		/* r11 = phys addr | flags */

	/* store into appropriate translation table entry */
	str		r11, [r4, r7, lsl #2]

	/* loop until we're done */
	add		r6, #1
	add		r7, #1
	subs	r8, #1
	bne		0b

	b		.Linitial_mapping_loop

.Linitial_mapping_done:

	/* set up the mmu */
	bl		.Lmmu_setup

#else
	/* see if we need to relocate */
	mov		r4, pc
	sub		r4, r4, #(.Laddr - _start)
.Laddr:
	ldr		r5, =_start
	cmp		r4, r5
	beq		.Lstack_setup

	/* we need to relocate ourselves to the proper spot */
	ldr		r6, =__data_end

.Lrelocate_loop:
	ldr		r7, [r4], #4
	str		r7, [r5], #4
	cmp		r5, r6
	bne		.Lrelocate_loop

	/* we're relocated, jump to the right address */
	ldr		r4, =.Lstack_setup
	bx		r4
#endif

	/* at this point we're running at our final location in virtual memory (if enabled) */
.Lstack_setup:
	/* set up the stack for irq, fiq, abort, undefined, system/user, and lastly supervisor mode */
	ldr		r12, =abort_stack
	add		r12, #ARCH_DEFAULT_STACK_SIZE

	cpsid	i,#0x12       /* irq */
	mov		sp, r12

	cpsid	i,#0x11       /* fiq */
	mov		sp, r12

	cpsid	i,#0x17       /* abort */
	mov		sp, r12

	cpsid	i,#0x1b       /* undefined */
	mov		sp, r12

	cpsid	i,#0x1f       /* system */
	mov		sp, r12

	cpsid	i,#0x13       /* supervisor */
	mov		sp, r12

	/* stay in supervisor mode from now on out */

	/* copy the initialized data segment out of rom if necessary */
	ldr		r4, =__data_start_rom
	ldr		r5, =__data_start
	ldr		r6, =__data_end

	cmp		r4, r5
	beq		.L__do_bss

.L__copy_loop:
	cmp		r5, r6
	ldrlt	r7, [r4], #4
	strlt	r7, [r5], #4
	blt		.L__copy_loop

.L__do_bss:
	/* clear out the bss */
	ldr		r4, =__bss_start
	ldr		r5, =_end
	mov		r6, #0
.L__bss_loop:
	cmp		r4, r5
	strlt	r6, [r4], #4
	blt		.L__bss_loop

	bl		lk_main
	b		.

#if WITH_KERNEL_VM
	/* per cpu mmu setup, shared between primary and secondary cpus
	   args:
	   r4 == translation table physical
	*/
.Lmmu_setup:
	/* Invalidate TLB */
	mov		r12, #0
	mcr		p15, 0, r12, c8, c7, 0
	isb

	/* Write 0 to TTBCR */
	mcr		p15, 0, r12, c2, c0, 2
	isb

	/* set cacheable attributes on translation walk */
	/* inner write-back write-allocate */
	orr		r12, r4, #(1<<6 | 0<<1)
	/* outer write-back write-allocate */
	orr		r12, #(1<<3)
#if WITH_SMP
	/* (SMP extensions) shareable, outer shareable */
	orr		r12, #(1<<1 | 0<<5)
#endif

	/* Write ttbr with phys addr of the translation table */
	mcr		p15, 0, r12, c2, c0, 0
	isb

	/* Write DACR */
	mov		r12, #0x1
	mcr		p15, 0, r12, c3, c0, 0
	isb

	/* Read SCTLR into r12 */
	mrc		p15, 0, r12, c1, c0, 0

	/* Disable TRE/AFE */
	bic		r12, #(1<<29 | 1<<28)

	/* Turn on the MMU */
	orr		r12, #0x1

	/* Write back SCTLR */
	mcr		p15, 0, r12, c1, c0, 0
	isb

	/* Jump to virtual code address */
	ldr		pc, =1f
1:

	/* Invalidate TLB */
	mov		r12, #0
	mcr		p15, 0, r12, c8, c7, 0
	isb

	/* assume lr was in physical memory, adjust it before returning */
	ldr		r12, =(KERNEL_BASE - MEMBASE)
	add		lr, r12
	bx		lr
#endif

#if WITH_SMP
	/* secondary cpu entry point */
	/* r0 holds cpu number */
FUNCTION(arm_secondary_setup)
	/* all other cpus, trap and wait to be released */
1:
	wfe
	ldr     r12, =(KERNEL_PHYS_DELTA + arm_boot_cpu_lock)
	ldr     r12, [r12]
	cmp     r12, #0
	bne     1b

	cmp		r0, #SMP_MAX_CPUS
	bge		unsupported_cpu_trap

	/* set up the stack for irq, fiq, abort, undefined, system/user, and lastly supervisor mode */
	ldr		r1, =abort_stack
	mov		r2, #ARCH_DEFAULT_STACK_SIZE
	add		r0, #1
	mul		r2, r2, r0
	add		r1, r2

	cpsid	i,#0x12       /* irq */
	mov		sp, r1

	cpsid	i,#0x11       /* fiq */
	mov		sp, r1

	cpsid	i,#0x17       /* abort */
	mov		sp, r1

	cpsid	i,#0x1b       /* undefined */
	mov		sp, r1

	cpsid	i,#0x1f       /* system */
	mov		sp, r1

	cpsid	i,#0x13       /* supervisor */
	mov		sp, r1

#if WITH_KERNEL_VM
	/* load the physical base of the translation table and clear the table */
	ldr		r4, =(KERNEL_PHYS_DELTA + arm_kernel_translation_table)

	/* set up the mmu on this cpu and switch to virtual memory */
	bl		.Lmmu_setup
#endif

	/* stay in supervisor and call into arm arch code to continue setup */
	bl		arm_secondary_entry

	/* cpus above the number we claim to support get trapped here */
unsupported_cpu_trap:
	wfe
	b 		unsupported_cpu_trap
#endif

.ltorg

.data
.align 2

/* vim: set ts=4 sw=4 noexpandtab: */
