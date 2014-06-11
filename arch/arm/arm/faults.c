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
#include <debug.h>
#include <bits.h>
#include <arch/arm.h>
#include <kernel/thread.h>

static void dump_mode_regs(uint32_t spsr)
{
	struct arm_mode_regs regs;
	arm_save_mode_regs(&regs);

	dprintf(CRITICAL, "%c%s r13 0x%08x r14 0x%08x\n", ((spsr & MODE_MASK) == MODE_FIQ) ? '*' : ' ', "fiq", regs.fiq_r13, regs.fiq_r14);
	dprintf(CRITICAL, "%c%s r13 0x%08x r14 0x%08x\n", ((spsr & MODE_MASK) == MODE_IRQ) ? '*' : ' ', "irq", regs.irq_r13, regs.irq_r14);
	dprintf(CRITICAL, "%c%s r13 0x%08x r14 0x%08x\n", ((spsr & MODE_MASK) == MODE_SVC) ? '*' : ' ', "svc", regs.svc_r13, regs.svc_r14);
	dprintf(CRITICAL, "%c%s r13 0x%08x r14 0x%08x\n", ((spsr & MODE_MASK) == MODE_UND) ? '*' : ' ', "und", regs.und_r13, regs.und_r14);
	dprintf(CRITICAL, "%c%s r13 0x%08x r14 0x%08x\n", ((spsr & MODE_MASK) == MODE_SYS) ? '*' : ' ', "sys", regs.sys_r13, regs.sys_r14);

	// dump the bottom of the current stack
	addr_t stack;
	switch (spsr & MODE_MASK) {
		case MODE_FIQ:
			stack = regs.fiq_r13;
			break;
		case MODE_IRQ:
			stack = regs.irq_r13;
			break;
		case MODE_SVC:
			stack = regs.svc_r13;
			break;
		case MODE_UND:
			stack = regs.und_r13;
			break;
		case MODE_SYS:
			stack = regs.sys_r13;
			break;
		default:
			stack = 0;
	}

	if (stack != 0) {
		dprintf(CRITICAL, "bottom of stack at 0x%08x:\n", (unsigned int)stack);
		hexdump((void *)stack, 128);
	}
}

static void dump_fault_frame(struct arm_fault_frame *frame)
{
	dprintf(CRITICAL, "r0  0x%08x r1  0x%08x r2  0x%08x r3  0x%08x\n", frame->r[0], frame->r[1], frame->r[2], frame->r[3]);
	dprintf(CRITICAL, "r4  0x%08x r5  0x%08x r6  0x%08x r7  0x%08x\n", frame->r[4], frame->r[5], frame->r[6], frame->r[7]);
	dprintf(CRITICAL, "r8  0x%08x r9  0x%08x r10 0x%08x r11 0x%08x\n", frame->r[8], frame->r[9], frame->r[10], frame->r[11]);
	dprintf(CRITICAL, "r12 0x%08x usp 0x%08x ulr 0x%08x pc  0x%08x\n", frame->r[12], frame->usp, frame->ulr, frame->pc);
	dprintf(CRITICAL, "spsr 0x%08x\n", frame->spsr);

	dump_mode_regs(frame->spsr);
}

static void dump_iframe(struct arm_iframe *frame)
{
	dprintf(CRITICAL, "r0  0x%08x r1  0x%08x r2  0x%08x r3  0x%08x\n", frame->r0, frame->r1, frame->r2, frame->r3);
	dprintf(CRITICAL, "r12 0x%08x usp 0x%08x ulr 0x%08x pc  0x%08x\n", frame->r12, frame->usp, frame->ulr, frame->pc);
	dprintf(CRITICAL, "spsr 0x%08x\n", frame->spsr);

	dump_mode_regs(frame->spsr);
}

static void exception_die(struct arm_fault_frame *frame, const char *msg)
{
	inc_critical_section();
	dprintf(CRITICAL, msg);
	dump_fault_frame(frame);

	halt();
	for (;;);
}

static void exception_die_iframe(struct arm_iframe *frame, const char *msg)
{
	inc_critical_section();
	dprintf(CRITICAL, msg);
	dump_iframe(frame);

	halt();
	for (;;);
}

void arm_syscall_handler(struct arm_fault_frame *frame)
{
	exception_die(frame, "unhandled syscall, halting\n");
}

void arm_undefined_handler(struct arm_iframe *frame)
{
	inc_critical_section();

	/* look at the undefined instruction, figure out if it's something we can handle */
	bool in_thumb = frame->spsr & (1<<5);
	if (in_thumb) {
		frame->pc -= 2;
	} else {
		frame->pc -= 4;
	}

	__UNUSED uint32_t opcode = *(uint32_t *)frame->pc;
	//dprintf(CRITICAL, "undefined opcode 0x%x\n", opcode);

#if ARM_WITH_VFP
	if (in_thumb) {
		/* look for a 32bit thumb instruction */
		if (opcode & 0x0000e800) {
			/* swap the 16bit words */
			opcode = (opcode >> 16) | (opcode << 16);
		}

		if (((opcode & 0xec000e00) == 0xec000a00) || // vfp
			((opcode & 0xef000000) == 0xef000000) || // advanced simd data processing
			((opcode & 0xff100000) == 0xf9000000)) { // VLD

			//dprintf(CRITICAL, "vfp/neon thumb instruction 0x%08x\n", opcode);
			goto fpu;
		}
	} else {
		/* look for arm vfp/neon coprocessor instructions */
		if (((opcode & 0x0c000e00) == 0x0c000a00) || // vfp
		    ((opcode & 0xfe000000) == 0xf2000000) || // advanced simd data processing
			((opcode & 0xff100000) == 0xf4000000)) { // VLD
			//dprintf(CRITICAL, "vfp/neon arm instruction 0x%08x\n", opcode);
			goto fpu;
		}
	}
#endif

	exception_die_iframe(frame, "undefined abort, halting\n");
	return;

#if ARM_WITH_VFP
fpu:
	arm_fpu_undefined_instruction();
	dec_critical_section();
#endif
}

void arm_data_abort_handler(struct arm_fault_frame *frame)
{
	exception_die(frame, "data abort, halting\n");
}

void arm_prefetch_abort_handler(struct arm_fault_frame *frame)
{
	exception_die(frame, "prefetch abort, halting\n");
}

/* vim: set ts=4 sw=4 noexpandtab: */
