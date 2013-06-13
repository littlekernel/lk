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
#include <trace.h>
#include <err.h>
#include <sys/types.h>
#include <kernel/thread.h>
#include <platform.h>
#include <platform/timer.h>
#include <arch/arm/cm.h>

#include <pmc/pmc.h>
#include <tc/tc.h>

#define MCLK 84000000 /* XXX read this */
#define TICK_RATE (MCLK / 2)

#define LOCAL_TRACE 0

static volatile uint64_t ticks = 0;

static platform_timer_callback cb;
static void *cb_args;

/* use systick as the kernel tick */
void _systick(void)
{
	arm_cm_irq_entry();

	bool resched = false;
	if (cb) {
		lk_time_t now = current_time();
		if (cb(cb_args, now) == INT_RESCHEDULE)
			resched = true;
	}

	arm_cm_irq_exit(resched);
}

status_t platform_set_periodic_timer(platform_timer_callback callback, void *arg, lk_time_t interval)
{
	LTRACEF("callback %p, arg %p, interval %u\n", callback, arg, (uint)interval);

	cb = callback;
	cb_args = arg;

	arm_cm_systick_set_periodic(/* XXX */ MCLK, interval);

	return NO_ERROR;
}

void sam3_tc0_irq(void)
{
	tc_get_status(TC0, 0);

	ticks += 0xffff;
}

lk_time_t current_time(void)
{
	return current_time_hires() / 1000;
}

lk_bigtime_t current_time_hires(void)
{
	uint64_t t;
	do {
		t = ticks;
		uint16_t delta = tc_get_cv(TC0, 0);

		if (ticks != t)
			continue;

		t += delta;
	} while (0);

	/* convert ticks to usec */
	lk_bigtime_t res = t / (TICK_RATE / 1000000);

	return res;
}

void sam_timer_early_init(void)
{
	pmc_enable_periph_clk(ID_TC0);


	uint32_t ul_div;
	uint32_t ul_tcclks;
	uint32_t ul_sysclk = MCLK; // sysclk_get_cpu_hz();

#if 0
	tc_find_mck_divisor(100, ul_sysclk, &ul_div, &ul_tcclks, ul_sysclk);
	tc_init(TC0, 0, TC_CMR_TCCLKS_TIMER_CLOCK1 | TC_CMR_CPCTRG);
	tc_write_rc(TC0, 0, (ul_sysclk / ul_div) / 4);
#endif

	tc_find_mck_divisor(100, ul_sysclk, &ul_div, &ul_tcclks, ul_sysclk);
	tc_init(TC0, 0, TC_CMR_TCCLKS_TIMER_CLOCK1 | TC_CMR_CPCTRG);
	tc_write_rc(TC0, 0, 0xffff); // slowest we can run

	/* Configure and enable interrupt on RC compare */
	NVIC_SetPriority(ID_TC0, arm_cm_highest_priority());
	NVIC_EnableIRQ((IRQn_Type) ID_TC0);
	tc_enable_interrupt(TC0, 0, TC_IER_CPCS);

	tc_start(TC0, 0);

}

void sam_timer_init(void)
{
}


