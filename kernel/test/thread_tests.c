/*
 * Copyright (c) 2008-2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
/*
 * Correctness tests for the core kernel synchronization primitives.
 *
 * These run as part of `ut all`, which is executed at boot by
 * scripts/run-qemu-boot-tests.py under a 60 second per architecture budget on
 * emulated targets. Keep the iteration counts small enough that the whole case
 * stays well under a second on a slow emulator, and prefer joins and events
 * over fixed sleeps. The cycle counting benchmarks that used to live alongside
 * these tests remain console commands in app/tests.
 */
#include <lib/unittest.h>

#include <lk/err.h>
#include <rand.h>
#include <arch/atomic.h>
#include <kernel/event.h>
#include <kernel/mutex.h>
#include <kernel/semaphore.h>
#include <kernel/thread.h>

/* -------------------------------------------------------------------------- */
/* mutex                                                                       */
/* -------------------------------------------------------------------------- */

#define MUTEX_THREADS 5
#define MUTEX_ITERATIONS 1000

static volatile int mutex_shared;
static volatile bool mutex_corrupted;

static int mutex_thread(void *arg) {
    mutex_t *m = (mutex_t *)arg;

    for (int i = 0; i < MUTEX_ITERATIONS; i++) {
        mutex_acquire(m);

        /* nobody else may be inside the mutex with us */
        if (mutex_shared != 0)
            mutex_corrupted = true;

        mutex_shared = (intptr_t)get_current_thread();
        thread_yield();
        mutex_shared = 0;

        mutex_release(m);
        thread_yield();
    }

    return 0;
}

static bool test_mutex_static_init(void) {
    BEGIN_TEST;

    static mutex_t imutex = MUTEX_INITIAL_VALUE(imutex);

    EXPECT_EQ(MUTEX_MAGIC, imutex.magic, "statically initialized mutex magic");
    EXPECT_EQ(0, imutex.count, "statically initialized mutex is not held");
    EXPECT_NULL(imutex.holder, "statically initialized mutex has no holder");

    END_TEST;
}

