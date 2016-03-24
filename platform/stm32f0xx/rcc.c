
#include "platform/rcc.h"

#include <stm32f0xx.h>

static __IO uint32_t *stm32_rcc_get_clock_en_reg(stm32_rcc_clk_t clock) {
    switch (STM32_RCC_CLK_REG(clock)) {
        case STM32_RCC_REG_AHB:
            return &RCC->AHBENR;

        case STM32_RCC_REG_APB1:
            return &RCC->APB1ENR;

        case STM32_RCC_REG_APB2:
            return &RCC->APB2ENR;

        default:
            return NULL;
    }
}

static __IO uint32_t *stm32_rcc_get_clock_rst_reg(stm32_rcc_clk_t clock) {
    switch (STM32_RCC_CLK_REG(clock)) {
        case STM32_RCC_REG_AHB:
            return &RCC->AHBRSTR;

        case STM32_RCC_REG_APB1:
            return &RCC->APB1RSTR;

        case STM32_RCC_REG_APB2:
            return &RCC->APB2RSTR;

        default:
            return NULL;
    }
}

void stm32_rcc_set_enable(stm32_rcc_clk_t clock, bool enable) {
    __IO uint32_t *reg = stm32_rcc_get_clock_en_reg(clock);
    if (enable) {
        *reg |= 1 << STM32_RCC_CLK_INDEX(clock);
    } else {
        *reg &= ~(1 << STM32_RCC_CLK_INDEX(clock));
    }
}

void stm32_rcc_set_reset(stm32_rcc_clk_t clock, bool reset) {
    switch(clock) {
        // These clocks to not have reset bits.
        case STM32_RCC_CLK_DMA:
        case STM32_RCC_CLK_DMA2:
        case STM32_RCC_CLK_SRAM:
        case STM32_RCC_CLK_FLITF:
        case STM32_RCC_CLK_CRC:
            return;

        default:
            break;
    }

    __IO uint32_t *reg = stm32_rcc_get_clock_rst_reg(clock);
    if (reset) {
        *reg |= 1 << STM32_RCC_CLK_INDEX(clock);
    } else {
        *reg &= ~(1 << STM32_RCC_CLK_INDEX(clock));
    }
}

