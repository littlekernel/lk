//
// Copyright (c) 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
//
#include <lk/debug.h>
#include <lk/trace.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <kernel/thread.h>

struct thread *_current_thread;

static void initial_thread_func(void) __NO_RETURN;
static void initial_thread_func(void) {
    thread_t *ct = get_current_thread();

    /* release the thread lock that was implicitly held across the reschedule */
    spin_unlock(&thread_lock);
    arch_enable_ints();

    int ret = ct->entry(ct->arg);

    thread_exit(ret);
}

void arch_thread_initialize(thread_t *t) {
    /* zero out the thread context */
    memset(&t->arch.cs_frame, 0, sizeof(t->arch.cs_frame));

    t->arch.cs_frame.sp = (vaddr_t)t->stack + t->stack_size - 96;
    t->arch.cs_frame.pc = (vaddr_t)&initial_thread_func;
}

void arch_context_switch(thread_t *oldthread, thread_t *newthread) {
    sparc_context_switch(&oldthread->arch.cs_frame, &newthread->arch.cs_frame);
}

void arch_dump_thread(const thread_t *t) {
    if (t->state != THREAD_RUNNING) {
        dprintf(INFO, "\tarch: sp 0x%x, pc 0x%x\n", t->arch.cs_frame.sp, t->arch.cs_frame.pc);
    }
}
