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
#include <stm32f2xx_rcc.h>
#include <stm32f2xx_tim.h>
#include <misc.h>
#include <arch/arm/cm.h>

#define LOCAL_TRACE 0

static void stm32_tim_irq(uint num)
{
    TRACEF("tim irq %d\n", num);
    PANIC_UNIMPLEMENTED;
}

void stm32_TIM3_IRQ(void)
{
    stm32_tim_irq(3);
}

void stm32_TIM4_IRQ(void)
{
    stm32_tim_irq(4);
}

void stm32_TIM5_IRQ(void)
{
    stm32_tim_irq(5);
}

void stm32_TIM6_IRQ(void)
{
    stm32_tim_irq(6);
}

void stm32_TIM7_IRQ(void)
{
    stm32_tim_irq(7);
}

/* time base */
void stm32_TIM2_IRQ(void)
{
    stm32_tim_irq(2);
}

void stm32_timer_early_init(void)
{
}

void stm32_timer_init(void)
{
}
