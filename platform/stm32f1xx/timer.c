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
#include <err.h>
#include <sys/types.h>
#include <kernel/thread.h>
#include <platform.h>
#include <platform/timer.h>
#include <stm32f10x_rcc.h>
#include <stm32f10x_tim.h>
#include <misc.h>
#include <arch/arm/cm3.h>

#define TIME_BASE_COUNT 0xffff
#define TICK_RATE 1000000

static volatile uint64_t ticks = 0;

static platform_timer_callback cb;
static void *cb_args;

void stm32_tim2_irq(void)
{
	/* time base */
	ticks += TIME_BASE_COUNT;
	TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
}

void stm32_tim3_irq(void)
{
	inc_critical_section();

	TIM_ClearITPendingBit(TIM3, TIM_IT_Update);

	bool resched = false;
	if (cb) {
		time_t now = current_time();
		if (cb(cb_args, now) == INT_RESCHEDULE)
			resched = true;
	}

	if (resched) {
		// have the cortex-m3 queue a preemption
		cm3_trigger_preempt();
	}

	dec_critical_section();
}

static void stm32_tim_irq(uint num)
{
	printf("tim irq %d\n", num);
	PANIC_UNIMPLEMENTED;
}

void stm32_tim4_irq(void)
{
	stm32_tim_irq(4);
}

void stm32_tim5_irq(void)
{
	stm32_tim_irq(5);
}

void stm32_tim6_irq(void)
{
	stm32_tim_irq(6);
}

void stm32_tim7_irq(void)
{
	stm32_tim_irq(7);
}

time_t current_time(void)
{
	return current_time_hires() / 1000;
}

bigtime_t current_time_hires(void)
{
	bigtime_t res = 0;
	do {
		uint64_t t = ticks;
		uint16_t delta = TIM_GetCounter(TIM2);

		if (ticks != t)
			continue;

		res = t + delta;
	} while (0);

	return res;
}

status_t platform_set_periodic_timer(platform_timer_callback callback, void *arg, time_t interval)
{
	TRACEF("callback %p, arg %p, interval %d\n", callback, arg, interval);

	cb = callback;
	cb_args = arg;

	TIM_Cmd(TIM3, DISABLE);

	TIM_SetCounter(TIM3, interval);
	TIM_SetAutoreload(TIM3, interval);
	
	TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);
	TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
	NVIC_EnableIRQ(TIM3_IRQn);	

	TIM_Cmd(TIM3, ENABLE);

	return NO_ERROR;
}

void stm32_timer_early_init(void)
{
	/* start the time base unit */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

	TIM_TimeBaseInitTypeDef tbase;
	TIM_TimeBaseStructInit(&tbase);

	/* try to run the clock at 1Mhz */
    RCC_ClocksTypeDef clocks;
    RCC_GetClocksFreq(&clocks);

	// XXX why do we need a *2 here?
	tbase.TIM_Prescaler = (clocks.PCLK1_Frequency / 1000000) * 2 - 1;

	TIM_TimeBaseInit(TIM2, &tbase);

	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
	TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
	NVIC_SetPriority(TIM2_IRQn, cm3_highest_priority());
	NVIC_EnableIRQ(TIM2_IRQn);

	TIM_Cmd(TIM2, ENABLE);

	/* for dynamic ticks, use TIM3 */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

	/* run the tick at ms resolution */
	TIM_PrescalerConfig(TIM3, (clocks.PCLK1_Frequency / 1000) * 2 - 1, TIM_PSCReloadMode_Immediate);
	TIM_CounterModeConfig(TIM3, TIM_CounterMode_Down);
}

void stm32_timer_init(void)
{
}

