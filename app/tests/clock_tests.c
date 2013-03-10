/*
 * Copyright (c) 2012 Travis Geiselbrecht
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
#include <rand.h>
#include <err.h>
#include <app/tests.h>
#include <kernel/thread.h>
#include <kernel/mutex.h>
#include <kernel/semaphore.h>
#include <kernel/event.h>
#include <platform.h>

void clock_tests(void)
{
	uint32_t c;
	lk_time_t t;
	lk_bigtime_t t2;

	thread_sleep(100);
	c = arch_cycle_count();
	t = current_time();
	c = arch_cycle_count() - c;
	printf("%u cycles per current_time()\n", c);

	thread_sleep(100);
	c = arch_cycle_count();
	t2 = current_time_hires();
	c = arch_cycle_count() - c;
	printf("%u cycles per current_time_hires()\n", c);

	printf("making sure time never goes backwards\n");
	{
		printf("testing current_time()\n");
		lk_time_t start = current_time();
		lk_time_t last = start;
		for (;;) {
			t = current_time();
			//printf("%lu %lu\n", last, t);
			if (TIME_LT(t, last)) {
				printf("WARNING: time ran backwards: %lu < %lu\n", last, t);
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
				printf("WARNING: time ran backwards: %llu < %llu\n", last, t2);
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
			if (t > (t2 / 1000)) {
				printf("WARNING: current_time() ahead of current_time_hires() %lu %llu\n", t, t2);
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
		uint cycles = arch_cycle_count();
		lk_bigtime_t start = current_time_hires();
		while ((current_time_hires() - start) < 1000000)
			;
		cycles = arch_cycle_count() - cycles;
		printf("%u cycles per second\n", cycles);
	}
}

