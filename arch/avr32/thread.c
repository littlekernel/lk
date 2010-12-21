/*
 * Copyright (c) 2009 Travis Geiselbrecht
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
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <compiler.h>
#include <debug.h>
#include <kernel/thread.h>
#include <arch/avr32.h>

struct context_switch_frame {
	vaddr_t lr;
	vaddr_t sr;
	vaddr_t r9;
	vaddr_t r8;
	vaddr_t r7;
	vaddr_t r6;
	vaddr_t r5;
	vaddr_t r4;
	vaddr_t r3;
	vaddr_t r2;
	vaddr_t r1;
	vaddr_t r0;
};

extern void avr32_context_switch(addr_t *old_sp, addr_t new_sp);

static void initial_thread_func(void) __NO_RETURN;
static void initial_thread_func(void)
{
	int ret;

	printf("initial_thread_func: thread %p calling %p with arg %p\n", current_thread, current_thread->entry, current_thread->arg);
	printf("initial mode 0x%x\n", avr32_get_mode());
//	dump_thread(current_thread);

	/* exit the implicit critical section we're within */
	exit_critical_section();

	printf("actually calling\n");
	ret = current_thread->entry(current_thread->arg);

	printf("initial_thread_func: thread %p exiting with %d\n", current_thread, ret);

	thread_exit(ret);
}

void arch_thread_initialize(thread_t *t)
{
	// create a default stack frame on the stack
	vaddr_t stack_top = (vaddr_t)t->stack + t->stack_size;

	struct context_switch_frame *frame = (struct context_switch_frame *)(stack_top);
	frame--;

	// fill it in
	memset(frame, 0, sizeof(*frame));
	frame->lr = (vaddr_t)&initial_thread_func;

	// set the initial SR for supervisor mode, global interrupt mask
	frame->sr = (1<<22)|(1<<16);
	
	// set the stack pointer
	t->arch.sp = (vaddr_t)frame;

	printf("finished initializing thread stack: thread %p, sp 0x%x. stack 0x%x-0x%x\n", t, t->arch.sp, t->stack, stack_top);
	hexdump((void *)t->arch.sp, 64);
}

void arch_context_switch(thread_t *oldthread, thread_t *newthread)
{
	printf("arch_context_switch: old %p (%s) sp %p, new %p (%s) sp %p\n", 
		oldthread, oldthread->name, (void *)oldthread->arch.sp, 
		newthread, newthread->name, (void *)newthread->arch.sp);
//	printf("sr 0x%x\n", avr32_get_sr());
	hexdump(newthread->arch.sp, 128);
//	hexdump(__GET_FRAME(0), 128);
//	printf("before mode 0x%x\n", avr32_get_mode());
	avr32_context_switch(&oldthread->arch.sp, newthread->arch.sp);
//	printf("after mode 0x%x\n", avr32_get_mode());
//	printf("after sr 0x%x\n", avr32_get_sr());
}

