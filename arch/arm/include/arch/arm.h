/*
 * Copyright (c) 2008 Travis Geiselbrecht
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

#if defined(__cplusplus)
extern "C" {
#endif

void arm_context_switch(vaddr_t *old_sp, vaddr_t new_sp);

static inline uint32_t read_cpsr() {
	uint32_t cpsr;

	__asm__ volatile("mrs   %0, cpsr" : "=r" (cpsr));
	return cpsr;
}

struct arm_iframe {
	uint32_t spsr;
	uint32_t r0;
	uint32_t r1;
	uint32_t r2;
	uint32_t r3;
	uint32_t r12;
	uint32_t lr;
	uint32_t pc;
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

uint32_t arm_read_cr1(void);
void arm_write_cr1(uint32_t val);
uint32_t arm_read_cr1_aux(void);
void arm_write_cr1_aux(uint32_t val);
void arm_write_ttbr(uint32_t val);
void arm_write_dacr(uint32_t val);
void arm_invalidate_tlb(void);

#if defined(__cplusplus)
}
#endif

#endif
