/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <kernel/mp.h>
#include <kernel/thread.h>
#include <lib/unittest.h>
#include <lk/err.h>
#include <lk/trace.h>

#define LOCAL_TRACE 0

/* Scheduler placement tests.
 *
 * These exist mostly to exercise thread_set_pinned_cpu()'s migration path,
 * which has no other caller in the tree: everything else pins a thread before
 * resuming it, so the interesting case -- repinning a thread that is already
 * sitting on some cpu's run queue -- is never reached. That path decrements a
 * per-cpu runnable count and clears a per-cpu bitmap bit, and getting the cpu
 * wrong corrupts the other cpu's bookkeeping silently.
 */

static volatile bool worker_should_exit;
static volatile int worker_observed_cpu;

static int record_cpu_thread(void *arg) {
    while (!worker_should_exit) {
        worker_observed_cpu = arch_curr_cpu_num();
        thread_yield();
    }
    return 0;
}

/* Find a cpu that is active and is not `avoid`. Returns -1 if there isn't one. */
static int find_other_active_cpu(uint avoid) {
    for (uint i = 0; i < SMP_MAX_CPUS; i++) {
        if (i != avoid && mp_is_cpu_active(i)) {
            return (int)i;
        }
    }
    return -1;
}

static bool pin_before_resume(void) {
    BEGIN_TEST;

    const uint local = arch_curr_cpu_num();

    worker_should_exit = false;
    worker_observed_cpu = -1;

    thread_t *t = thread_create("pin_before_resume", record_cpu_thread, NULL,
                                DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
    ASSERT_NONNULL(t, "thread_create");

    thread_set_pinned_cpu(t, (int)local);
    ASSERT_EQ(NO_ERROR, thread_resume(t), "resume");

    /* give it a chance to run and notice where it is */
    for (int i = 0; i < 100 && worker_observed_cpu < 0; i++) {
        thread_sleep(10);
    }

    EXPECT_EQ((int)local, worker_observed_cpu, "a pinned thread must run on its cpu");

    worker_should_exit = true;
    int retcode = -1;
    thread_join(t, &retcode, INFINITE_TIME);

    END_TEST;
}

/* Repin a thread that is already runnable and queued, which is the case that
 * has to dequeue it from one cpu and enqueue it on another.
 *
 * Getting it reliably queued-but-not-running takes a little care: pin it to the
 * cpu this test is running on, at a lower priority than us. We are occupying
 * that cpu, so it cannot be scheduled, and pinning keeps any other cpu from
 * taking it. Then move it.
 */
static bool repin_runnable_thread(void) {
    BEGIN_TEST;

    const uint local = arch_curr_cpu_num();
    const int other = find_other_active_cpu(local);

    if (other < 0) {
        unittest_printf("[SKIPPED: needs more than one active cpu] ");
        END_TEST;
    }

    worker_should_exit = false;
    worker_observed_cpu = -1;

    thread_t *t = thread_create("repin_runnable", record_cpu_thread, NULL,
                                LOW_PRIORITY, DEFAULT_STACK_SIZE);
    ASSERT_NONNULL(t, "thread_create");

    /* pin it here, where we are already running at a higher priority, so it
     * goes onto this cpu's run queue and stays there */
    thread_set_pinned_cpu(t, (int)local);
    ASSERT_EQ(NO_ERROR, thread_resume(t), "resume");
    EXPECT_EQ(-1, worker_observed_cpu, "a lower priority thread must not have run yet");

    /* the migration under test: it is READY on this cpu's queue right now */
    thread_set_pinned_cpu(t, other);

    /* it should now be runnable on the other cpu, which is free to schedule it */
    for (int i = 0; i < 100 && worker_observed_cpu < 0; i++) {
        thread_sleep(10);
    }

    EXPECT_EQ(other, worker_observed_cpu,
              "a repinned runnable thread must move to its new cpu and run there");

    worker_should_exit = true;
    int retcode = -1;
    thread_join(t, &retcode, INFINITE_TIME);

    END_TEST;
}

/* Repin the running thread itself -- this one -- to another cpu and yield,
 * which is the idiom arch/x86/test uses to run cpuid on a specific cpu. The
 * yield has to requeue us on the new cpu rather than the local one, and we
 * must come back on the cpu we asked for, and then again on the original when
 * the pin is restored.
 */
static bool repin_current_thread(void) {
    BEGIN_TEST;

    thread_t *self = get_current_thread();
    const int old_pin = thread_pinned_cpu(self);
    const uint local = arch_curr_cpu_num();
    const int other = find_other_active_cpu(local);

    if (other < 0) {
        unittest_printf("[SKIPPED: needs more than one active cpu] ");
        END_TEST;
    }

    thread_set_pinned_cpu(self, other);
    thread_yield();
    EXPECT_EQ(other, (int)arch_curr_cpu_num(), "a repinned thread must yield onto its new cpu");

    /* and back, so the rest of the suite runs where it started */
    thread_set_pinned_cpu(self, (int)local);
    thread_yield();
    EXPECT_EQ((int)local, (int)arch_curr_cpu_num(), "and back again");

    thread_set_pinned_cpu(self, old_pin);

    END_TEST;
}

BEGIN_TEST_CASE(sched_tests)
RUN_TEST(pin_before_resume)
RUN_TEST(repin_runnable_thread)
RUN_TEST(repin_current_thread)
END_TEST_CASE(sched_tests)
