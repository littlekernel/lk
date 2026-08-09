/*
 * Copyright (c) 2008-2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
/*
 * Scheduler benchmarks and interactive scheduler pokers. These measure cycle
 * counts or need a human to look at the output, so they have no pass/fail
 * criterion and stay console commands. The self validating tests for the
 * kernel synchronization primitives live in kernel/test/thread_tests.c and run
 * under `ut all`.
 */
#include "tests.h"

#include <lk/debug.h>
#include <lk/trace.h>
#include <lk/err.h>
#include <assert.h>
#include <kernel/thread.h>
#include <kernel/event.h>
#include <platform.h>
#include <platform/time.h>
#include <arch/atomic.h>

/* Everything here is only reachable through the console command at the bottom,
 * so without lib/console in the build there is nothing to emit. */
#if WITH_LIB_CONSOLE

static event_t context_switch_event;
static event_t context_switch_done_event;

static int context_switch_tester(void *arg) {
    int i;
    uint total_count = 0;
    const int iter = 100000;
    int thread_count = (intptr_t)arg;

    event_wait(&context_switch_event);

    uint count = arch_cycle_count();
    for (i = 0; i < iter; i++) {
        thread_yield();
    }
    total_count += arch_cycle_count() - count;
    thread_sleep(1000);
    printf("took %u cycles to yield %d times, %u per yield, %u per yield per thread\n",
           total_count, iter, total_count / iter, total_count / iter / thread_count);

    event_signal(&context_switch_done_event, true);

    return 0;
}

static void context_switch_test(void) {
    event_init(&context_switch_event, false, 0);
    event_init(&context_switch_done_event, false, 0);

    thread_detach_and_resume(thread_create("context switch idle", &context_switch_tester, (void *)1, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE));
    thread_sleep(100);
    event_signal(&context_switch_event, true);
    event_wait(&context_switch_done_event);
    thread_sleep(100);

    event_unsignal(&context_switch_event);
    event_unsignal(&context_switch_done_event);
    thread_detach_and_resume(thread_create("context switch 2a", &context_switch_tester, (void *)2, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE));
    thread_detach_and_resume(thread_create("context switch 2b", &context_switch_tester, (void *)2, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE));
    thread_sleep(100);
    event_signal(&context_switch_event, true);
    event_wait(&context_switch_done_event);
    thread_sleep(100);

    event_unsignal(&context_switch_event);
    event_unsignal(&context_switch_done_event);
    thread_detach_and_resume(thread_create("context switch 4a", &context_switch_tester, (void *)4, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE));
    thread_detach_and_resume(thread_create("context switch 4b", &context_switch_tester, (void *)4, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE));
    thread_detach_and_resume(thread_create("context switch 4c", &context_switch_tester, (void *)4, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE));
    thread_detach_and_resume(thread_create("context switch 4d", &context_switch_tester, (void *)4, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE));
    thread_sleep(100);
    event_signal(&context_switch_event, true);
    event_wait(&context_switch_done_event);
    thread_sleep(100);
}

static volatile int preempt_count;

static int preempt_tester(void *arg) {
    spin(1000000);

    printf("exiting ts %lld\n", current_time_hires());

    atomic_add(&preempt_count, -1);
#undef COUNT

    return 0;
}

static void preempt_test(void) {
    /* create 5 threads, let them run. If the system is properly timer preempting,
     * the threads should interleave each other at a fine enough granularity so
     * that they complete at roughly the same time. */
    printf("testing preemption\n");

    preempt_count = 5;

    for (int i = 0; i < preempt_count; i++)
        thread_detach_and_resume(thread_create("preempt tester", &preempt_tester, NULL, LOW_PRIORITY, DEFAULT_STACK_SIZE));

    while (preempt_count > 0) {
        thread_sleep(1000);
    }

    printf("done with preempt test, above time stamps should be very close\n");

    /* do the same as above, but mark the threads as real time, which should
     * effectively disable timer based preemption for them. They should
     * complete in order, about a second apart. */
    printf("testing real time preemption\n");

    preempt_count = 5;

    for (int i = 0; i < preempt_count; i++) {
        thread_t *t = thread_create("preempt tester", &preempt_tester, NULL, LOW_PRIORITY, DEFAULT_STACK_SIZE);
        thread_set_real_time(t);
        thread_detach_and_resume(t);
    }

    while (preempt_count > 0) {
        thread_sleep(1000);
    }

    printf("done with real-time preempt test, above time stamps should be 1 second apart\n");
}

static void spinlock_test(void) {
    arch_interrupt_saved_state_t state;
    spin_lock_t lock;

    spin_lock_init(&lock);

    // verify basic functionality (single core)
    printf("testing spinlock:\n");
    ASSERT(!spin_lock_held(&lock));
    ASSERT(!arch_ints_disabled());
    state = spin_lock_irqsave(&lock);
    ASSERT(arch_ints_disabled());
    ASSERT(spin_lock_held(&lock));
    spin_unlock_irqrestore(&lock, state);
    ASSERT(!spin_lock_held(&lock));
    ASSERT(!arch_ints_disabled());
    printf("seems to work\n");

#define COUNT (1024*1024)
    state = arch_interrupt_save();
    uint32_t c = arch_cycle_count();
    for (uint i = 0; i < COUNT; i++) {
        spin_lock(&lock);
        spin_unlock(&lock);
    }
    c = arch_cycle_count() - c;
    arch_interrupt_restore(state);

    printf("%u cycles to acquire/release lock %u times (%u cycles per)\n", c, COUNT, c / COUNT);

    c = arch_cycle_count();
    for (uint i = 0; i < COUNT; i++) {
        state = spin_lock_irqsave(&lock);
        spin_unlock_irqrestore(&lock, state);
    }
    c = arch_cycle_count() - c;

    printf("%u cycles to acquire/release lock w/irqsave %u times (%u cycles per)\n", c, COUNT, c / COUNT);
#undef COUNT
}

static int thread_bench(int argc, const console_cmd_args *argv) {
    spinlock_test();

    thread_sleep(200);
    context_switch_test();

    preempt_test();

    return 0;
}

STATIC_COMMAND_START
STATIC_COMMAND("thread_tests", "scheduler benchmarks", &thread_bench)
STATIC_COMMAND_END(thread_tests);

#endif // WITH_LIB_CONSOLE
