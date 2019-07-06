/*
 * Copyright (c) 2012 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/debug.h>
#include <lk/trace.h>
#include <lk/err.h>
#include <sys/types.h>
#include <kernel/thread.h>
#include <platform.h>
#include <platform/timer.h>
#include <platform/stm32.h>
#include <arch/arm/cm.h>

#define LOCAL_TRACE 0

static void stm32_tim_irq(uint num) {
    TRACEF("tim irq %d\n", num);
    PANIC_UNIMPLEMENTED;
}

void stm32_TIM3_IRQ(void) {
    stm32_tim_irq(3);
}

void stm32_TIM4_IRQ(void) {
    stm32_tim_irq(4);
}

void stm32_TIM5_IRQ(void) {
    stm32_tim_irq(5);
}

void stm32_TIM6_IRQ(void) {
    stm32_tim_irq(6);
}

void stm32_TIM7_IRQ(void) {
    stm32_tim_irq(7);
}

/* time base */
void stm32_TIM2_IRQ(void) {
    stm32_tim_irq(2);
}

void stm32_timer_early_init(void) {
}

void stm32_timer_init(void) {
}
