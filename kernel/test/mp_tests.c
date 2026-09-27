/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

/* Tests for the cross cpu call in kernel/mp.c. Every test also runs on a
 * uniprocessor build, where the active mask is just cpu 0. */

#include <arch/atomic.h>
#include <arch/ops.h>
#include <kernel/mp.h>
#include <kernel/thread.h>
#include <lib/unittest.h>
#include <lk/err.h>

struct hit_counts {
    int per_cpu[SMP_MAX_CPUS];
    int bad_cpu;
};

static void count_hit(void *arg) {
    struct hit_counts *hits = arg;
    uint cpu = arch_curr_cpu_num();

    if (cpu >= SMP_MAX_CPUS) {
        atomic_add(&hits->bad_cpu, 1);
        return;
    }
    atomic_add(&hits->per_cpu[cpu], 1);
}

static int total_hits(const struct hit_counts *hits) {
    int total = 0;
    for (uint cpu = 0; cpu < SMP_MAX_CPUS; cpu++) {
        total += hits->per_cpu[cpu];
    }
    return total;
}

/* every active cpu runs the task exactly once, on itself */
static bool test_sync_exec_all(void) {
    BEGIN_TEST;

    mp_cpu_mask_t active = mp_get_active_mask();
    struct hit_counts hits = {};

    mp_sync_exec(MP_IPI_TARGET_ALL, 0, count_hit, &hits);

    for (uint cpu = 0; cpu < SMP_MAX_CPUS; cpu++) {
        int expected = (active & (1U << cpu)) ? 1 : 0;
        EXPECT_EQ(expected, hits.per_cpu[cpu], "hits on cpu");
    }
    EXPECT_EQ(0, hits.bad_cpu, "task saw an out of range cpu number");

    END_TEST;
}

/* one cpu at a time, both remote ones and whichever cpu this thread is on */
static bool test_sync_exec_single(void) {
    BEGIN_TEST;

    mp_cpu_mask_t active = mp_get_active_mask();

    for (uint cpu = 0; cpu < SMP_MAX_CPUS; cpu++) {
        if (!(active & (1U << cpu))) {
            continue;
        }

        struct hit_counts hits = {};
        mp_sync_exec(MP_IPI_TARGET_MASK, 1U << cpu, count_hit, &hits);

        EXPECT_EQ(1, hits.per_cpu[cpu], "hit on the target cpu");
        EXPECT_EQ(1, total_hits(&hits), "hits on other cpus");
    }

    END_TEST;
}

/* every active cpu but the one making the call */
static bool test_sync_exec_all_but_local(void) {
    BEGIN_TEST;

    mp_cpu_mask_t active = mp_get_active_mask();
    struct hit_counts hits = {};

    mp_sync_exec(MP_IPI_TARGET_ALL_BUT_LOCAL, 0, count_hit, &hits);

    EXPECT_EQ(__builtin_popcount(active) - 1, total_hits(&hits), "total hits");
    for (uint cpu = 0; cpu < SMP_MAX_CPUS; cpu++) {
        EXPECT_GE(1, hits.per_cpu[cpu], "hits on one cpu");
    }

    END_TEST;
}

/* a mask that names no active cpu runs nothing and still returns */
static bool test_sync_exec_inactive(void) {
    BEGIN_TEST;

    struct hit_counts hits = {};

    mp_sync_exec(MP_IPI_TARGET_MASK, ~mp_get_active_mask(), count_hit, &hits);
    EXPECT_EQ(0, total_hits(&hits), "hits");

    mp_sync_exec(MP_IPI_TARGET_MASK, 0, count_hit, &hits);
    EXPECT_EQ(0, total_hits(&hits), "hits");

    END_TEST;
}

/* mp_sync_exec_cpus() runs on exactly the cpus named, once each. Only cpus
 * that are running can be named here: a cpu that never starts would never
 * answer, and the call would not return. */
