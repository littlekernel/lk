/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
/*
 * The kill signal and the interruptible waits (WAIT_QUEUE_INTERRUPTIBLE).
 *
 * A signal is never cleared, so only throwaway threads are signaled, never the
 * thread running the tests. Each helper reports through its return code and
 * touches nothing on the test's stack, so a helper outliving a failed join
 * corrupts nothing. Waits that a kill should cut short are long, so a kill
 * that fails shows up as a join that times out.
 */
#include <lib/unittest.h>

#include <kernel/event.h>
#include <kernel/thread.h>
#include <kernel/wait.h>
#include <lk/err.h>
#include <platform.h>

#define LONG_WAIT_MS 10000
#define JOIN_MS 5000

static event_t never_signaled = EVENT_INITIAL_VALUE(never_signaled, false, 0);
static event_t go = EVENT_INITIAL_VALUE(go, false, 0);

static thread_t *start(const char *name, thread_start_routine fn, void *arg) {
    thread_t *t = thread_create(name, fn, arg, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
    if (t) {
        thread_resume(t);
    }
    return t;
}

// Whether t reached THREAD_BLOCKED within a second.
static bool wait_until_blocked(thread_t *t) {
    for (int i = 0; i < 1000; i++) {
        THREAD_LOCK(state);
        const bool blocked = t->state == THREAD_BLOCKED;
        THREAD_UNLOCK(state);
        if (blocked) {
            return true;
        }
        thread_sleep(1);
    }
    return false;
}

static int event_wait_interruptible(void *arg) {
    return event_wait_etc(&never_signaled, (lk_time_t)(uintptr_t)arg, WAIT_QUEUE_INTERRUPTIBLE);
}

// A kill ends an interruptible wait, with or without a timeout armed.
static bool kill_ends_interruptible_wait(void) {
    BEGIN_TEST;

    const lk_time_t timeouts[] = { INFINITE_TIME, LONG_WAIT_MS };
    for (size_t i = 0; i < countof(timeouts); i++) {
        thread_t *t = start("kill wait", event_wait_interruptible, (void *)(uintptr_t)timeouts[i]);
        ASSERT_NONNULL(t, "thread_create");
        ASSERT_TRUE(wait_until_blocked(t), "helper blocks");

        thread_signal_kill(t);
        int ret = 0;
        ASSERT_EQ(NO_ERROR, thread_join(t, &ret, JOIN_MS), "the kill ends the wait");
        EXPECT_EQ(ERR_INTERRUPTED, ret, "the wait's status");
    }

    END_TEST;
}

// Kills itself first, then tries to wait.
static int wait_with_kill_pending(void *arg) {
    thread_signal_kill(get_current_thread());
    if (!(thread_signals(get_current_thread()) & THREAD_SIGNAL_KILL)) {
        return ERR_GENERIC;
    }
    return event_wait_etc(&never_signaled, LONG_WAIT_MS, WAIT_QUEUE_INTERRUPTIBLE);
}

// A kill already latched refuses an interruptible wait without blocking.
static bool pending_kill_refuses_wait(void) {
    BEGIN_TEST;

    const lk_time_t before = current_time();
    thread_t *t = start("kill pending", wait_with_kill_pending, NULL);
    ASSERT_NONNULL(t, "thread_create");
    int ret = 0;
    ASSERT_EQ(NO_ERROR, thread_join(t, &ret, JOIN_MS), "join");
    EXPECT_EQ(ERR_INTERRUPTED, ret, "the wait's status");
    EXPECT_LT(current_time() - before, (lk_time_t)LONG_WAIT_MS, "it never blocked for long");

    END_TEST;
}

static int sleep_interruptible(void *arg) {
    return thread_sleep_etc((lk_time_t)(uintptr_t)arg, WAIT_QUEUE_INTERRUPTIBLE);
}

// An interruptible sleep ends early when killed and runs its full length when not.
static bool interruptible_sleep(void) {
    BEGIN_TEST;

    thread_t *t = start("kill sleep", sleep_interruptible, (void *)(uintptr_t)LONG_WAIT_MS);
    ASSERT_NONNULL(t, "thread_create");
    ASSERT_TRUE(wait_until_blocked(t), "helper sleeps on a wait queue");
    thread_signal_kill(t);
    int ret = 0;
    ASSERT_EQ(NO_ERROR, thread_join(t, &ret, JOIN_MS), "the kill ends the sleep");
    EXPECT_EQ(ERR_INTERRUPTED, ret, "the sleep's status");

    const lk_time_t delay = 20;
    const lk_time_t before = current_time();
    t = start("full sleep", sleep_interruptible, (void *)(uintptr_t)delay);
    ASSERT_NONNULL(t, "thread_create");
    ASSERT_EQ(NO_ERROR, thread_join(t, &ret, JOIN_MS), "join");
    EXPECT_EQ(NO_ERROR, ret, "an unkilled sleep's status");
    EXPECT_GE(current_time() - before, delay, "slept the whole delay");

    END_TEST;
}

// Kills itself, then waits without the flag: each wait runs as if no signal
// were there. Returns 0 when all of them did.
static int uninterruptible_waits(void *arg) {
    thread_t *self = get_current_thread();
    thread_signal_kill(self);

    lk_time_t before = current_time();
    if (event_wait_timeout(&never_signaled, 30) != ERR_TIMED_OUT || current_time() - before < 30) {
        return -1;
    }
    before = current_time();
    if (thread_sleep_etc(30, 0) != NO_ERROR || current_time() - before < 30) {
        return -2;
    }
    // released by the test, so this wait is woken rather than timed out
    if (event_wait_etc(&go, LONG_WAIT_MS, 0) != NO_ERROR) {
        return -3;
    }
    return (thread_signals(self) & THREAD_SIGNAL_KILL) ? 0 : -4;
}

// Waits without WAIT_QUEUE_INTERRUPTIBLE never see the signal, which stays latched.
static bool uninterruptible_waits_ignore_kill(void) {
    BEGIN_TEST;

    event_unsignal(&go);
    thread_t *t = start("no kill", uninterruptible_waits, NULL);
    ASSERT_NONNULL(t, "thread_create");
    thread_sleep(100);
    event_signal(&go, true);

    int ret = 1;
    ASSERT_EQ(NO_ERROR, thread_join(t, &ret, JOIN_MS), "join");
    EXPECT_EQ(0, ret, "every wait ran as without a signal");
    event_unsignal(&go);

    END_TEST;
}

// Spins until released, then reports whether the kill sent meanwhile is latched.
static int spin_until_go(void *arg) {
    volatile bool *released = (volatile bool *)arg;
    while (!*released) {
    }
    return (thread_signals(get_current_thread()) & THREAD_SIGNAL_KILL) ? 0 : -1;
}

static volatile bool spinner_released;

// A thread killed while it runs (on another cpu, when there is one) carries on
// with the signal latched.
static bool kill_while_running_latches(void) {
    BEGIN_TEST;

    spinner_released = false;
    thread_t *t = start("kill spin", spin_until_go, (void *)&spinner_released);
    ASSERT_NONNULL(t, "thread_create");
    thread_sleep(10);
    thread_signal_kill(t);
    thread_sleep(10);
    spinner_released = true;

    int ret = 1;
    ASSERT_EQ(NO_ERROR, thread_join(t, &ret, JOIN_MS), "join");
    EXPECT_EQ(0, ret, "the signal is latched");

    END_TEST;
}

static int event_wait_go_interruptible(void *arg) {
    return event_wait_etc(&go, LONG_WAIT_MS, WAIT_QUEUE_INTERRUPTIBLE);
}

// An interruptible wait that nobody kills is woken as any other.
static bool interruptible_wait_wakes_normally(void) {
    BEGIN_TEST;

    event_unsignal(&go);
    thread_t *t = start("wake", event_wait_go_interruptible, NULL);
    ASSERT_NONNULL(t, "thread_create");
    ASSERT_TRUE(wait_until_blocked(t), "helper blocks");
    event_signal(&go, true);

    int ret = 1;
    ASSERT_EQ(NO_ERROR, thread_join(t, &ret, JOIN_MS), "join");
    EXPECT_EQ(NO_ERROR, ret, "woken by the event");
    event_unsignal(&go);

    END_TEST;
}

BEGIN_TEST_CASE(kill_signal_tests)
RUN_TEST(kill_ends_interruptible_wait);
RUN_TEST(pending_kill_refuses_wait);
RUN_TEST(interruptible_sleep);
RUN_TEST(uninterruptible_waits_ignore_kill);
RUN_TEST(kill_while_running_latches);
RUN_TEST(interruptible_wait_wakes_normally);
END_TEST_CASE(kill_signal_tests)
