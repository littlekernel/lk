/*
 * Copyright (c) 2008-2015 Travis Geiselbrecht
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
#include <debug.h>
#include <trace.h>
#include <rand.h>
#include <err.h>
#include <assert.h>
#include <string.h>
#include <app/tests.h>
#include <kernel/thread.h>
#include <kernel/mutex.h>
#include <kernel/semaphore.h>
#include <kernel/event.h>
#include <platform.h>

static int sleep_thread(void *arg)
{
    for (;;) {
        printf("sleeper %p\n", get_current_thread());
        thread_sleep(rand() % 500);
    }
    return 0;
}

int sleep_test(void)
{
    int i;
    for (i=0; i < 16; i++)
        thread_detach_and_resume(thread_create("sleeper", &sleep_thread, NULL, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE));
    return 0;
}

static semaphore_t sem;
static const int sem_total_its = 10000;
static const int sem_thread_max_its = 1000;
static const int sem_start_value = 10;
static int sem_remaining_its = 0;
static int sem_threads = 0;
static mutex_t sem_test_mutex;

static int semaphore_producer(void *unused)
{
    printf("semaphore producer %p starting up, running for %d iterations\n", get_current_thread(), sem_total_its);

    for (int x = 0; x < sem_total_its; x++) {
        sem_post(&sem, true);
    }

    return 0;
}

static int semaphore_consumer(void *unused)
{
    unsigned int iterations = 0;

    mutex_acquire(&sem_test_mutex);
    if (sem_remaining_its >= sem_thread_max_its) {
        iterations = rand();
        iterations %= sem_thread_max_its;
    } else {
        iterations = sem_remaining_its;
    }
    sem_remaining_its -= iterations;
    mutex_release(&sem_test_mutex);

    printf("semaphore consumer %p starting up, running for %u iterations\n", get_current_thread(), iterations);
    for (unsigned int x = 0; x < iterations; x++)
        sem_wait(&sem);
    printf("semaphore consumer %p done\n", get_current_thread());
    atomic_add(&sem_threads, -1);
    return 0;
}

static int semaphore_test(void)
{
    static semaphore_t isem = SEMAPHORE_INITIAL_VALUE(isem, 99);
    printf("preinitialized semaphore:\n");
    hexdump(&isem, sizeof(isem));

    sem_init(&sem, sem_start_value);
    mutex_init(&sem_test_mutex);

    sem_remaining_its = sem_total_its;
    while (1) {
        mutex_acquire(&sem_test_mutex);
        if (sem_remaining_its) {
            thread_detach_and_resume(thread_create("semaphore consumer", &semaphore_consumer, NULL, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE));
            atomic_add(&sem_threads, 1);
        } else {
            mutex_release(&sem_test_mutex);
            break;
        }
        mutex_release(&sem_test_mutex);
    }

    thread_detach_and_resume(thread_create("semaphore producer", &semaphore_producer, NULL, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE));

    while (sem_threads)
        thread_yield();

    if (sem.count == sem_start_value)
        printf("semaphore tests successfully complete\n");
    else
        printf("semaphore tests failed: %d != %d\n", sem.count, sem_start_value);

    sem_destroy(&sem);
    mutex_destroy(&sem_test_mutex);

    return 0;
}

static int mutex_thread(void *arg)
{
    int i;
    const int iterations = 1000000;

    static volatile int shared = 0;

    mutex_t *m = (mutex_t *)arg;

    printf("mutex tester thread %p starting up, will go for %d iterations\n", get_current_thread(), iterations);

    for (i = 0; i < iterations; i++) {
        mutex_acquire(m);

        if (shared != 0)
            panic("someone else has messed with the shared data\n");

        shared = (intptr_t)get_current_thread();
        thread_yield();
        shared = 0;

        mutex_release(m);
        thread_yield();
    }

    return 0;
}

static int mutex_timeout_thread(void *arg)
{
    mutex_t *timeout_mutex = (mutex_t *)arg;
    status_t err;

    printf("mutex_timeout_thread acquiring mutex %p with 1 second timeout\n", timeout_mutex);
    err = mutex_acquire_timeout(timeout_mutex, 1000);
    if (err == ERR_TIMED_OUT)
        printf("mutex_acquire_timeout returns with TIMEOUT\n");
    else
        printf("mutex_acquire_timeout returns %d\n", err);

    return err;
}

static int mutex_zerotimeout_thread(void *arg)
{
    mutex_t *timeout_mutex = (mutex_t *)arg;
    status_t err;

    printf("mutex_zerotimeout_thread acquiring mutex %p with zero second timeout\n", timeout_mutex);
    err = mutex_acquire_timeout(timeout_mutex, 0);
    if (err == ERR_TIMED_OUT)
        printf("mutex_acquire_timeout returns with TIMEOUT\n");
    else
        printf("mutex_acquire_timeout returns %d\n", err);

    return err;
}

int mutex_test(void)
{
    static mutex_t imutex = MUTEX_INITIAL_VALUE(imutex);
    printf("preinitialized mutex:\n");
    hexdump(&imutex, sizeof(imutex));

    mutex_t m;
    mutex_init(&m);

    thread_t *threads[5];

    for (uint i=0; i < countof(threads); i++) {
        threads[i] = thread_create("mutex tester", &mutex_thread, &m, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
        thread_resume(threads[i]);
    }

    for (uint i=0; i < countof(threads); i++) {
        thread_join(threads[i], NULL, INFINITE_TIME);
    }

    printf("done with simple mutex tests\n");

    printf("testing mutex timeout\n");

    mutex_t timeout_mutex;

    mutex_init(&timeout_mutex);
    mutex_acquire(&timeout_mutex);

    for (uint i=0; i < 2; i++) {
        threads[i] = thread_create("mutex timeout tester", &mutex_timeout_thread, (void *)&timeout_mutex, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
        thread_resume(threads[i]);
    }

    for (uint i=2; i < 4; i++) {
        threads[i] = thread_create("mutex timeout tester", &mutex_zerotimeout_thread, (void *)&timeout_mutex, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
        thread_resume(threads[i]);
    }

    thread_sleep(5000);
    mutex_release(&timeout_mutex);

    for (uint i=0; i < 4; i++) {
        thread_join(threads[i], NULL, INFINITE_TIME);
    }

    printf("done with mutex tests\n");

    mutex_destroy(&timeout_mutex);

    return 0;
}

static event_t e;

static int event_signaler(void *arg)
{
    printf("event signaler pausing\n");
    thread_sleep(1000);

//  for (;;) {
    printf("signaling event\n");
    event_signal(&e, true);
    printf("done signaling event\n");
    thread_yield();
//  }

    return 0;
}

static int event_waiter(void *arg)
{
    int count = (intptr_t)arg;

    printf("event waiter starting\n");

    while (count > 0) {
        printf("%p: waiting on event...\n", get_current_thread());
        if (event_wait(&e) < 0) {
            printf("%p: event_wait() returned error\n", get_current_thread());
            return -1;
        }
        printf("%p: done waiting on event...\n", get_current_thread());
        thread_yield();
        count--;
    }

    return 0;
}

void event_test(void)
{
    thread_t *threads[5];

    static event_t ievent = EVENT_INITIAL_VALUE(ievent, true, 0x1234);
    printf("preinitialized event:\n");
    hexdump(&ievent, sizeof(ievent));

    printf("event tests starting\n");

    /* make sure signaling the event wakes up all the threads */
    event_init(&e, false, 0);
    threads[0] = thread_create("event signaler", &event_signaler, NULL, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
    threads[1] = thread_create("event waiter 0", &event_waiter, (void *)2, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
    threads[2] = thread_create("event waiter 1", &event_waiter, (void *)2, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
    threads[3] = thread_create("event waiter 2", &event_waiter, (void *)2, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
    threads[4] = thread_create("event waiter 3", &event_waiter, (void *)2, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);

    for (uint i = 0; i < countof(threads); i++)
        thread_resume(threads[i]);

    thread_sleep(2000);
    printf("destroying event\n");
    event_destroy(&e);

    for (uint i = 0; i < countof(threads); i++)
        thread_join(threads[i], NULL, INFINITE_TIME);

    /* make sure signaling the event wakes up precisely one thread */
    event_init(&e, false, EVENT_FLAG_AUTOUNSIGNAL);
    threads[0] = thread_create("event signaler", &event_signaler, NULL, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
    threads[1] = thread_create("event waiter 0", &event_waiter, (void *)99, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
    threads[2] = thread_create("event waiter 1", &event_waiter, (void *)99, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
    threads[3] = thread_create("event waiter 2", &event_waiter, (void *)99, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
    threads[4] = thread_create("event waiter 3", &event_waiter, (void *)99, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);

    for (uint i = 0; i < countof(threads); i++)
        thread_resume(threads[i]);

    thread_sleep(2000);
    event_destroy(&e);

    for (uint i = 0; i < countof(threads); i++)
        thread_join(threads[i], NULL, INFINITE_TIME);

    printf("event tests done\n");
}

static int quantum_tester(void *arg)
{
    for (;;) {
        printf("%p: in this thread. rq %d\n", get_current_thread(), get_current_thread()->remaining_quantum);
    }
    return 0;
}

void quantum_test(void)
{
    thread_detach_and_resume(thread_create("quantum tester 0", &quantum_tester, NULL, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE));
    thread_detach_and_resume(thread_create("quantum tester 1", &quantum_tester, NULL, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE));
    thread_detach_and_resume(thread_create("quantum tester 2", &quantum_tester, NULL, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE));
    thread_detach_and_resume(thread_create("quantum tester 3", &quantum_tester, NULL, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE));
}

static event_t context_switch_event;
static event_t context_switch_done_event;

static int context_switch_tester(void *arg)
{
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

void context_switch_test(void)
{
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

static volatile int atomic;
static volatile int atomic_count;

static int atomic_tester(void *arg)
{
    int add = (intptr_t)arg;
    int i;

    const int iter = 10000000;

    TRACEF("add %d, %d iterations\n", add, iter);

    for (i=0; i < iter; i++) {
        atomic_add(&atomic, add);
    }

    int old = atomic_add(&atomic_count, -1);
    TRACEF("exiting, old count %d\n", old);

    return 0;
}

static void atomic_test(void)
{
    atomic = 0;
    atomic_count = 8;

    printf("testing atomic routines\n");

    thread_t *threads[8];
    threads[0] = thread_create("atomic tester 1", &atomic_tester, (void *)1, LOW_PRIORITY, DEFAULT_STACK_SIZE);
    threads[1] = thread_create("atomic tester 1", &atomic_tester, (void *)1, LOW_PRIORITY, DEFAULT_STACK_SIZE);
    threads[2] = thread_create("atomic tester 1", &atomic_tester, (void *)1, LOW_PRIORITY, DEFAULT_STACK_SIZE);
    threads[3] = thread_create("atomic tester 1", &atomic_tester, (void *)1, LOW_PRIORITY, DEFAULT_STACK_SIZE);
    threads[4] = thread_create("atomic tester 2", &atomic_tester, (void *)-1, LOW_PRIORITY, DEFAULT_STACK_SIZE);
    threads[5] = thread_create("atomic tester 2", &atomic_tester, (void *)-1, LOW_PRIORITY, DEFAULT_STACK_SIZE);
    threads[6] = thread_create("atomic tester 2", &atomic_tester, (void *)-1, LOW_PRIORITY, DEFAULT_STACK_SIZE);
    threads[7] = thread_create("atomic tester 2", &atomic_tester, (void *)-1, LOW_PRIORITY, DEFAULT_STACK_SIZE);

    /* start all the threads */
    for (uint i = 0; i < countof(threads); i++)
        thread_resume(threads[i]);

    /* wait for them to all stop */
    for (uint i = 0; i < countof(threads); i++) {
        thread_join(threads[i], NULL, INFINITE_TIME);
    }

    printf("atomic count == %d (should be zero)\n", atomic);
}

static volatile int preempt_count;

static int preempt_tester(void *arg)
{
    spin(1000000);

    printf("exiting ts %lld\n", current_time_hires());

    atomic_add(&preempt_count, -1);
#undef COUNT

    return 0;
}

static void preempt_test(void)
{
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

static int join_tester(void *arg)
{
    long val = (long)arg;

    printf("\t\tjoin tester starting\n");
    thread_sleep(500);
    printf("\t\tjoin tester exiting with result %ld\n", val);

    return val;
}

static int join_tester_server(void *arg)
{
    int ret;
    status_t err;
    thread_t *t;

    printf("\ttesting thread_join/thread_detach\n");

    printf("\tcreating and waiting on thread to exit with thread_join\n");
    t = thread_create("join tester", &join_tester, (void *)1, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
    thread_resume(t);
    ret = 99;
    printf("\tthread magic is 0x%x (should be 0x%x)\n", t->magic, THREAD_MAGIC);
    err = thread_join(t, &ret, INFINITE_TIME);
    printf("\tthread_join returns err %d, retval %d\n", err, ret);
    printf("\tthread magic is 0x%x (should be 0)\n", t->magic);

    printf("\tcreating and waiting on thread to exit with thread_join, after thread has exited\n");
    t = thread_create("join tester", &join_tester, (void *)2, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
    thread_resume(t);
    thread_sleep(1000); // wait until thread is already dead
    ret = 99;
    printf("\tthread magic is 0x%x (should be 0x%x)\n", t->magic, THREAD_MAGIC);
    err = thread_join(t, &ret, INFINITE_TIME);
    printf("\tthread_join returns err %d, retval %d\n", err, ret);
    printf("\tthread magic is 0x%x (should be 0)\n", t->magic);

    printf("\tcreating a thread, detaching it, let it exit on its own\n");
    t = thread_create("join tester", &join_tester, (void *)3, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
    thread_detach(t);
    thread_resume(t);
    thread_sleep(1000); // wait until the thread should be dead
    printf("\tthread magic is 0x%x (should be 0)\n", t->magic);

    printf("\tcreating a thread, detaching it after it should be dead\n");
    t = thread_create("join tester", &join_tester, (void *)4, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
    thread_resume(t);
    thread_sleep(1000); // wait until thread is already dead
    printf("\tthread magic is 0x%x (should be 0x%x)\n", t->magic, THREAD_MAGIC);
    thread_detach(t);
    printf("\tthread magic is 0x%x\n", t->magic);

    printf("\texiting join tester server\n");

    return 55;
}

static void join_test(void)
{
    int ret;
    status_t err;
    thread_t *t;

    printf("testing thread_join/thread_detach\n");

    printf("creating thread join server thread\n");
    t = thread_create("join tester server", &join_tester_server, (void *)1, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
    thread_resume(t);
    ret = 99;
    err = thread_join(t, &ret, INFINITE_TIME);
    printf("thread_join returns err %d, retval %d (should be 0 and 55)\n", err, ret);
}

static void spinlock_test(void)
{
    spin_lock_saved_state_t state;
    spin_lock_t lock;

    spin_lock_init(&lock);

    // verify basic functionality (single core)
    printf("testing spinlock:\n");
    ASSERT(!spin_lock_held(&lock));
    ASSERT(!arch_ints_disabled());
    spin_lock_irqsave(&lock, state);
    ASSERT(arch_ints_disabled());
    ASSERT(spin_lock_held(&lock));
    spin_unlock_irqrestore(&lock, state);
    ASSERT(!spin_lock_held(&lock));
    ASSERT(!arch_ints_disabled());
    printf("seems to work\n");

#define COUNT (1024*1024)
    uint32_t c = arch_cycle_count();
    for (uint i = 0; i < COUNT; i++) {
        spin_lock(&lock);
        spin_unlock(&lock);
    }
    c = arch_cycle_count() - c;

    printf("%u cycles to acquire/release lock %u times (%u cycles per)\n", c, COUNT, c / COUNT);

    c = arch_cycle_count();
    for (uint i = 0; i < COUNT; i++) {
        spin_lock_irqsave(&lock, state);
        spin_unlock_irqrestore(&lock, state);
    }
    c = arch_cycle_count() - c;

    printf("%u cycles to acquire/release lock w/irqsave %u times (%u cycles per)\n", c, COUNT, c / COUNT);
#undef COUNT
}

int thread_tests(void)
{
    mutex_test();
    semaphore_test();
    event_test();

    spinlock_test();
    atomic_test();

    thread_sleep(200);
    context_switch_test();

    preempt_test();

    join_test();

    return 0;
}

static int spinner_thread(void *arg)
{
    for (;;)
        ;

    return 0;
}

int spinner(int argc, const cmd_args *argv)
{
    if (argc < 2) {
        printf("not enough args\n");
        printf("usage: %s <priority> <rt>\n", argv[0].str);
        return -1;
    }

    thread_t *t = thread_create("spinner", spinner_thread, NULL, argv[1].u, DEFAULT_STACK_SIZE);
    if (!t)
        return ERR_NO_MEMORY;

    if (argc >= 3 && !strcmp(argv[2].str, "rt")) {
        thread_set_real_time(t);
    }
    thread_resume(t);

    return 0;
}