static bool test_sync_exec_cpus(void) {
    BEGIN_TEST;

    struct hit_counts hits = {};
    mp_sync_exec_cpus(mp_get_active_mask(), count_hit, &hits);
    EXPECT_EQ(__builtin_popcount(mp_get_active_mask()), total_hits(&hits), "hits");
    for (uint cpu = 0; cpu < SMP_MAX_CPUS; cpu++) {
        EXPECT_EQ(mp_is_cpu_active(cpu) ? 1 : 0, hits.per_cpu[cpu], "hits on one cpu");
    }

    struct hit_counts none = {};
    mp_sync_exec_cpus(0, count_hit, &none);
    EXPECT_EQ(0, total_hits(&none), "hits");

    END_TEST;
}

/* back to back calls must not lose or double count a target */
static bool test_sync_exec_repeat(void) {
    BEGIN_TEST;

    int expected = __builtin_popcount(mp_get_active_mask());
    int mismatches = 0;

    for (int i = 0; i < 100; i++) {
        struct hit_counts hits = {};
        mp_sync_exec(MP_IPI_TARGET_ALL, 0, count_hit, &hits);
        if (total_hits(&hits) != expected) {
            mismatches++;
        }
    }
    EXPECT_EQ(0, mismatches, "calls with a wrong hit count");

    END_TEST;
}

/* Several cpus issuing calls at each other at the same time. Each target
 * has to keep servicing the others' requests while waiting on its own. The
 * threads are pinned, so each one also knows which cpu ALL_BUT_LOCAL must skip. */
struct storm_worker {
    uint cpu;
    int mismatches;
};

static int storm_thread(void *arg) {
    struct storm_worker *w = arg;
    int active = __builtin_popcount(mp_get_active_mask());

    for (int i = 0; i < 25; i++) {
        struct hit_counts hits = {};
        mp_sync_exec(MP_IPI_TARGET_ALL, 0, count_hit, &hits);
        if (total_hits(&hits) != active || hits.per_cpu[w->cpu] != 1) {
            w->mismatches++;
        }

        struct hit_counts others = {};
        mp_sync_exec(MP_IPI_TARGET_ALL_BUT_LOCAL, 0, count_hit, &others);
        if (total_hits(&others) != active - 1 || others.per_cpu[w->cpu] != 0) {
            w->mismatches++;
        }
    }
    return 0;
}

static bool test_sync_exec_concurrent(void) {
    BEGIN_TEST;

    mp_cpu_mask_t active = mp_get_active_mask();
    struct storm_worker workers[SMP_MAX_CPUS] = {};
    thread_t *threads[SMP_MAX_CPUS] = {};

    for (uint cpu = 0; cpu < SMP_MAX_CPUS; cpu++) {
        if (!(active & (1U << cpu))) {
            continue;
        }
        workers[cpu].cpu = cpu;
        threads[cpu] = thread_create("mp_sync_storm", storm_thread, &workers[cpu],
                                     DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
        ASSERT_NONNULL(threads[cpu], "thread_create");
        thread_set_pinned_cpu(threads[cpu], cpu);
        thread_resume(threads[cpu]);
    }

    int mismatches = 0;
    for (uint cpu = 0; cpu < SMP_MAX_CPUS; cpu++) {
        if (threads[cpu]) {
            int retcode = -1;
            EXPECT_EQ(NO_ERROR, thread_join(threads[cpu], &retcode, 5000), "join");
            EXPECT_EQ(0, retcode, "thread return");
            mismatches += workers[cpu].mismatches;
        }
    }
    EXPECT_EQ(0, mismatches, "calls with a wrong hit count");

    END_TEST;
}

BEGIN_TEST_CASE(mp_tests)
RUN_TEST(test_sync_exec_all);
RUN_TEST(test_sync_exec_single);
RUN_TEST(test_sync_exec_all_but_local);
RUN_TEST(test_sync_exec_inactive);
RUN_TEST(test_sync_exec_cpus);
RUN_TEST(test_sync_exec_repeat);
RUN_TEST(test_sync_exec_concurrent);
END_TEST_CASE(mp_tests)
