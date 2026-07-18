//
// Copyright (c) 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
//
#include <kernel/thread.h>
#include <lk/debug.h>
#include <lk/trace.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#define LOCAL_TRACE 0

extern "C" {
struct thread *_current_thread;
}

namespace {

__NO_RETURN
void initial_thread_func() {
    thread_t *ct = get_current_thread();

    /* release the thread lock that was implicitly held across the reschedule */
    spin_unlock(&thread_lock);
    arch_enable_ints();

    int ret = ct->entry(ct->arg);

    thread_exit(ret);
}

} // namespace

void arch_thread_initialize(thread_t *t) {
    /* zero out the thread context */
    memset(&t->arch.cs_frame, 0, sizeof(t->arch.cs_frame));

    // compute the top of the allocate space for the stack, rounded down to an 8 byte boundary.
    uint32_t stack_top = ROUNDDOWN((vaddr_t)t->stack + t->stack_size, 8);

    stack_top -= 96; // leave space for the register window spill area, which is 96 bytes
    // clear the top of the stack
    memset((void *)stack_top, 0x99, 96);

    // set up the initial state
    t->arch.cs_frame.sp = stack_top;
    // - 8 because the retl in the context switch routine will add 8 to the pc before jumping to it
    t->arch.cs_frame.pc = (vaddr_t)&initial_thread_func - 8;

    TRACEF("thread %p: sp %#x, pc %#x\n", t, t->arch.cs_frame.sp, t->arch.cs_frame.pc);
}

void arch_context_switch(thread_t *oldthread, thread_t *newthread) {
    LTRACEF("o %p, n %p, new stack %#x\n", oldthread, newthread, newthread->arch.cs_frame.sp);
    sparc_context_switch(&oldthread->arch.cs_frame, &newthread->arch.cs_frame);
}

void arch_dump_thread(const thread_t *t) {
    if (t->state != THREAD_RUNNING) {
        dprintf(INFO, "\tarch: sp 0x%x, pc 0x%x\n", t->arch.cs_frame.sp, t->arch.cs_frame.pc);
    }
}
