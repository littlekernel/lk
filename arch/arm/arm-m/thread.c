/*
 * Copyright (c) 2012 Travis Geiselbrecht
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
#include <assert.h>
#include <kernel/thread.h>
#include <arch/arm.h>
#include <arch/arm/cm.h>

#define LOCAL_TRACE 0

struct arm_cm_context_switch_frame {
	uint32_t r4;
	uint32_t r5;
	uint32_t r6;
	uint32_t r7;
	uint32_t r8;
	uint32_t r9;
	uint32_t r10;
	uint32_t r11;
	uint32_t lr;
};

static void initial_thread_func(void) __NO_RETURN;
static void initial_thread_func(void)
{
	int ret;

	LTRACEF("thread %p calling %p with arg %p\n", current_thread, current_thread->entry, current_thread->arg);
#if LOCAL_TRACE
	dump_thread(current_thread);
#endif

	/* exit the implicit critical section we're within */
	exit_critical_section();

	ret = current_thread->entry(current_thread->arg);

	LTRACEF("thread %p exiting with %d\n", current_thread, ret);

	thread_exit(ret);
}

void arch_thread_initialize(struct thread *t)
{
	LTRACEF("thread %p, stack %p\n", t, t->stack);

	/* find the top of the stack and align it on an 8 byte boundary */
	uint32_t *sp = (void *)ROUNDDOWN((vaddr_t)t->stack + t->stack_size, 8);

	struct arm_cm_context_switch_frame *frame = (void *)sp;
	frame--;

	/* arrange for lr to point to our starting routine */
	frame->lr = (uint32_t)&initial_thread_func;

	t->arch.sp = (addr_t)frame;
	t->arch.was_preempted = false;
}

volatile struct arm_cm_exception_frame_long *preempt_frame;

static void pendsv(struct arm_cm_exception_frame_long *frame)
{
	arch_disable_ints();
	inc_critical_section();

	ASSERT(critical_section_count == 1);

	LTRACEF("preempting thread %p (%s)\n", current_thread, current_thread->name);

	/* save the iframe the pendsv fired on and hit the preemption code */
	preempt_frame = frame;
	thread_preempt();

	LTRACEF("fell through\n");

	/* if we got here, there wasn't anything to switch to, so just fall through and exit */
	preempt_frame = NULL;

	dec_critical_section();
	arch_enable_ints();
}

/*
 * raw pendsv exception handler, triggered by interrupt glue to schedule
 * a preemption check.
 */
__NAKED void _pendsv(void)
{
	__asm__ volatile(
	    "push	{ r4-r11, lr };"
	    "mov	r0, sp;"
	    "bl		%0;"
	    "pop	{ r4-r11, lr };"
	    "bx		lr;"
	    :: "i" (pendsv)
	);
	__UNREACHABLE;
}

/* 
 * svc handler, used to hard switch the cpu into exception mode to return
 * to preempted thread.
 */
__NAKED void _svc(void)
{
	__asm__ volatile(
		/* load the pointer to the original exception frame we want to restore */
		"mov	sp, r4;"
		"pop	{ r4-r11, lr };"
		"bx		lr;"
	);
}

__NAKED static void _half_save_and_svc(vaddr_t *fromsp, vaddr_t tosp)
{
	__asm__ volatile(
		"push	{ r4-r11, lr };"
		"str	sp, [r0];"

		/* make sure we load the destination sp here before we reenable interrupts */
		"mov	sp, r1;"

		"clrex;"
		"cpsie 	i;"

		"mov	r4, r1;"
		"svc #0;" /* make a svc call to get us into handler mode */
	);	
}

/* simple scenario where the to and from thread yielded */
__NAKED static void _arch_non_preempt_context_switch(vaddr_t *fromsp, vaddr_t tosp)
{
	__asm__ volatile(
		"push	{ r4-r11, lr };"
		"str	sp, [r0];"

		"mov	sp, r1;"
		"pop	{ r4-r11, lr };"
		"clrex;"
		"bx		lr;"
	);
}

__NAKED static void _thread_mode_bounce(void)
{
	__asm__ volatile(
		"pop	{ r4-r11, lr };"
		"bx		lr;"
	);
	__UNREACHABLE;
}

/*
 * The raw context switch routine. Called by the scheduler when it decides to switch.
 * Called either in the context of a thread yielding or blocking (interrupts disabled,
 * on the system stack), or inside the pendsv handler on a thread that is being preempted
 * (interrupts disabled, in handler mode). If preempt_frame is set the thread
 * is being preempted.
 */
void arch_context_switch(struct thread *oldthread, struct thread *newthread)
{
	LTRACE_ENTRY;

	if (newthread->arch.was_preempted) {
		/* we're about to return directly to a thread that was preempted (in user space),
		 * so push its critical section count back down to zero
		 */
		critical_section_count = newthread->saved_critical_section_count = 0;
	}

	/* if preempt_frame is set, we are being preempted */
	if (preempt_frame) {
		oldthread->arch.was_preempted = true;
		oldthread->arch.sp = (addr_t)preempt_frame;
		preempt_frame = NULL;

		LTRACEF("we're preempted, new %d\n", newthread->arch.was_preempted);
		if (newthread->arch.was_preempted) {
			/* return directly to the preempted thread's iframe */
			__asm__ volatile(
			    "mov	sp, %0;"
			    "cpsie	i;"
			    "pop	{ r4-r11, lr };"
			    "clrex;"
			    "bx		lr;"
			    :: "r"(newthread->arch.sp)
			);
			__UNREACHABLE;
		} else {
			/* we're inside a pendsv, switching to a user mode thread */
			/* set up a fake frame to exception return to */
			struct arm_cm_exception_frame_short *frame = (void *)newthread->arch.sp;
			frame--;

			frame->pc = (uint32_t)&_thread_mode_bounce;
			frame->psr = (1 << 24); /* thread bit set, IPSR 0 */
			frame->r0 = frame->r1 =  frame->r2 = frame->r3 = frame->r12 = frame->lr = 99;

			LTRACEF("iretting to user space\n");
			//hexdump(frame, sizeof(*frame) + 64);

			__asm__ volatile(
			    "clrex;"
			    "mov	sp, %0;"
			    "bx		%1;"
			    :: "r"(frame), "r"(0xfffffff9)
			);
			__UNREACHABLE;
		}
	} else {
		oldthread->arch.was_preempted = false;

		if (newthread->arch.was_preempted) {
			LTRACEF("not being preempted, but switching to preempted thread\n");
			_half_save_and_svc(&oldthread->arch.sp, newthread->arch.sp);
		} else {
			/* fast path, both sides did not preempt */
			_arch_non_preempt_context_switch(&oldthread->arch.sp, newthread->arch.sp);
		}
	}

}

