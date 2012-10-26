/*
 * Copyright (c) 2008-2012 Travis Geiselbrecht
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
#include <arch/defines.h>

.text

#if ARM_WITH_CACHE

/* low level cache routines for various cpu families */

#if ARM_CPU_ARM1136 || ARM_CPU_ARM926

/* void arch_disable_cache(uint flags) */
FUNCTION(arch_disable_cache)
	mov		r12, #0						// zero register
	mrs		r3, cpsr					// save the old interrupt state
#if ARM_ISA_ARMv6
	cpsid 	iaf							// interrupts disabled
#else
	orr		r3, r3, #(1<<7)
	msr		cpsr, r3
#endif
	
.Ldcache_disable:
	tst		r0, #DCACHE
	beq		.Licache_disable
	mrc     p15, 0, r1, c1, c0, 0		// cr1
	tst		r1, #(1<<2)					// is the dcache already disabled?
	beq		.Licache_disable

	bic		r1, #(1<<2)
	mcr		p15, 0, r1, c1, c0, 0		// disable dcache

#if ARM_CPU_ARM1136
    mcr     p15, 0, r12, c7, c14, 0     // clean & invalidate dcache
#elif ARM_CPU_ARM926
0:
	mrc		p15, 0, r15, c7, c14, 3		// clean & invalidate dcache
	bne		0b
#else
#error whut?
#endif
	mcr		p15, 0, r0, c7, c10, 4		// data sync barrier (formerly drain write buffer)

.Licache_disable:
	tst		r0, #ICACHE
	beq		.Ldone_disable

	mrc     p15, 0, r1, c1, c0, 0		// cr1
	bic		r1, #(1<<12)
	mcr		p15, 0, r1, c1, c0, 0		// disable icache

	mcr		p15, 0, r12, c7, c5, 0		// invalidate icache

.Ldone_disable:
	msr		cpsr, r3
	bx		lr

/* void arch_enable_cache(uint flags) */
FUNCTION(arch_enable_cache)
	mov		r12, #0						// zero register
	mrs		r3, cpsr					// save the old interrupt state
#if ARM_ISA_ARMv6
	cpsid iaf							// interrupts disabled
#else
	orr		r3, r3, #(1<<7)
	msr		cpsr, r3
#endif
	
.Ldcache_enable:
	tst		r0, #DCACHE
	beq		.Licache_enable
	mrc     p15, 0, r1, c1, c0, 0		// cr1
	tst		r1, #(1<<2)					// is the dcache already enabled?
	bne		.Licache_enable

    mcr     p15, 0, r12, c7, c6, 0      // invalidate dcache

	orr		r1, #(1<<2)
	mcr		p15, 0, r1, c1, c0, 0		// enable dcache

.Licache_enable:
	tst		r0, #ICACHE
	beq		.Ldone_enable

	mcr		p15, 0, r12, c7, c5, 0		// invalidate icache

	mrc     p15, 0, r1, c1, c0, 0		// cr1
	orr		r1, #(1<<12)
	mcr		p15, 0, r1, c1, c0, 0		// enable icache

.Ldone_enable:
	msr		cpsr, r3
	bx		lr

#elif ARM_CPU_CORTEX_A8

/* void arch_disable_cache(uint flags) */
FUNCTION(arch_disable_cache)
	stmfd	sp!, {r4-r11, lr}

	mov		r7, r0						// save flags

	mrs		r12, cpsr					// save the old interrupt state
	cpsid	iaf							// interrupts disabled

.Ldcache_disable:
	tst		r7, #DCACHE
	beq		.Licache_disable
	mrc     p15, 0, r0, c1, c0, 0		// cr1
	tst		r0, #(1<<2)					// is the dcache already disabled?
	beq		.Ldcache_already_disabled

	bic		r0, #(1<<2)
	mcr		p15, 0, r0, c1, c0, 0		// disable dcache

	// flush and invalidate the dcache
	// NOTE: trashes a bunch of registers, can't be spilling stuff to the stack
	bl		flush_invalidate_cache_v7

	b		.Ldcache_disable_L2

.Ldcache_already_disabled:
	// make sure all of the caches are invalidated
	// NOTE: trashes a bunch of registers, can't be spilling stuff to the stack
	bl		invalidate_cache_v7

.Ldcache_disable_L2:

#if ARM_WITH_L2
	// disable the L2, if present
	mrc     p15, 0, r0, c1, c0, 1		// aux cr1
	bic		r0, #(1<<1)
	mcr		p15, 0, r0, c1, c0, 1		// disable L2 dcache
#endif

.Licache_disable:
	tst		r7, #ICACHE
	beq		.Ldone_disable

	mrc     p15, 0, r0, c1, c0, 0		// cr1
	bic		r0, #(1<<12)
	mcr		p15, 0, r0, c1, c0, 0		// disable icache

