/*
 * Copyright (c) 2008-2012 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#ifndef __ARM_M_ARCH_THREAD_H
#define __ARM_M_ARCH_THREAD_H

#include <stdbool.h>
#include <sys/types.h>

struct arch_thread {
    vaddr_t sp;
};

/* Cortex-M does not hold the sched lock across the context switch: the switch
 * is taken in PendSV, which arch_context_switch() can only reach by dropping
 * the lock and enabling interrupts, and a thread preempted out of an interrupt
 * handler resumes straight into thread code with no scheduler code on the way.
 * So no lock is ever handed from the outgoing thread to the incoming one and
 * there is no incoming side hook; the scheduler skips that bookkeeping here. */
#define ARCH_CONTEXT_SWITCH_DROPS_LOCK 1

#endif

