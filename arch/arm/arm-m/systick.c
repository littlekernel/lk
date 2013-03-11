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
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <debug.h>
#include <kernel/thread.h>
#include <arch/arm.h>
#include <arch/arm/cm.h>

#define LOCAL_TRACE 0

void arm_cm_systick_init(void)
{
	NVIC_SetPriority(SysTick_IRQn, arm_cm_medium_priority());
}

void arm_cm_systick_set_periodic(uint32_t systick_clk_freq, lk_time_t period)
{
	LTRACEF("clk_freq %u, period %u\n", systick_clk_freq, (uint)period);

	uint32_t ticks = systick_clk_freq / (1000 / period);
	LTRACEF("ticks %d\n", ticks);

	SysTick->LOAD = (ticks & SysTick_LOAD_RELOAD_Msk) - 1;
	SysTick->VAL = 0;
	SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
}

void arm_cm_systick_cancel_periodic(void)
{
	SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
}