.Ldone_disable:
	// make sure the icache is always invalidated
	mov		r0, #0
	mcr		p15, 0, r0, c7, c5, 0		// invalidate icache to PoU

	msr		cpsr, r12
	ldmfd	sp!, {r4-r11, pc}

/* void arch_enable_cache(uint flags) */
FUNCTION(arch_enable_cache)
	stmfd	sp!, {r4-r11, lr}

	mov		r7, r0						// save flags

	mrs		r12, cpsr					// save the old interrupt state
	cpsid	iaf							// interrupts disabled
	
.Ldcache_enable:
	tst		r7, #DCACHE
	beq		.Licache_enable
	mrc     p15, 0, r0, c1, c0, 0		// cr1
	tst		r0, #(1<<2)					// is the dcache already enabled?
	bne		.Licache_enable

	// invalidate L1 and L2
	// NOTE: trashes a bunch of registers, can't be spilling stuff to the stack
	bl		invalidate_cache_v7

#if ARM_WITH_L2
	// enable the L2, if present
	mrc     p15, 0, r0, c1, c0, 1		// aux cr1
	orr		r0, #(1<<1)
	mcr		p15, 0, r0, c1, c0, 1		// enable L2 dcache
#endif

	mrc     p15, 0, r0, c1, c0, 0		// cr1
	orr		r0, #(1<<2)
	mcr		p15, 0, r0, c1, c0, 0		// enable dcache

.Licache_enable:
	tst		r7, #ICACHE
	beq		.Ldone_enable

	mov		r0, #0
	mcr		p15, 0, r0, c7, c5, 0		// invalidate icache to PoU

	mrc     p15, 0, r0, c1, c0, 0		// cr1
	orr		r0, #(1<<12)
	mcr		p15, 0, r0, c1, c0, 0		// enable icache

.Ldone_enable:
	msr		cpsr, r12
	ldmfd	sp!, {r4-r11, pc}

// flush & invalidate cache routine, trashes r0-r6, r9-r11
flush_invalidate_cache_v7:
	/* from ARMv7 manual, B2-17 */
	MRC 	p15, 1, R0, c0, c0, 1 		// Read CLIDR 
	ANDS 	R3, R0, #0x7000000 
	MOV 	R3, R3, LSR #23 			// Cache level value (naturally aligned) 
	BEQ 	.Lfinished 
	MOV 	R10, #0 
.Loop1:
	ADD 	R2, R10, R10, LSR #1 		// Work out 3xcachelevel 
	MOV 	R1, R0, LSR R2 				// bottom 3 bits are the Cache type for this level 
	AND 	R1, R1, #7 					// get those 3 bits alone 
	CMP 	R1, #2 
	BLT 	.Lskip 						// no cache or only instruction cache at this level 
	MCR 	p15, 2, R10, c0, c0, 0 		// write the Cache Size selection register 
	isb						 			// ISB to sync the change to the CacheSizeID reg 
	MRC 	p15, 1, R1, c0, c0, 0 		// reads current Cache Size ID register 
	AND 	R2, R1, #0x7 				// extract the line length field 
	ADD 	R2, R2, #4 					// add 4 for the line length offset (log2 16 bytes) 
	LDR 	R4, =0x3FF 
	ANDS 	R4, R4, R1, LSR #3 			// R4 is the max number on the way size (right aligned) 
	CLZ 	R5, R4 						// R5 is the bit position of the way size increment 
	LDR 	R6, =0x00007FFF 
	ANDS 	R6, R6, R1, LSR #13 		// R6 is the max number of the index size (right aligned) 
.Loop2:
	MOV 	R9, R4 						// R9 working copy of the max way size (right aligned) 
.Loop3:
	ORR 	R11, R10, R9, LSL R5 		// factor in the way number and cache number into R11 
	ORR 	R11, R11, R6, LSL R2 		// factor in the index number 
	MCR 	p15, 0, R11, c7, c14, 2 	// clean & invalidate by set/way 
	SUBS 	R9, R9, #1 					// decrement the way number 
	BGE 	.Loop3 
	SUBS 	R6, R6, #1 					// decrement the index 
	BGE 	.Loop2 
.Lskip:
 	ADD 	R10, R10, #2 					// increment the cache number 
	CMP 	R3, R10 
	BGT 	.Loop1 

.Lfinished:
	mov		r10, #0
	mcr		p15, 2, r10, c0, c0, 0		// select cache level 0
	isb

	bx		lr

// invalidate cache routine, trashes r0-r6, r9-r11
invalidate_cache_v7:
	/* from ARMv7 manual, B2-17 */
	MRC 	p15, 1, R0, c0, c0, 1 		// Read CLIDR 
	ANDS 	R3, R0, #0x7000000 
	MOV 	R3, R3, LSR #23 			// Cache level value (naturally aligned) 
	BEQ 	.Lfinished_invalidate
	MOV 	R10, #0 
