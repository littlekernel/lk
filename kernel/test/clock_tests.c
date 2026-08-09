/*
 * Copyright (c) 2012-2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
/*
 * Validates the platform clock contract the rest of the kernel relies on:
 * current_time() and current_time_hires() never run backwards, actually
 * advance, and tick from the same base. The cycle count and clock frequency
 * benchmarks that used to live alongside these checks remain a console
 * command in app/tests.
 */
#include <lib/unittest.h>

#include <platform.h>
#include <sys/types.h>

/* How long each observation loop watches the clock. Long enough to span many
 * timer ticks on every platform, short enough to stay cheap in the boot time
 * CI budget. */
#define CLOCK_TEST_WINDOW_MS 500
#define CLOCK_TEST_WINDOW_US (CLOCK_TEST_WINDOW_MS * 1000)

/* A clock that returns the same value this many times in a row is stuck. Even
 * the fastest host needs a few nanoseconds per read, so this many identical
 * samples spans well over any legitimate tick period, while a broken clock
 * trips it instead of hanging the observation loop (and the CI run) forever. */
#define CLOCK_STUCK_READS (10 * 1000 * 1000)

static bool test_current_time_monotonic(void) {
    BEGIN_TEST;

    const lk_time_t start = current_time();
    lk_time_t last = start;
    int same = 0;
    while (last - start < CLOCK_TEST_WINDOW_MS) {
        const lk_time_t now = current_time();
        ASSERT_FALSE(TIME_LT(now, last), "current_time went backwards");
        same = (now == last) ? same + 1 : 0;
        ASSERT_LT(same, CLOCK_STUCK_READS, "current_time is not advancing");
        last = now;
    }

    END_TEST;
}

static bool test_current_time_hires_monotonic(void) {
    BEGIN_TEST;

    const lk_bigtime_t start = current_time_hires();
    lk_bigtime_t last = start;
    int same = 0;
    while (last - start < CLOCK_TEST_WINDOW_US) {
        const lk_bigtime_t now = current_time_hires();
        ASSERT_FALSE(now < last, "current_time_hires went backwards");
        same = (now == last) ? same + 1 : 0;
        ASSERT_LT(same, CLOCK_STUCK_READS, "current_time_hires is not advancing");
        last = now;
    }

    END_TEST;
}

/* The two clocks must tick from the same base at the same rate: a millisecond
 * sample bracketed by two microsecond samples has to land inside the bracket,
 * give or take the coarser clock's tick granularity. */
#define CLOCK_BASE_SLOP_MS 10

static bool test_clocks_same_base(void) {
    BEGIN_TEST;

    const lk_bigtime_t start = current_time_hires();
    int iterations = 0;
    while (current_time_hires() - start < CLOCK_TEST_WINDOW_US &&
           iterations++ < CLOCK_STUCK_READS) {
        const lk_bigtime_t before = current_time_hires();
        const lk_time_t t = current_time();
        const lk_bigtime_t after = current_time_hires();

        const lk_time_t low = (lk_time_t)(before / 1000);
        const lk_time_t high = (lk_time_t)(after / 1000);
        if (TIME_LT(t + CLOCK_BASE_SLOP_MS, low)) {
            UNITTEST_FAIL_TRACEF("current_time %u lags current_time_hires %llu\n", t, before);
            all_ok = false;
            break;
        }
        if (TIME_GT(t, high + CLOCK_BASE_SLOP_MS)) {
            UNITTEST_FAIL_TRACEF("current_time %u is ahead of current_time_hires %llu\n", t, after);
            all_ok = false;
            break;
        }
    }

    END_TEST;
}

BEGIN_TEST_CASE(clock_tests)
RUN_TEST(test_current_time_monotonic);
RUN_TEST(test_current_time_hires_monotonic);
RUN_TEST(test_clocks_same_base);
END_TEST_CASE(clock_tests)
