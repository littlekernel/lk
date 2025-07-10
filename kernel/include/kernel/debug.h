//
// Copyright (c) 2012-2014 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
//
#pragma once

#include <lk/compiler.h>

__BEGIN_CDECLS

#include <lk/debug.h>

// This file defines the kernel event log, which is a simple logging mechanism
// for kernel events. It is primarily used for debugging and performance analysis.
#if WITH_KERNEL_EVLOG

#include <lib/evlog.h>

#ifndef KERNEL_EVLOG_LEN
#define KERNEL_EVLOG_LEN 1024
#endif

void kernel_evlog_init(void);

void kernel_evlog_add(uintptr_t id, uintptr_t arg0, uintptr_t arg1);
void kernel_evlog_dump(void);

#else // !WITH_KERNEL_EVLOG

// Do-nothing versions.
static inline void kernel_evlog_init(void) {}
static inline void kernel_evlog_add(uintptr_t id, uintptr_t arg0, uintptr_t arg1) {}
static inline void kernel_evlog_dump(void) {}

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

#define KEVLOG_THREAD_SWITCH(from, to) kernel_evlog_add(KERNEL_EVLOG_CONTEXT_SWITCH, (uintptr_t)(from), (uintptr_t)(to))
#define KEVLOG_THREAD_PREEMPT(thread) kernel_evlog_add(KERNEL_EVLOG_PREEMPT, (uintptr_t)(thread), 0)
#define KEVLOG_TIMER_TICK() kernel_evlog_add(KERNEL_EVLOG_TIMER_TICK, 0, 0)
#define KEVLOG_TIMER_CALL(ptr, arg) kernel_evlog_add(KERNEL_EVLOG_TIMER_CALL, (uintptr_t)(ptr), (uintptr_t)(arg))
#define KEVLOG_IRQ_ENTER(irqn) kernel_evlog_add(KERNEL_EVLOG_IRQ_ENTER, (uintptr_t)(irqn), 0)
#define KEVLOG_IRQ_EXIT(irqn) kernel_evlog_add(KERNEL_EVLOG_IRQ_EXIT, (uintptr_t)(irqn), 0)

__END_CDECLS
