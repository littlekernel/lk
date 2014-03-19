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

void sam3_tc0_irq(void)
{
#if 0
	tc_get_status(TC0, 0);

	ticks += 0xffff;
#endif
}

void sam_timer_early_init(void)
{
#if 0
	pmc_enable_periph_clk(ID_TC0);


	uint32_t ul_div;
	uint32_t ul_tcclks;
	uint32_t ul_sysclk = MCLK; // sysclk_get_cpu_hz();

	tc_find_mck_divisor(100, ul_sysclk, &ul_div, &ul_tcclks, ul_sysclk);
	tc_init(TC0, 0, TC_CMR_TCCLKS_TIMER_CLOCK1 | TC_CMR_CPCTRG);
	tc_write_rc(TC0, 0, (ul_sysclk / ul_div) / 4);

	tc_find_mck_divisor(100, ul_sysclk, &ul_div, &ul_tcclks, ul_sysclk);
	tc_init(TC0, 0, TC_CMR_TCCLKS_TIMER_CLOCK1 | TC_CMR_CPCTRG);
	tc_write_rc(TC0, 0, 0xffff); // slowest we can run

	/* Configure and enable interrupt on RC compare */
	NVIC_SetPriority(ID_TC0, arm_cm_highest_priority());
	NVIC_EnableIRQ((IRQn_Type) ID_TC0);
	tc_enable_interrupt(TC0, 0, TC_IER_CPCS);
#endif

	tc_start(TC0, 0);

    arm_cm_systick_init(MCLK);
}

void sam_timer_init(void)
{
}


