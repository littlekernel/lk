/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <platform/rcc.h>

#include <lk/debug.h>
#include <stm32h5xx.h>

static __IO uint32_t *stm32_rcc_get_clock_en_reg(stm32_rcc_clk_t clock) {
    switch (STM32_RCC_CLK_REG(clock)) {
        case STM32_RCC_REG_AHB1:
            return &RCC->AHB1ENR;

        case STM32_RCC_REG_AHB2:
            return &RCC->AHB2ENR;

        case STM32_RCC_REG_AHB4:
            return &RCC->AHB4ENR;

        case STM32_RCC_REG_APB1L:
            return &RCC->APB1LENR;

        case STM32_RCC_REG_APB1H:
            return &RCC->APB1HENR;

        case STM32_RCC_REG_APB2:
            return &RCC->APB2ENR;

        case STM32_RCC_REG_APB3:
            return &RCC->APB3ENR;

        default:
            panic("unknown rcc clock register %u\n", STM32_RCC_CLK_REG(clock));
    }
}

void stm32_rcc_set_enable(stm32_rcc_clk_t clock, bool enable) {
    __IO uint32_t *reg = stm32_rcc_get_clock_en_reg(clock);
    if (enable) {
        *reg |= 1 << STM32_RCC_CLK_INDEX(clock);
    } else {
        *reg &= ~(1 << STM32_RCC_CLK_INDEX(clock));
    }

    /*
     * The reference manual calls for a read back of the enable register after
     * enabling a peripheral clock, to guarantee the clock is running before
     * the peripheral's registers are touched.
     */
    (void)*reg;
}
