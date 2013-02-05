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
#ifndef __KERNEL_DEBUG_H
#define __KERNEL_DEBUG_H

#include <debug.h>

/* kernel event log */
#if WITH_KERNEL_EVLOG

#include <lib/evlog.h>

#ifndef KERNEL_EVLOG_LEN
#define KERNEL_EVLOG_LEN 1024
#endif

void kernel_evlog_init(void);

void kernel_evlog_add(uintptr_t id, uintptr_t arg0, uintptr_t arg1);

#else // !WITH_KERNEL_EVLOG

/* do nothing versions */
static inline void kernel_evlog_init(void) {}
static inline void kernel_evlog_add(uintptr_t id, uintptr_t arg0, uintptr_t arg1) {}

#endif

enum {
	KERNEL_EVLOG_NULL = 0,
	KERNEL_EVLOG_CONTEXT_SWITCH,
	KERNEL_EVLOG_PREEMPT,
	KERNEL_EVLOG_TIMER_TICK,
	KERNEL_EVLOG_TIMER_CALL,
	KERNEL_EVLOG_IRQ_ENTER,
	KERNEL_EVLOG_IRQ_EXIT,
};

#define KEVLOG_THREAD_SWITCH(from, to) kernel_evlog_add(KERNEL_EVLOG_CONTEXT_SWITCH, (uintptr_t)from, (uintptr_t)to)
#define KEVLOG_THREAD_PREEMPT(thread) kernel_evlog_add(KERNEL_EVLOG_PREEMPT, (uintptr_t)thread, 0)
#define KEVLOG_TIMER_TICK() kernel_evlog_add(KERNEL_EVLOG_TIMER_TICK, 0, 0)
#define KEVLOG_TIMER_CALL(ptr, arg) kernel_evlog_add(KERNEL_EVLOG_TIMER_CALL, (uintptr_t)ptr, (uintptr_t)arg)
#define KEVLOG_IRQ_ENTER(irqn) kernel_evlog_add(KERNEL_EVLOG_IRQ_ENTER, (uintptr_t)irqn, 0)
#define KEVLOG_IRQ_EXIT(irqn) kernel_evlog_add(KERNEL_EVLOG_IRQ_EXIT, (uintptr_t)irqn, 0)

#endif

