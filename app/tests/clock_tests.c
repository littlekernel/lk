/*
 * Copyright (c) 2012 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <stdio.h>
#include <rand.h>
#include <lk/err.h>
#include <app/tests.h>
#include <kernel/thread.h>
#include <kernel/mutex.h>
#include <kernel/semaphore.h>
#include <kernel/event.h>
#include <platform.h>

int clock_tests(int argc, const console_cmd_args *argv) {
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

    printf("making sure time never goes backwards\n");
    {
        printf("testing current_time()\n");
        lk_time_t start = current_time();
        lk_time_t last = start;
        for (;;) {
            t = current_time();
            //printf("%lu %lu\n", last, t);
            if (TIME_LT(t, last)) {
                printf("WARNING: time ran backwards: %u < %u\n", t, last);
                last = t;
                continue;
            }
            last = t;
            if (last - start > 5000)
                break;
        }
    }
    {
        printf("testing current_time_hires()\n");
        lk_bigtime_t start = current_time_hires();
        lk_bigtime_t last = start;
        for (;;) {
            t2 = current_time_hires();
            //printf("%llu %llu\n", last, t2);
            if (t2 < last) {
                printf("WARNING: time ran backwards: %llu < %llu\n", t2, last);
                last = t2;
                continue;
            }
            last = t2;
            if (last - start > 5000000)
                break;
        }
    }

    printf("making sure current_time() and current_time_hires() are always the same base\n");
    {
        lk_time_t start = current_time();
        for (;;) {
            t = current_time();
            t2 = current_time_hires();
            if (t > ((t2 + 500) / 1000)) {
                printf("WARNING: current_time() ahead of current_time_hires() %u %llu\n", t, t2);
            }
            if (t - start > 5000)
                break;
        }
    }

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