static bool test_mutex_mutual_exclusion(void) {
    BEGIN_TEST;

    mutex_t m;
    mutex_init(&m);

    mutex_shared = 0;
    mutex_corrupted = false;

    thread_t *threads[MUTEX_THREADS];
    for (uint i = 0; i < countof(threads); i++) {
        threads[i] = thread_create("mutex tester", &mutex_thread, &m,
                                   DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
        ASSERT_NONNULL(threads[i], "could not create mutex tester thread");
        thread_resume(threads[i]);
    }

    for (uint i = 0; i < countof(threads); i++) {
        thread_join(threads[i], NULL, INFINITE_TIME);
    }

    EXPECT_FALSE(mutex_corrupted, "two threads held the mutex at the same time");
    EXPECT_EQ(0, mutex_shared, "shared value left dirty");

    mutex_destroy(&m);

    END_TEST;
}

/* The hold time below is deliberately much longer than the timeout: a waiter
 * thread has until (hold - timeout) after its thread_resume() to actually reach
 * mutex_acquire_timeout(), or its timeout window extends past the release and
 * it acquires the mutex instead of timing out. On a heavily loaded emulator
 * that scheduling delay can be substantial, so keep the slack generous. */
#define MUTEX_TIMEOUT_MS 250
#define MUTEX_HOLD_MS 2000

static int mutex_timeout_thread(void *arg) {
    mutex_t *timeout_mutex = (mutex_t *)arg;
    return mutex_acquire_timeout(timeout_mutex, MUTEX_TIMEOUT_MS);
}

static int mutex_zerotimeout_thread(void *arg) {
    mutex_t *timeout_mutex = (mutex_t *)arg;
    return mutex_acquire_timeout(timeout_mutex, 0);
}

static bool test_mutex_timeout(void) {
    BEGIN_TEST;

    mutex_t timeout_mutex;
    mutex_init(&timeout_mutex);
    mutex_acquire(&timeout_mutex);

    thread_t *threads[4];
    for (uint i = 0; i < 2; i++) {
        threads[i] = thread_create("mutex timeout tester", &mutex_timeout_thread,
                                   &timeout_mutex, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
        ASSERT_NONNULL(threads[i], "could not create mutex timeout thread");
        thread_resume(threads[i]);
    }
    for (uint i = 2; i < 4; i++) {
        threads[i] = thread_create("mutex zerotimeout tester", &mutex_zerotimeout_thread,
                                   &timeout_mutex, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
        ASSERT_NONNULL(threads[i], "could not create mutex zero timeout thread");
        thread_resume(threads[i]);
    }

    /* hold the mutex past every waiter's timeout */
    thread_sleep(MUTEX_HOLD_MS);
    mutex_release(&timeout_mutex);

    for (uint i = 0; i < countof(threads); i++) {
        int retcode = NO_ERROR;
        thread_join(threads[i], &retcode, INFINITE_TIME);
        EXPECT_EQ(ERR_TIMED_OUT, retcode, "mutex_acquire_timeout should have timed out");
    }

    mutex_destroy(&timeout_mutex);

    END_TEST;
}

/* -------------------------------------------------------------------------- */
/* semaphore                                                                   */
/* -------------------------------------------------------------------------- */

#define SEM_TOTAL_ITS 2000
#define SEM_THREAD_MAX_ITS 200
#define SEM_START_VALUE 10
#define SEM_MAX_CONSUMERS 32

static semaphore_t sem;

static int semaphore_producer(void *unused) {
    for (int x = 0; x < SEM_TOTAL_ITS; x++) {
        sem_post(&sem, true);
    }
    return 0;
}

static int semaphore_consumer(void *arg) {
    unsigned int iterations = (unsigned int)(uintptr_t)arg;

    for (unsigned int x = 0; x < iterations; x++)
        sem_wait(&sem);

    return 0;
}

static bool test_semaphore_static_init(void) {
    BEGIN_TEST;

    static semaphore_t isem = SEMAPHORE_INITIAL_VALUE(isem, 99);

    EXPECT_EQ(SEMAPHORE_MAGIC, isem.magic, "statically initialized semaphore magic");
    EXPECT_EQ(99, isem.count, "statically initialized semaphore count");

    END_TEST;
}

static bool test_semaphore(void) {
    BEGIN_TEST;

    sem_init(&sem, SEM_START_VALUE);

    /* Hand out all SEM_TOTAL_ITS waits among a set of consumers, so that the
     * consumers between them wait exactly as many times as the producer posts.
     * The semaphore must therefore end up back at its starting value. */
    thread_t *consumers[SEM_MAX_CONSUMERS];
    uint consumer_count = 0;

    int remaining = SEM_TOTAL_ITS;
    while (remaining > 0) {
        /* the last slot soaks up whatever is left so every wait is accounted for */
        unsigned int iterations;
        if (consumer_count == countof(consumers) - 1 || remaining < SEM_THREAD_MAX_ITS) {
            iterations = remaining;
        } else {
            /* note: LK's rand() returns a full width value that may be
             * negative, so make it unsigned before taking the modulus */
            iterations = 1 + ((unsigned int)rand() % SEM_THREAD_MAX_ITS);
        }
        remaining -= iterations;

        consumers[consumer_count] = thread_create("semaphore consumer", &semaphore_consumer,
                                                  (void *)(uintptr_t)iterations,
                                                  DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
        ASSERT_NONNULL(consumers[consumer_count], "could not create semaphore consumer");
        thread_resume(consumers[consumer_count]);
        consumer_count++;
    }

    ASSERT_EQ(0, remaining, "did not distribute all the semaphore waits");

    thread_t *producer = thread_create("semaphore producer", &semaphore_producer, NULL,
                                       DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
    ASSERT_NONNULL(producer, "could not create semaphore producer");
    thread_resume(producer);

    for (uint i = 0; i < consumer_count; i++)
        thread_join(consumers[i], NULL, INFINITE_TIME);
    thread_join(producer, NULL, INFINITE_TIME);

    EXPECT_EQ(SEM_START_VALUE, sem.count, "semaphore did not return to its start value");

    sem_destroy(&sem);

    END_TEST;
}

/* -------------------------------------------------------------------------- */
/* event                                                                       */
/* -------------------------------------------------------------------------- */

#define EVENT_WAITERS 4

static event_t e;
static volatile int event_woken;

static int event_waiter(void *arg) {
    status_t err = event_wait(&e);
    if (err == NO_ERROR)
        atomic_add(&event_woken, 1);
    return err;
}

static bool test_event_static_init(void) {
    BEGIN_TEST;

    static event_t ievent = EVENT_INITIAL_VALUE(ievent, true, 0x1234);

    EXPECT_EQ(EVENT_MAGIC, ievent.magic, "statically initialized event magic");
    EXPECT_TRUE(ievent.signaled, "statically initialized event is signaled");
    EXPECT_EQ(0x1234u, ievent.flags, "statically initialized event flags");

    END_TEST;
}

/* A signal on an event without EVENT_FLAG_AUTOUNSIGNAL wakes every waiter. */
static bool test_event_broadcast(void) {
    BEGIN_TEST;

    event_init(&e, false, 0);
    event_woken = 0;

    thread_t *threads[EVENT_WAITERS];
    for (uint i = 0; i < countof(threads); i++) {
        threads[i] = thread_create("event waiter", &event_waiter, NULL,
                                   DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
        ASSERT_NONNULL(threads[i], "could not create event waiter");
        thread_resume(threads[i]);
    }

    event_signal(&e, true);

    for (uint i = 0; i < countof(threads); i++) {
        int retcode = -1;
        thread_join(threads[i], &retcode, INFINITE_TIME);
        EXPECT_EQ(NO_ERROR, retcode, "waiter did not wake cleanly");
    }

    EXPECT_EQ(EVENT_WAITERS, event_woken, "a broadcast event must wake every waiter");

    event_destroy(&e);

    END_TEST;
}

/* A signal on an EVENT_FLAG_AUTOUNSIGNAL event wakes exactly one waiter.
 *
 * This is the one test here whose correctness is gated on a sleep rather than a
 * join: there is no way to observe "every waiter is now blocked" or "no second
 * thread woke" other than to wait a while. If this ever flakes on a heavily
 * loaded or very slow emulator, the first sleep is the one to lengthen. */
static bool test_event_autounsignal(void) {
    BEGIN_TEST;

    event_init(&e, false, EVENT_FLAG_AUTOUNSIGNAL);
    event_woken = 0;

    thread_t *threads[EVENT_WAITERS];
    for (uint i = 0; i < countof(threads); i++) {
        threads[i] = thread_create("event waiter", &event_waiter, NULL,
                                   DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
        ASSERT_NONNULL(threads[i], "could not create event waiter");
        thread_resume(threads[i]);
    }

    /* let every waiter reach the event before signalling it */
    thread_sleep(100);

    event_signal(&e, true);

    /* give a wrongly woken second thread a chance to show up */
    thread_sleep(100);

    EXPECT_EQ(1, event_woken, "an autounsignal event must wake exactly one waiter");

    /* release the rest so they can be joined */
    event_destroy(&e);

    for (uint i = 0; i < countof(threads); i++)
        thread_join(threads[i], NULL, INFINITE_TIME);

    EXPECT_EQ(1, event_woken, "only one waiter may have woken with NO_ERROR");

    END_TEST;
}

/* -------------------------------------------------------------------------- */
/* atomics                                                                     */
/* -------------------------------------------------------------------------- */

#define ATOMIC_THREADS 8
#define ATOMIC_ITERATIONS 10000

static volatile int atomic_value;

static int atomic_tester(void *arg) {
    int add = (intptr_t)arg;

    for (int i = 0; i < ATOMIC_ITERATIONS; i++) {
        atomic_add(&atomic_value, add);
    }

    return 0;
}

static bool test_atomic_add(void) {
    BEGIN_TEST;

    atomic_value = 0;

    /* half the threads count up, half count down, by the same amount */
    thread_t *threads[ATOMIC_THREADS];
    for (uint i = 0; i < countof(threads); i++) {
        intptr_t add = (i < countof(threads) / 2) ? 1 : -1;
        threads[i] = thread_create("atomic tester", &atomic_tester, (void *)add,
                                   LOW_PRIORITY, DEFAULT_STACK_SIZE);
        ASSERT_NONNULL(threads[i], "could not create atomic tester thread");
    }

    for (uint i = 0; i < countof(threads); i++)
        thread_resume(threads[i]);

    for (uint i = 0; i < countof(threads); i++)
        thread_join(threads[i], NULL, INFINITE_TIME);

    EXPECT_EQ(0, atomic_value, "atomic adds did not cancel out");

    END_TEST;
}

/* -------------------------------------------------------------------------- */
/* thread join / detach                                                        */
/* -------------------------------------------------------------------------- */

static int join_tester(void *arg) {
    return (int)(intptr_t)arg;
}

/* Joining a thread must return its exit code, whether or not it has already
 * exited by the time we get around to joining it. Note that thread_join() frees
 * the thread structure, so the handle must not be touched afterwards. */
static bool test_thread_join(void) {
    BEGIN_TEST;

    thread_t *t = thread_create("join tester", &join_tester, (void *)1,
                                DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
    ASSERT_NONNULL(t, "could not create join tester thread");
    EXPECT_EQ(THREAD_MAGIC, t->magic, "new thread magic");
    thread_resume(t);

    int retcode = 99;
    status_t err = thread_join(t, &retcode, INFINITE_TIME);
    EXPECT_EQ(NO_ERROR, err, "thread_join failed");
    EXPECT_EQ(1, retcode, "thread_join returned the wrong exit code");

    END_TEST;
}

static bool test_thread_join_after_exit(void) {
    BEGIN_TEST;

    thread_t *t = thread_create("join tester", &join_tester, (void *)2,
                                DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
    ASSERT_NONNULL(t, "could not create join tester thread");
    thread_resume(t);

    /* let the thread run to completion before joining it */
    thread_sleep(100);

    int retcode = 99;
    status_t err = thread_join(t, &retcode, INFINITE_TIME);
    EXPECT_EQ(NO_ERROR, err, "joining an already exited thread failed");
    EXPECT_EQ(2, retcode, "thread_join returned the wrong exit code");

    END_TEST;
}

static bool test_thread_detach(void) {
    BEGIN_TEST;

    /* detach before the thread runs, then let it clean itself up */
    thread_t *t = thread_create("detach tester", &join_tester, (void *)3,
                                DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
    ASSERT_NONNULL(t, "could not create detach tester thread");
    EXPECT_EQ(NO_ERROR, thread_detach(t), "thread_detach failed");
    thread_resume(t);
    thread_sleep(100);

    /* and detach one that has already exited, which must also reap it */
    t = thread_create("detach tester", &join_tester, (void *)4,
                      DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
    ASSERT_NONNULL(t, "could not create detach tester thread");
    thread_resume(t);
    thread_sleep(100);
    EXPECT_EQ(THREAD_MAGIC, t->magic, "an unreaped exited thread keeps its magic");
    EXPECT_EQ(NO_ERROR, thread_detach(t), "detaching an exited thread failed");

    END_TEST;
}

BEGIN_TEST_CASE(thread_tests)
RUN_TEST(test_mutex_static_init);
RUN_TEST(test_mutex_mutual_exclusion);
RUN_TEST(test_mutex_timeout);
RUN_TEST(test_semaphore_static_init);
RUN_TEST(test_semaphore);
RUN_TEST(test_event_static_init);
RUN_TEST(test_event_broadcast);
RUN_TEST(test_event_autounsignal);
RUN_TEST(test_atomic_add);
RUN_TEST(test_thread_join);
RUN_TEST(test_thread_join_after_exit);
RUN_TEST(test_thread_detach);
END_TEST_CASE(thread_tests)
