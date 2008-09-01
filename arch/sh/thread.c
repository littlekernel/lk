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
#include <sys/types.h>
#include <string.h>
#include <debug.h>
#include <kernel/thread.h>
#include <arch/sh4.h>

struct context_switch_frame {
	addr_t pr;
	addr_t r14;
	addr_t r13;
	addr_t r12;
	addr_t r11;
	addr_t r10;
	addr_t r9;
	addr_t r8;
	addr_t macl;
	addr_t mach;
	addr_t fpscr;
	addr_t fr15;
	addr_t fr14;
	addr_t fr13;
	addr_t fr12;
};

extern void sh4_context_switch(addr_t *old_sp, addr_t new_sp);

static void initial_thread_func(void) __NO_RETURN;
static void initial_thread_func(void)
{
	int ret;

//	dprintf("initial_thread_func: thread %p calling %p with arg %p\n", current_thread, current_thread->entry, current_thread->arg);
//	dump_thread(current_thread);

	/* exit the implicit critical section we're within */
	exit_critical_section();

	ret = current_thread->entry(current_thread->arg);

	dprintf("initial_thread_func: thread %p exiting with %d\n", current_thread, ret);

	thread_exit(ret);
}

void arch_thread_initialize(thread_t *t)
{
	// create a default stack frame on the stack
	struct context_switch_frame *frame = (struct context_switch_frame *)((vaddr_t)t->stack + t->stack_size);
	frame--;

	// fill it in
	memset(frame, 0, sizeof(*frame));
	frame->pr = (vaddr_t)&initial_thread_func;
	
	// set the stack pointer
	t->arch.sp = (vaddr_t)frame;
}

void arch_context_switch(thread_t *oldthread, thread_t *newthread)
{
//	dprintf("arch_context_switch: old %p (%s), new %p (%s)\n", oldthread, oldthread->name, newthread, newthread->name);
	sh4_context_switch(&oldthread->arch.sp, newthread->arch.sp);
}

