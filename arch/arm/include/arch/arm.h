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
#ifndef __ARCH_ARM_H
#define __ARCH_ARM_H

#include <sys/types.h>
#include <arch/arm/cores.h>
#include <compiler.h>

/* due to the cp15 accessors below, you're gonna have a bad time if you try
 * to compile in thumb mode. Either compile in ARM only or get a thumb2 capable cpu.
 */
#if defined(__thumb__) && !defined(__thumb2__)
#error this file unsupported in thumb1 mode
#endif

__BEGIN_CDECLS

#if ARM_ISA_ARMV7
#define DSB __asm__ volatile("dsb" ::: "memory")
#define ISB __asm__ volatile("isb" ::: "memory")
#elif ARM_ISA_ARMV6
#define DSB __asm__ volatile("mcr p15, 0, %0, c7, c10, 4" :: "r" (0) : "memory")
#define ISB __asm__ volatile("mcr p15, 0, %0, c7, c5, 4" :: "r" (0) : "memory")
#else
#error unhandled arm isa
#endif

void arm_context_switch(vaddr_t *old_sp, vaddr_t new_sp);

static inline uint32_t read_cpsr(void)
{
	uint32_t cpsr;

	__asm__ volatile("mrs   %0, cpsr" : "=r" (cpsr));
	return cpsr;
}

struct arm_iframe {
	uint32_t usp;
	uint32_t ulr;
	uint32_t r0;
	uint32_t r1;
	uint32_t r2;
	uint32_t r3;
	uint32_t r12;
	uint32_t lr;
	uint32_t pc;
	uint32_t spsr;
};

struct arm_fault_frame {
	uint32_t spsr;
	uint32_t usp;
	uint32_t ulr;
	uint32_t r[13];
	uint32_t pc;
};

#define MODE_MASK 0x1f
#define MODE_USR 0x10
#define MODE_FIQ 0x11
#define MODE_IRQ 0x12
#define MODE_SVC 0x13
#define MODE_MON 0x16
#define MODE_ABT 0x17
#define MODE_UND 0x1b
#define MODE_SYS 0x1f

struct arm_mode_regs {
	uint32_t fiq_r13, fiq_r14;
	uint32_t irq_r13, irq_r14;
	uint32_t svc_r13, svc_r14;
	uint32_t abt_r13, abt_r14;
	uint32_t und_r13, und_r14;
	uint32_t sys_r13, sys_r14;
};

void arm_save_mode_regs(struct arm_mode_regs *regs);

#define GEN_CP15_REG_FUNCS(reg, op1, c1, c2, op2) \
static inline __ALWAYS_INLINE uint32_t arm_read_##reg(void) { \
	uint32_t val; \
	__asm__ volatile("mrc p15, " #op1 ", %0, " #c1 ","  #c2 "," #op2 : "=r" (val)); \
	return val; \
} \
\
static inline __ALWAYS_INLINE void arm_write_##reg(uint32_t val) { \
	__asm__ volatile("mcr p15, " #op1 ", %0, " #c1 ","  #c2 "," #op2 :: "r" (val)); \
	ISB; \
}

/* armv6+ control regs */
GEN_CP15_REG_FUNCS(sctlr, 0, c1, c0, 0);
GEN_CP15_REG_FUNCS(actlr, 0, c1, c0, 1);
GEN_CP15_REG_FUNCS(cpacr, 0, c1, c0, 2);

GEN_CP15_REG_FUNCS(ttbr, 0, c2, c0, 0);
GEN_CP15_REG_FUNCS(ttbr0, 0, c2, c0, 0);
GEN_CP15_REG_FUNCS(ttbr1, 0, c2, c0, 1);
GEN_CP15_REG_FUNCS(ttbcr, 0, c2, c0, 2);
GEN_CP15_REG_FUNCS(dacr, 0, c3, c0, 0);
GEN_CP15_REG_FUNCS(dfsr, 0, c5, c0, 0);
GEN_CP15_REG_FUNCS(ifsr, 0, c5, c0, 1);
GEN_CP15_REG_FUNCS(dfar, 0, c6, c0, 0);
GEN_CP15_REG_FUNCS(wfar, 0, c6, c0, 1);
GEN_CP15_REG_FUNCS(ifar, 0, c6, c0, 2);

GEN_CP15_REG_FUNCS(fcseidr, 0, c13, c0, 0);
GEN_CP15_REG_FUNCS(contextidr, 0, c13, c0, 1);
GEN_CP15_REG_FUNCS(tpidrurw, 0, c13, c0, 2);
GEN_CP15_REG_FUNCS(tpidruro, 0, c13, c0, 3);
GEN_CP15_REG_FUNCS(tpidrprw, 0, c13, c0, 4);

/* armv7+ */
GEN_CP15_REG_FUNCS(vbar, 0, c12, c0, 0);

void arm_invalidate_tlb(void);

__END_CDECLS

#endif