.Loop1_invalidate:
	ADD 	R2, R10, R10, LSR #1 		// Work out 3xcachelevel 
	MOV 	R1, R0, LSR R2 				// bottom 3 bits are the Cache type for this level 
	AND 	R1, R1, #7 					// get those 3 bits alone 
	CMP 	R1, #2 
	BLT 	.Lskip_invalidate 			// no cache or only instruction cache at this level 
	MCR 	p15, 2, R10, c0, c0, 0 		// write the Cache Size selection register 
	isb						 			// ISB to sync the change to the CacheSizeID reg 
	MRC 	p15, 1, R1, c0, c0, 0 		// reads current Cache Size ID register 
	AND 	R2, R1, #0x7 				// extract the line length field 
	ADD 	R2, R2, #4 					// add 4 for the line length offset (log2 16 bytes) 
	LDR 	R4, =0x3FF 
	ANDS 	R4, R4, R1, LSR #3 			// R4 is the max number on the way size (right aligned) 
	CLZ 	R5, R4 						// R5 is the bit position of the way size increment 
	LDR 	R6, =0x00007FFF 
	ANDS 	R6, R6, R1, LSR #13 		// R6 is the max number of the index size (right aligned) 
.Loop2_invalidate:
	MOV 	R9, R4 						// R9 working copy of the max way size (right aligned) 
.Loop3_invalidate:
	ORR 	R11, R10, R9, LSL R5 		// factor in the way number and cache number into R11 
	ORR 	R11, R11, R6, LSL R2 		// factor in the index number 
	MCR 	p15, 0, R11, c7, c6, 2 		// invalidate by set/way 
	SUBS 	R9, R9, #1 					// decrement the way number 
	BGE 	.Loop3_invalidate 
	SUBS 	R6, R6, #1 					// decrement the index 
	BGE 	.Loop2_invalidate 
.Lskip_invalidate:
 	ADD 	R10, R10, #2 				// increment the cache number 
	CMP 	R3, R10 
	BGT 	.Loop1_invalidate 

.Lfinished_invalidate:
	mov		r10, #0
	mcr		p15, 2, r10, c0, c0, 0		// select cache level 0
	isb

	bx		lr

#else
#error unhandled cpu
#endif

#if ARM_CPU_ARM926 || ARM_CPU_ARM1136 || ARM_CPU_CORTEX_A8
/* shared cache flush routines */

	/* void arch_flush_cache_range(addr_t start, size_t len); */
FUNCTION(arch_clean_cache_range)
#if ARM_WITH_CP15
	add		r2, r0, r1					// calculate the end address
	bic		r0, #(CACHE_LINE-1)			// align the start with a cache line
0:
	mcr		p15, 0, r0, c7, c10, 1		// clean cache to PoC by MVA
	add		r0, r0, #CACHE_LINE
	cmp		r0, r2
	blo		0b
	
	mov		r0, #0
	mcr		p15, 0, r0, c7, c10, 4		// data sync barrier
#endif
	bx		lr

	/* void arch_flush_invalidate_cache_range(addr_t start, size_t len); */
FUNCTION(arch_clean_invalidate_cache_range)
#if ARM_WITH_CP15
	add		r2, r0, r1					// calculate the end address
	bic		r0, #(CACHE_LINE-1)			// align the start with a cache line
0:
	mcr		p15, 0, r0, c7, c14, 1		// clean & invalidate dcache to PoC by MVA
	add		r0, r0, #CACHE_LINE
	cmp		r0, r2
	blo		0b

	mov		r0, #0
	mcr		p15, 0, r0, c7, c10, 4		// data sync barrier
#endif
	bx		lr

	/* void arch_invalidate_cache_range(addr_t start, size_t len); */
FUNCTION(arch_invalidate_cache_range)
#if ARM_WITH_CP15
	add		r2, r0, r1					// calculate the end address
	bic		r0, #(CACHE_LINE-1)			// align the start with a cache line
0:
	mcr		p15, 0, r0, c7, c6, 1		// invalidate dcache to PoC by MVA
	add		r0, r0, #CACHE_LINE
	cmp		r0, r2
	blo		0b

	mov		r0, #0
	mcr		p15, 0, r0, c7, c10, 4		// data sync barrier
#endif
	bx		lr

	/* void arch_sync_cache_range(addr_t start, size_t len); */
FUNCTION(arch_sync_cache_range)
	push    { r14 }
	bl      arch_clean_cache_range

	mov     r0, #0
	mcr     p15, 0, r0, c7, c5, 0       // invalidate icache to PoU

	pop     { pc }

#endif // ARM_CPU_...

#else

/* no cache */

FUNCTION(arch_disable_cache)
	bx		lr

FUNCTION(arch_enable_cache)
	bx		lr

FUNCTION(arch_clean_cache_range)
	bx		lr

FUNCTION(arch_clean_invalidate_cache_range)
	bx		lr

FUNCTION(arch_sync_cache_range)
	bx		lr

#endif // ARM_WITH_CACHE

