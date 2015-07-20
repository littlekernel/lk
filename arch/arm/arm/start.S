/*
 * Copyright (c) 2008-2015 Travis Geiselbrecht
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
#if WITH_SMP
	b	arm_reset
#endif

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
#if WITH_KERNEL_VM
	/* enable caches so atomics and spinlocks work */
	orr		r12, r12, #(1<<12)
	orr		r12, r12, #(1<<2)
#endif // WITH_KERNEL_VM
	mcr		p15, 0, r12, c1, c0, 0

	/* calculate the physical offset from our eventual virtual location */
.Lphys_offset:
	ldr		r4, =.Lphys_offset
	adr		r11, .Lphys_offset
	sub		r11, r11, r4

#if WITH_SMP
	/* figure out our cpu number */
	mrc     p15, 0, r12, c0, c0, 5 /* read MPIDR */

	/* mask off the bottom bits to test cluster number:cpu number */
	ubfx    r12, r12, #0, #SMP_CPU_ID_BITS

	/* if we're not cpu 0:0, fall into a trap and wait */
	teq     r12, #0
	movne	r0, r12
	bne     arm_secondary_setup
#endif // WITH_SMP

#if WITH_CPU_EARLY_INIT
	/* call platform/arch/etc specific init code */
	bl __cpu_early_init
#endif // WITH_CPU_EARLY_INIT

#if WITH_NO_PHYS_RELOCATION
	/* assume that image is properly loaded in physical memory */
#else
	/* see if we need to relocate to our proper location in physical memory */
	adr		r4, _start                           /* this emits sub r4, pc, #constant */
	ldr		r5, =(MEMBASE + KERNEL_LOAD_OFFSET)  /* calculate the binary's physical load address */
	subs	r12, r4, r5                          /* calculate the delta between where we're loaded and the proper spot */
	beq		.Lrelocate_done

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
	nop		/* skipped in the add to pc */

	/* recalculate the physical offset */
	sub		r11, r11, r12

.Lrelocate_done:
#endif // !WITH_NO_PHYS_RELOCATION

#if WITH_KERNEL_VM
.Lsetup_mmu:

	/* set up the mmu according to mmu_initial_mappings */

	/* load the base of the translation table and clear the table */
	ldr		r4, =arm_kernel_translation_table
	add		r4, r4, r11
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
	ldr		r5, =mmu_initial_mappings
	add		r5, r5, r11
		/* r5 = physical address of mmu initial mapping table */

