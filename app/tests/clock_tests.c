/*
 * Copyright (c) 2012 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
/*
 * Clock benchmarks: these measure cycle counts and print timing for a human to
 * eyeball, so they have no pass/fail criterion and stay console commands. The
 * self validating clock invariant checks (monotonicity, common time base) live
 * in kernel/test/clock_tests.c and run under `ut all`.
 */
#include "tests.h"

#include <stdio.h>
#include <lk/err.h>
#include <kernel/thread.h>
#include <platform.h>

int clock_bench(int argc, const console_cmd_args *argv) {
    ulong c;
    lk_time_t t;
    lk_bigtime_t t2;

#define CYCLE_COUNT_TRIES 1000000
    thread_sleep(100);
    c = arch_cycle_count();
    for (int i = 0; i < CYCLE_COUNT_TRIES; i++) {
        t = current_time();
    }
    c = arch_cycle_count() - c;
    printf("%lu cycles per current_time()\n", c / CYCLE_COUNT_TRIES);

    thread_sleep(100);
    c = arch_cycle_count();
    for (int i = 0; i < CYCLE_COUNT_TRIES; i++) {
        t2 = current_time_hires();
    }
    c = arch_cycle_count() - c;
    printf("%lu cycles per current_time_hires()\n", c / CYCLE_COUNT_TRIES);

    (void)t;
    (void)t2;

    printf("counting to 5, in one second intervals\n");
    for (int i = 0; i < 5; i++) {
        thread_sleep(1000);
        printf("%d\n", i + 1);
    }

    printf("measuring cpu clock against current_time_hires()\n");
    for (int i = 0; i < 5; i++) {
        ulong cycles = arch_cycle_count();
        lk_bigtime_t start = current_time_hires();
        while ((current_time_hires() - start) < 1000000)
            ;
        cycles = arch_cycle_count() - cycles;
        printf("%lu cycles per second\n", cycles);
    }

    return NO_ERROR;
}
