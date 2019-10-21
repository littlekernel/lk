/*
 * Copyright (c) 2019 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <assert.h>
#include <lk/debug.h>
#include <lk/trace.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <kernel/thread.h>
#include <arch/vax.h>

#define LOCAL_TRACE 1

struct thread *_current_thread;

extern void vax_initial_thread_func(void);

void initial_thread_func(void) __NO_RETURN;
void initial_thread_func(void) {
    DEBUG_ASSERT(arch_ints_disabled());

    thread_t *ct = get_current_thread();

#if LOCAL_TRACE
    LTRACEF("thread %p calling %p with arg %p\n", ct, ct->entry, ct->arg);
    dump_thread(ct);
#endif

    // release the thread lock that was implicitly held across the reschedule
    spin_unlock(&thread_lock);
    arch_enable_ints();

    int ret = ct->entry(ct->arg);

    LTRACEF("thread %p exiting with %d\n", ct, ret);

    thread_exit(ret);
}

void arch_thread_initialize(thread_t *t) {
    LTRACEF("t %p (%s)\n", t, t->name);

    // zero out the arch thread context, including the PCB
    memset(&t->arch, 0, sizeof(t->arch));

    // initialize the top of the stack
    uint32_t *stack_top = (uint32_t *)((uintptr_t)t->stack + t->stack_size);
    t->arch.pcb.ksp = (uint32_t)stack_top;

    // set the initial address to the initial_thread_func
    t->arch.pcb.pc = (uint32_t)&vax_initial_thread_func;
    t->arch.pcb.psl = (31 << 16); // IPL 31
    t->arch.pcb.p0lr = (4 << 24); // ASTLVL 4
}

void arch_context_switch(thread_t *oldthread, thread_t *newthread) {
    DEBUG_ASSERT(arch_ints_disabled());

    LTRACEF("old %p (%s) pcb %p, new %p (%s) pcb %p\n",
            oldthread, oldthread->name, &oldthread->arch.pcb,
            newthread, newthread->name, &newthread->arch.pcb);

    if (LOCAL_TRACE) {
        hexdump(&newthread->arch.pcb, sizeof(struct vax_pcb));
    }

    vax_context_switch(&newthread->arch.pcb);
}

void arch_dump_thread(thread_t *t) {
    if (t->state != THREAD_RUNNING) {
        dprintf(INFO, "\tarch: ");
        dprintf(INFO, "pcb %p, sp %#x\n", &t->arch.pcb, t->arch.pcb.ksp);
    }
}

