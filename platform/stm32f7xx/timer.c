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
#include <platform/stm32.h>
#include <arch/arm/cm.h>

#define LOCAL_TRACE 0

/* terminal count of the timer, a nice even boundary that uses most of the 32bit counter */
#define WRAP_INTERVAL_US (4000000000)

static volatile lk_time_t ticks;

static TIM_HandleTypeDef timer2;

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    LTRACEF("WRAP\n");
    // wrapping the time base counter
    ticks += WRAP_INTERVAL_US / 1000;
}

void stm32_TIM2_IRQ(void)
{
    arm_cm_irq_entry();
    HAL_TIM_IRQHandler(&timer2);
    arm_cm_irq_exit(false);
}

void stm32_timer_early_init(void)
{
    uint32_t pclk2 = HAL_RCC_GetPCLK2Freq();

    /* timer 2 - timebase */
    __HAL_RCC_TIM2_CLK_ENABLE();
    timer2.Instance = TIM2;
    timer2.Init.Prescaler = (pclk2 / 1000000) - 1;
    timer2.Init.CounterMode = TIM_COUNTERMODE_UP;
    timer2.Init.Period = WRAP_INTERVAL_US;
    timer2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    timer2.Init.RepetitionCounter = 0;

    HAL_TIM_Base_Init(&timer2);

    HAL_TIM_Base_Start_IT(&timer2);
}

void stm32_timer_init(void)
{
    HAL_NVIC_EnableIRQ(TIM2_IRQn);
}

lk_time_t current_time(void)
{
    uint32_t t, delta;
    do {
        t = ticks;
        delta = __HAL_TIM_GET_COUNTER(&timer2);
        DMB;
    } while (ticks != t);

    return t + delta / 1000;
}

lk_bigtime_t current_time_hires(void)
{
    uint32_t t, delta;
    do {
        t = ticks;
        delta = __HAL_TIM_GET_COUNTER(&timer2);
        DMB;
    } while (ticks != t);

    return (lk_bigtime_t)t * 1000 + delta;
}