.Linitial_mapping_loop:
	ldmia	r5!, { r6-r10 }
		/* r6 = phys, r7 = virt, r8 = size, r9 = flags, r10 = name */

	/* round size up to 1MB alignment */
	ubfx		r10, r6, #0, #20
	add		r8, r8, r10
	add		r8, r8, #(1 << 20)
	sub		r8, r8, #1

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
	orr		r12, r10, r6, lsl #20
		/* r12 = phys addr | flags */

	/* store into appropriate translation table entry */
	str		r12, [r4, r7, lsl #2]

	/* loop until we're done */
	add		r6, #1
	add		r7, #1
	subs	r8, #1
	bne		0b

	b		.Linitial_mapping_loop

.Linitial_mapping_done:

#if MMU_WITH_TRAMPOLINE
	/* move arm_kernel_translation_table address to r8 and
	 * set cacheable attributes on translation walk
	 */
	orr		r8, r4, #MMU_TTBRx_FLAGS

	/* Prepare tt_trampoline page table */
	/* Calculate pagetable physical addresses */
	ldr		r4, =tt_trampoline	/* r4 = tt_trampoline vaddr */
	add		r4, r4, r11		/* r4 = tt_trampoline paddr */

	/* Zero tt_trampoline translation tables */
	mov		r6, #0
	mov		r7, #0
1:
	str		r7, [r4, r6, lsl#2]
	add		r6, #1
	cmp		r6, #0x1000
	blt		1b

	/* Setup 1M section mapping at
	 * phys	 -> phys   and
	 * virt	 -> phys
	 */
	lsr		r6, pc, #20		/* r6 = paddr index */
	ldr		r7, =MMU_KERNEL_L1_PTE_FLAGS
	add		r7, r7, r6, lsl #20	/* r7 = pt entry */

	str		r7, [r4, r6, lsl #2]	/* tt_trampoline[paddr index] = pt entry */

	rsb		r6, r11, r6, lsl #20	/* r6 = vaddr */
	str		r7, [r4, r6, lsr #(20 - 2)]	/* tt_trampoline[vaddr index] = pt entry */
#endif // MMU_WITH_TRAMPOLINE

	/* set up the mmu */
	bl		.Lmmu_setup
#endif // WITH_KERNEL_VM

	/* at this point we're running at our final location in virtual memory (if enabled) */
.Lstack_setup:
	/* set up the stack for irq, fiq, abort, undefined, system/user, and lastly supervisor mode */
	mov		r12, #0

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
	ldr		r12, =abort_stack
	add		r12, #ARCH_DEFAULT_STACK_SIZE
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
	   r8 == final translation table physical (if using trampoline)
	*/
.Lmmu_setup:
	/* Invalidate TLB */
	mov		r12, #0
	mcr		p15, 0, r12, c8, c7, 0
	isb

	/* Write 0 to TTBCR */
	mcr		p15, 0, r12, c2, c0, 2
	isb

	/* Set cacheable attributes on translation walk */
	orr		r12, r4, #MMU_TTBRx_FLAGS

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

#if MMU_WITH_TRAMPOLINE
	/* Switch to main page table */
	mcr		p15, 0, r8, c2, c0, 0
	isb
#endif

	/* Invalidate TLB */
	mov		r12, #0
	mcr		p15, 0, r12, c8, c7, 0
	isb

	/* assume lr was in physical memory, adjust it before returning */
	sub		lr, r11
	bx		lr
#endif

#if WITH_SMP
	/* secondary cpu entry point */
	/* r0 holds cpu number */
	/* r11 hold phys offset */
FUNCTION(arm_secondary_setup)
	/* all other cpus, trap and wait to be released */
1:
	wfe
	ldr     r12, =arm_boot_cpu_lock
	add     r12, r12, r11
	ldr     r12, [r12]
	cmp     r12, #0
	bne     1b

	and	r1, r0, #0xff
	cmp	r1, #(1 << SMP_CPU_CLUSTER_SHIFT)
	bge	unsupported_cpu_trap
	bic	r0, r0, #0xff
	orr	r0, r1, r0, LSR #(8 - SMP_CPU_CLUSTER_SHIFT)

	cmp		r0, #SMP_MAX_CPUS
	bge		unsupported_cpu_trap
	mov		r5, r0 /* save cpu num */

	/* set up the stack for irq, fiq, abort, undefined, system/user, and lastly supervisor mode */
	mov		r1, #0
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
	ldr		r1, =abort_stack
	mov		r2, #ARCH_DEFAULT_STACK_SIZE
	add		r0, #1
	mul		r2, r2, r0
	add		r1, r2

	mov		sp, r1

#if WITH_KERNEL_VM
	/* load the physical base of the translation table and clear the table */
	ldr		r4, =arm_kernel_translation_table
	add		r4, r4, r11

#if MMU_WITH_TRAMPOLINE
	/* move arm_kernel_translation_table address to r8 and
	 * set cacheable attributes on translation walk
	 */
	orr		r8, r4, #MMU_TTBRx_FLAGS

	/* Prepare tt_trampoline page table */
	/* Calculate pagetable physical addresses */
	ldr		r4, =tt_trampoline	/* r4 = tt_trampoline vaddr */
	add		r4, r4, r11		/* r4 = tt_trampoline paddr */
#endif

	/* set up the mmu on this cpu and switch to virtual memory */
	bl		.Lmmu_setup
#endif

	/* stay in supervisor and call into arm arch code to continue setup */
	mov		r0, r5
	bl		arm_secondary_entry

	/* cpus above the number we claim to support get trapped here */
unsupported_cpu_trap:
	wfe
	b 		unsupported_cpu_trap
#endif

.ltorg

#if WITH_KERNEL_VM && MMU_WITH_TRAMPOLINE
.section ".bss.prebss.translation_table"
.align 14
DATA(tt_trampoline)
	.skip 16384
#endif

.data
.align 2

/* vim: set ts=4 sw=4 noexpandtab: */
