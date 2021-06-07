/*
 * Copyright (c) 2021 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/debug.h>
#include <lk/trace.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <kernel/thread.h>
#include <arch/m68k.h>

#define LOCAL_TRACE 0

struct thread *_current_thread;

static void initial_thread_func(void) __NO_RETURN;
static void initial_thread_func(void) {
    thread_t *ct = get_current_thread();

#if LOCAL_TRACE
    LTRACEF("thread %p calling %p with arg %p\n", ct, ct->entry, ct->arg);
    dump_thread(ct);
#endif

    /* release the thread lock that was implicitly held across the reschedule */
    spin_unlock(&thread_lock);
    arch_enable_ints();

    int ret = ct->entry(ct->arg);

    LTRACEF("thread %p exiting with %d\n", ct, ret);

    thread_exit(ret);
}

void arch_thread_initialize(thread_t *t) {
    LTRACEF("t %p (%s)\n", t, t->name);

    /* zero out the thread context */
    memset(&t->arch.cs_frame, 0, sizeof(t->arch.cs_frame));

    t->arch.cs_frame.sp = (vaddr_t)t->stack + t->stack_size;
    t->arch.cs_frame.pc = (vaddr_t)&initial_thread_func;
}

void arch_context_switch(thread_t *oldthread, thread_t *newthread) {
    LTRACEF("old %p (%s), new %p (%s)\n", oldthread, oldthread->name, newthread, newthread->name);

    m68k_context_switch(&oldthread->arch.cs_frame, &newthread->arch.cs_frame);
}

void arch_dump_thread(thread_t *t) {
#if 0
    if (t->state != THREAD_RUNNING) {
        dprintf(INFO, "\tarch: ");
        dprintf(INFO, "sp 0x%x\n", t->arch.cs_frame.r1);
    }
#endif
}

