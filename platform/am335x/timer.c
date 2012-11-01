/*
 * Copyright (c) 2012 Corey Tabaka
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
#include <sys/types.h>
#include <err.h>
#include <debug.h>
#include <kernel/thread.h>
#include <platform.h>
#include <platform/interrupts.h>
#include <platform/timer.h>
#include <platform/am335x.h>
#include "platform_p.h"

#include <soc_AM335x.h>
#include <dmtimer.h>
#include <hw_cm_per.h>
#include <hw_cm_dpll.h>
#include <hw_types.h>
#include <interrupt.h>

#define CLK_RATE (24000000)

static volatile lk_time_t current_ms;
static lk_time_t delta_ms;

static uint32_t tdlr;

static platform_timer_callback t_callback;

status_t platform_set_periodic_timer(platform_timer_callback callback, void *arg, lk_time_t interval)
{
	enter_critical_section();

	t_callback = callback;
	delta_ms = interval;

	DMTimerCounterSet(SOC_DMTIMER_2_REGS, tdlr);
	DMTimerReloadSet(SOC_DMTIMER_2_REGS, tdlr);
	DMTimerModeConfigure(SOC_DMTIMER_2_REGS, DMTIMER_AUTORLD_NOCMP_ENABLE);

	DMTimerIntEnable(SOC_DMTIMER_2_REGS, DMTIMER_INT_OVF_EN_FLAG);
	DMTimerEnable(SOC_DMTIMER_2_REGS);

	unmask_interrupt(SYS_INT_TINT2);

	exit_critical_section();

	return NO_ERROR;
}

lk_bigtime_t current_time_hires(void)
{
	lk_bigtime_t ret;
	lk_bigtime_t counter;

	enter_critical_section();
	ret = current_ms;
	exit_critical_section();

	counter = DMTimerCounterGet(SOC_DMTIMER_2_REGS);

	return ret * 1000 + (counter - tdlr) * 1000000 / CLK_RATE;
}

lk_time_t current_time(void)
{
	lk_time_t ret;
	lk_time_t counter;

	enter_critical_section();
	ret = current_ms;
	exit_critical_section();

	counter = DMTimerCounterGet(SOC_DMTIMER_2_REGS);

	return ret + (counter - tdlr) * 1000 / CLK_RATE;
}

static enum handler_return platform_tick(void *arg)
{
	enum handler_return ret;

	DMTimerIntDisable(SOC_DMTIMER_2_REGS, DMTIMER_INT_OVF_EN_FLAG);
	DMTimerIntStatusClear(SOC_DMTIMER_2_REGS, DMTIMER_INT_OVF_EN_FLAG);

	current_ms += delta_ms;

	if (t_callback) {
		ret = t_callback(arg, current_time());
	} else {
		ret = INT_NO_RESCHEDULE;
	}

	DMTimerEndOfInterrupt(SOC_DMTIMER_2_REGS);
	DMTimerIntEnable(SOC_DMTIMER_2_REGS, DMTIMER_INT_OVF_EN_FLAG);

	return ret;
}

void platform_init_timer(void)
{
	HWREG(SOC_CM_DPLL_REGS + CM_DPLL_CLKSEL_TIMER2_CLK) &=
	    ~(CM_DPLL_CLKSEL_TIMER2_CLK_CLKSEL);

	HWREG(SOC_CM_DPLL_REGS + CM_DPLL_CLKSEL_TIMER2_CLK) |=
	    CM_DPLL_CLKSEL_TIMER2_CLK_CLKSEL_CLK_M_OSC;

	while ((HWREG(SOC_CM_DPLL_REGS + CM_DPLL_CLKSEL_TIMER2_CLK) &
	        CM_DPLL_CLKSEL_TIMER2_CLK_CLKSEL) !=
	        CM_DPLL_CLKSEL_TIMER2_CLK_CLKSEL_CLK_M_OSC);

	HWREG(SOC_CM_PER_REGS + CM_PER_TIMER2_CLKCTRL) |=
	    CM_PER_TIMER2_CLKCTRL_MODULEMODE_ENABLE;

	while ((HWREG(SOC_CM_PER_REGS + CM_PER_TIMER2_CLKCTRL) &
	        CM_PER_TIMER2_CLKCTRL_MODULEMODE) != CM_PER_TIMER2_CLKCTRL_MODULEMODE_ENABLE);

	while (!(HWREG(SOC_CM_PER_REGS + CM_PER_L4LS_CLKSTCTRL) &
	         (CM_PER_L4LS_CLKSTCTRL_CLKACTIVITY_L4LS_GCLK |
	          CM_PER_L4LS_CLKSTCTRL_CLKACTIVITY_TIMER2_GCLK)));

	DMTimerResetConfigure(SOC_DMTIMER_2_REGS, DMTIMER_SFT_RESET_ENABLE);
	DMTimerReset(SOC_DMTIMER_2_REGS);

	register_int_handler(SYS_INT_TINT2, platform_tick, NULL);
}

