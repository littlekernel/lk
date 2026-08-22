/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

/* Scheduler scalability benchmark.
 *
 * fibo measures the scheduler, but it has no parallelism: there is never more
 * than one runnable thread, so with more than one cpu it measures the cost of
 * waking a remote cpu, not contention. This one gives every cpu its own
 * independent scheduler work and reports how throughput scales with the
 * number of cpus engaged. A global thread lock serializes the cpus and the
 * per-cpu rate falls as cpus are added; per-cpu locks should hold it flat.
 *
 * Modes, each run for 1..N cpus:
 *   local   one pair of threads per cpu, both pinned to it, ping-ponging
 *           through two events. Context switches and wait queue traffic with
 *           no ipis at all: the cleanest test of the locks.
 *   cross   the same pairs, but the two threads of a pair pinned to
 *           neighboring cpus, so every wake is a remote one. Measures the
 *           cross-cpu wake and ipi path, which is what fibo is bound by.
 *   spawn   each cpu creates, resumes and joins a thread pinned to itself,
 *           over and over: the thread lifecycle path (thread list lock,
 *           exit and reap) with real parallelism.
 *
 * usage: schedbench [mode|all] [iterations per thread] [max cpus]
 */
#include "tests.h"

#include <kernel/event.h>
#include <kernel/mp.h>
#include <kernel/thread.h>
#include <lk/err.h>
#include <platform.h>
#include <stdio.h>
#include <string.h>

#define MAX_PAIRS SMP_MAX_CPUS

struct pair {
    event_t to_a;
    event_t to_b;
    thread_t *a;
    thread_t *b;
    uint cpu_a;
    uint cpu_b;
};

static struct pair pairs[MAX_PAIRS];
static event_t start_gate;
static uint iterations;

static int pingpong_a(void *arg) {
    struct pair *p = arg;
    event_wait(&start_gate);
    for (uint i = 0; i < iterations; i++) {
        event_signal(&p->to_b);
        event_wait(&p->to_a);
    }
    return 0;
}

static int pingpong_b(void *arg) {
    struct pair *p = arg;
    event_wait(&start_gate);
    for (uint i = 0; i < iterations; i++) {
        event_wait(&p->to_b);
        event_signal(&p->to_a);
    }
    return 0;
}

static int spawn_child(void *arg) {
    return 0;
}

static int spawner(void *arg) {
    struct pair *p = arg;
    event_wait(&start_gate);
    for (uint i = 0; i < iterations; i++) {
        thread_t *t = thread_create("schedbench child", &spawn_child, NULL, DEFAULT_PRIORITY,
                                    DEFAULT_STACK_SIZE);
        if (!t) {
            printf("thread_create failed\n");
            return -1;
        }
        thread_set_pinned_cpu(t, (int)p->cpu_a);
        thread_resume(t);
        thread_join(t, NULL, INFINITE_TIME);
    }
    return 0;
}

/* the cpus that are up, in order, so that "n cpus" means a definite set */
static uint active_cpus(uint *cpus, uint max) {
    uint n = 0;
    for (uint i = 0; i < SMP_MAX_CPUS && n < max; i++) {
        if (mp_is_cpu_active(i)) {
            cpus[n++] = i;
        }
    }
    return n;
}

/* run |mode| on the first |ncpus| cpus and return the wall time in usecs */
static lk_bigtime_t run_once(const char *mode, uint ncpus, const uint *cpus) {
    const bool spawn = !strcmp(mode, "spawn");
    const bool cross = !strcmp(mode, "cross");

    event_init(&start_gate, false, 0);

    for (uint i = 0; i < ncpus; i++) {
        struct pair *p = &pairs[i];
        p->cpu_a = cpus[i];
        p->cpu_b = cross ? cpus[(i + 1) % ncpus] : cpus[i];
        event_init(&p->to_a, false, EVENT_FLAG_AUTOUNSIGNAL);
        event_init(&p->to_b, false, EVENT_FLAG_AUTOUNSIGNAL);

        if (spawn) {
            p->a = thread_create("schedbench spawner", &spawner, p, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
            p->b = NULL;
        } else {
            p->a = thread_create("schedbench a", &pingpong_a, p, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
            p->b = thread_create("schedbench b", &pingpong_b, p, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
        }
        thread_set_pinned_cpu(p->a, (int)p->cpu_a);
        thread_resume(p->a);
        if (p->b) {
            thread_set_pinned_cpu(p->b, (int)p->cpu_b);
            thread_resume(p->b);
        }
    }

    /* let everyone reach the gate, then open it and time to the last join */
    thread_sleep(20);
    lk_bigtime_t t = current_time_hires();
    event_signal(&start_gate);
    for (uint i = 0; i < ncpus; i++) {
        thread_join(pairs[i].a, NULL, INFINITE_TIME);
        if (pairs[i].b) {
            thread_join(pairs[i].b, NULL, INFINITE_TIME);
        }
    }
    t = current_time_hires() - t;

    for (uint i = 0; i < ncpus; i++) {
        event_destroy(&pairs[i].to_a);
        event_destroy(&pairs[i].to_b);
    }
    event_destroy(&start_gate);

    return t;
}

static void run_mode(const char *mode, uint maxcpus) {
    uint cpus[MAX_PAIRS];
    const uint n = active_cpus(cpus, maxcpus);

    /* one iteration is a round trip: two wakes and two switches for the
     * ping-pong modes, one create/resume/join for spawn */
    printf("schedbench %s: %u iterations per thread\n", mode, iterations);
    printf("  cpus   total ms   iters/ms   per cpu   scaling\n");

    lk_bigtime_t per_cpu_at_one = 0;
    for (uint ncpus = 1; ncpus <= n; ncpus++) {
        lk_bigtime_t us = run_once(mode, ncpus, cpus);
        if (us == 0) {
            us = 1;
        }
        /* rates in iterations per ms, x1000 for three digits of precision */
        const lk_bigtime_t total = (lk_bigtime_t)iterations * ncpus * 1000000ULL / us;
        const lk_bigtime_t per_cpu = total / ncpus;
        if (ncpus == 1) {
            per_cpu_at_one = per_cpu;
        }
        const lk_bigtime_t scale = per_cpu_at_one ? per_cpu * 100 / per_cpu_at_one : 0;
        printf("  %4u %10llu %7llu.%03llu %7llu.%03llu   %3llu.%02llu\n", ncpus,
               us / 1000, total / 1000, total % 1000, per_cpu / 1000, per_cpu % 1000,
               scale / 100, scale % 100);
    }
}

int schedbench(int argc, const console_cmd_args *argv) {
    const char *mode = (argc > 1) ? argv[1].str : "all";
    iterations = (argc > 2) ? argv[2].u : 10000;
    const uint maxcpus = (argc > 3) ? argv[3].u : SMP_MAX_CPUS;

    if (!strcmp(mode, "all")) {
        run_mode("local", maxcpus);
        run_mode("cross", maxcpus);
        run_mode("spawn", maxcpus);
    } else if (!strcmp(mode, "local") || !strcmp(mode, "cross") || !strcmp(mode, "spawn")) {
        run_mode(mode, maxcpus);
    } else {
        printf("usage: schedbench [local|cross|spawn|all] [iterations] [max cpus]\n");
        return ERR_INVALID_ARGS;
    }
    return NO_ERROR;
}
