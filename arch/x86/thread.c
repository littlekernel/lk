/*
 * Copyright (c) 2009 Corey Tabaka
 * Copyright (c) 2014 Travis Geiselbrecht
 * Copyright (c) 2015 Intel Corporation
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
#include <arch/fpu.h>

/* we're uniprocessor at this point for x86, so store a global pointer to the current thread */
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
    // create a default stack frame on the stack
    vaddr_t stack_top = (vaddr_t)t->stack + t->stack_size;

#if ARCH_X86_32
    // make sure the top of the stack is 8 byte aligned for ABI compliance
    stack_top = ROUNDDOWN(stack_top, 8);
    struct x86_32_context_switch_frame *frame = (struct x86_32_context_switch_frame *)(stack_top);
#endif
#if ARCH_X86_64
    // make sure the top of the stack is 16 byte aligned for ABI compliance
    stack_top = ROUNDDOWN(stack_top, 16);

    // make sure we start the frame 8 byte unaligned (relative to the 16 byte alignment) because
    // of the way the context switch will pop the return address off the stack. After the first
    // context switch, this leaves the stack in unaligned relative to how a called function expects it.
    stack_top -= 8;
    struct x86_64_context_switch_frame *frame = (struct x86_64_context_switch_frame *)(stack_top);
#endif

    // move down a frame size and zero it out
    frame--;
    memset(frame, 0, sizeof(*frame));

#if ARCH_X86_32
    frame->eip = (vaddr_t) &initial_thread_func;
    frame->eflags = 0x3002; // IF = 0, NT = 0, IOPL = 3
#endif

#if ARCH_X86_64
    frame->rip = (vaddr_t) &initial_thread_func;
    frame->rflags = 0x3002; /* IF = 0, NT = 0, IOPL = 3 */
#endif

    // initialize the saved fpu state
    fpu_init_thread_states(t);

    // set the stack pointer
    t->arch.sp = (vaddr_t)frame;
}

void arch_dump_thread(thread_t *t)
{
    if (t->state != THREAD_RUNNING) {
        dprintf(INFO, "\tarch: ");
        dprintf(INFO, "sp 0x%lx\n", t->arch.sp);
    }
}

#if ARCH_X86_32

void arch_context_switch(thread_t *oldthread, thread_t *newthread)
{
    //dprintf(DEBUG, "arch_context_switch: old %p (%s), new %p (%s)\n", oldthread, oldthread->name, newthread, newthread->name);

#if X86_WITH_FPU
    fpu_context_switch(oldthread, newthread);
#endif

    __asm__ __volatile__ (
        "pushl $1f			\n\t"
        "pushf				\n\t"
        "pusha				\n\t"
        "movl %%esp,(%%edx)	\n\t"
        "movl %%eax,%%esp	\n\t"
        "popa				\n\t"
        "popf				\n\t"
        "ret				\n\t"
        "1:					\n\t"

        :
        : "d" (&oldthread->arch.sp), "a" (newthread->arch.sp)
    );

    /*__asm__ __volatile__ (
        "pushf              \n\t"
        "pushl %%cs         \n\t"
        "pushl $1f          \n\t"
        "pushl %%gs         \n\t"
        "pushl %%fs         \n\t"
        "pushl %%es         \n\t"
        "pushl %%ds         \n\t"
        "pusha              \n\t"
        "movl %%esp,(%%edx) \n\t"
        "movl %%eax,%%esp   \n\t"
        "popa               \n\t"
        "popl %%ds          \n\t"
        "popl %%es          \n\t"
        "popl %%fs          \n\t"
        "popl %%gs          \n\t"
        "iret               \n\t"
        "1: "
        :
        : "d" (&oldthread->arch.sp), "a" (newthread->arch.sp)
    );*/
}
#endif

#if ARCH_X86_64

void arch_context_switch(thread_t *oldthread, thread_t *newthread)
{
#if X86_WITH_FPU
    fpu_context_switch(oldthread, newthread);
#endif

    x86_64_context_switch(&oldthread->arch.sp, newthread->arch.sp);
}
#endif

