/*
 * Copyright (c) 2009 Corey Tabaka
 * Copyright (c) 2014 Intel Corporation
 * Copyright (c) 2014 Travis Geiselbrecht
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
#include <debug.h>
#include <kernel/thread.h>
#include <kernel/spinlock.h>
#include <arch/x86.h>
#include <arch/x86/descriptor.h>

struct context_switch_frame {
	uint64_t rdi, rsi, rdx, rcx, rax, rbx, rbp;
	uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
	uint64_t rflags;
	uint64_t rip;
};

/* we're uniprocessor at this point for x86-64, so store a global pointer to the current thread */
struct thread *_current_thread;

static void initial_thread_func(void) __NO_RETURN;
static void initial_thread_func(void)
{
	int ret;

	/* release the thread lock that was implicitly held across the reschedule */
	spin_unlock(&thread_lock);
	arch_enable_ints();

	ret = _current_thread->entry(_current_thread->arg);

	thread_exit(ret);
}

void arch_thread_initialize(thread_t *t)
{
	/* create a default stack frame on the stack */
	vaddr_t stack_top = (vaddr_t)t->stack + t->stack_size;

	/* make sure the top of the stack is 8 byte aligned 
              for EABI compliance */

	stack_top = ROUNDDOWN(stack_top, 8);

	struct context_switch_frame *frame = 
                        (struct context_switch_frame *)(stack_top);
	frame--;

	/* fill it in */
	memset(frame, 0, sizeof(*frame));

	frame->rip = (vaddr_t) &initial_thread_func;
	frame->rflags = 0x3002; /* IF = 0, NT = 0, IOPL = 3 */

	/* set the stack pointer */
	t->arch.rsp = (vaddr_t)frame;
}

void arch_context_switch(thread_t *oldthread, thread_t *newthread)
{

	/* save the old context and restore the new */
	__asm__ __volatile__ (
		"pushq $1f			\n\t"
		"pushf				\n\t"
		"pushq %%rdi			\n\t"
		"pushq %%rsi			\n\t"
		"pushq %%rdx			\n\t"
		"pushq %%rcx			\n\t"
		"pushq %%rax			\n\t"
		"pushq %%rbx			\n\t"
		"pushq %%rbp			\n\t"
		"pushq %%r8			\n\t"
		"pushq %%r9			\n\t"
		"pushq %%r10			\n\t"
		"pushq %%r11			\n\t"
		"pushq %%r12			\n\t"
		"pushq %%r13			\n\t"
		"pushq %%r14			\n\t"
		"pushq %%r15			\n\t"

		"movq %%rsp,%0			\n\t"
		"movq %1,%%rsp			\n\t"

		"popq %%r15			\n\t"
		"popq %%r14			\n\t"
		"popq %%r13			\n\t"
		"popq %%r12			\n\t"
		"popq %%r11			\n\t"
		"popq %%r10			\n\t"
		"popq %%r9			\n\t"
		"popq %%r8			\n\t"
		"popq %%rbp			\n\t"
		"popq %%rbx			\n\t"
		"popq %%rax			\n\t"
		"popq %%rcx			\n\t"
		"popq %%rdx			\n\t"
		"popq %%rsi			\n\t"
		"popq %%rdi			\n\t"
		"popf				\n\t"

		"ret				\n\t"
		"1:				\n\t"
		: "=g" (oldthread->arch.rsp)
		: "g" (newthread->arch.rsp)
	);
}

/* vim: noexpandtab */
