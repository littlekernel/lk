/*
 * Copyright (c) 2015 Travis Geiselbrecht
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
#include <arch/riscv.h>

#define LOCAL_TRACE 0

struct thread *_current_thread;

static void initial_thread_func(void) __NO_RETURN;
static void initial_thread_func(void) {
    DEBUG_ASSERT(arch_ints_disabled());

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
    /* zero out the thread context */
    memset(&t->arch.cs_frame, 0, sizeof(t->arch.cs_frame));

    /* if FPU is implemented, default state of zero is default for the thread */

    /* make sure the top of the stack is 16 byte aligned */
    vaddr_t stack_top = ROUNDDOWN((vaddr_t)t->stack + t->stack_size, 16);

    t->arch.cs_frame.sp = stack_top;
    t->arch.cs_frame.ra = (vaddr_t)&initial_thread_func;

    LTRACEF("t %p (%s) stack top %#lx entry %p arg %p\n", t, t->name, stack_top, t->entry, t->arg);
}

void arch_context_switch(thread_t *oldthread, thread_t *newthread) {
    DEBUG_ASSERT(arch_ints_disabled());

    LTRACEF("old %p (%s), new %p (%s)\n", oldthread, oldthread->name, newthread, newthread->name);

    /* floating point context switch */
#if RISCV_FPU
    /* based on a combination of current fpu dirty state in hardware and saved state
     * on the new thread, do a partial or full context switch
     */
    ulong status = riscv_csr_read(RISCV_CSR_XSTATUS);
    ulong hw_state = status & RISCV_CSR_XSTATUS_FS_MASK;

    LTRACEF("old fpu dirty %d, new fpu dirty %d, status %#lx, sd %d\n", oldthread->arch.cs_frame.fpu_dirty,
            newthread->arch.cs_frame.fpu_dirty, hw_state >> RISCV_CSR_XSTATUS_FS_SHIFT,
            (status & RISCV_CSR_XSTATUS_SD) ? 1 : 0);

    /* hardware currently is in the dirty state, so save the state of the fpu regs
     * and mark the thread as dirty.
     */
    switch (hw_state) {
        case RISCV_CSR_XSTATUS_FS_DIRTY:
            oldthread->arch.cs_frame.fpu_dirty = true;
            riscv_fpu_save(&oldthread->arch.cs_frame.fpu);
            break;
        case RISCV_CSR_XSTATUS_FS_INITIAL:
            oldthread->arch.cs_frame.fpu_dirty = false;
            break;
        case RISCV_CSR_XSTATUS_FS_OFF:
            // TODO: handle fpu being disabled
            PANIC_UNIMPLEMENTED;
    }

    if (newthread->arch.cs_frame.fpu_dirty) {
        /* if the new thread has dirty saved state, load it here and mark the cpu as in the
         * clean state, which will transition to dirty if any regs are modified
         */
        riscv_fpu_restore(&newthread->arch.cs_frame.fpu);

        /* at this point the FPU hardware should be in the dirty state because of the above routine */

        /* TODO: see if it's totally safe to reduce to a single instruction based on moving from DIRTY -> CLEAN */
        riscv_csr_clear(RISCV_CSR_XSTATUS, RISCV_CSR_XSTATUS_FS_MASK);
        riscv_csr_set(RISCV_CSR_XSTATUS, RISCV_CSR_XSTATUS_FS_CLEAN);
    } else {
        /* if the thread previously hadn't dirtied the state, zero out the fpu
         * state and mark hardware as initial.
         */
        riscv_fpu_zero();

        /* at this point the FPU hardware should be in the dirty state because of the above routine */

        /* TODO: see if it's totally safe to reduce to a single instruction based on moving from DIRTY -> INITIAL */
        riscv_csr_clear(RISCV_CSR_XSTATUS, RISCV_CSR_XSTATUS_FS_MASK);
        riscv_csr_set(RISCV_CSR_XSTATUS, RISCV_CSR_XSTATUS_FS_INITIAL);
    }
#endif

    /* integer context switch.
     * stack is swapped as part of this routine, so the code will return only when
     * the current thread context is switched back to.
     */
    riscv_context_switch(&oldthread->arch.cs_frame, &newthread->arch.cs_frame);
}

void arch_dump_thread(thread_t *t) {
    if (t->state != THREAD_RUNNING) {
        dprintf(INFO, "\tarch: ");
#if RISCV_FPU
        dprintf(INFO, "fpu dirty %u, ", t->arch.cs_frame.fpu_dirty);
#endif
        dprintf(INFO, "sp %#lx\n", t->arch.cs_frame.sp);
    }
}

