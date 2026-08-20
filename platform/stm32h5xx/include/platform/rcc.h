/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <stm32h5xx.h>

/*
 * A clock is identified by which enable register it lives in and the bit
 * index within that register, packed into a single value.
 */
enum {
    STM32_RCC_REG_AHB1 = 0,
    STM32_RCC_REG_AHB2 = 1,
    STM32_RCC_REG_AHB4 = 2,
    STM32_RCC_REG_APB1L = 3,
    STM32_RCC_REG_APB1H = 4,
    STM32_RCC_REG_APB2 = 5,
    STM32_RCC_REG_APB3 = 6,
};

#define STM32_RCC_CLK(reg, index)  (((reg) << 16) | (index))

#define STM32_RCC_CLK_REG(clk) ((clk) >> 16)
#define STM32_RCC_CLK_INDEX(clk) ((clk) & 0xffff)

typedef enum {
    // AHB2: the GPIO ports.
    STM32_RCC_CLK_GPIOA = STM32_RCC_CLK(STM32_RCC_REG_AHB2, RCC_AHB2ENR_GPIOAEN_Pos),
    STM32_RCC_CLK_GPIOB = STM32_RCC_CLK(STM32_RCC_REG_AHB2, RCC_AHB2ENR_GPIOBEN_Pos),
    STM32_RCC_CLK_GPIOC = STM32_RCC_CLK(STM32_RCC_REG_AHB2, RCC_AHB2ENR_GPIOCEN_Pos),
    STM32_RCC_CLK_GPIOD = STM32_RCC_CLK(STM32_RCC_REG_AHB2, RCC_AHB2ENR_GPIODEN_Pos),
    STM32_RCC_CLK_GPIOE = STM32_RCC_CLK(STM32_RCC_REG_AHB2, RCC_AHB2ENR_GPIOEEN_Pos),
    STM32_RCC_CLK_GPIOF = STM32_RCC_CLK(STM32_RCC_REG_AHB2, RCC_AHB2ENR_GPIOFEN_Pos),
    STM32_RCC_CLK_GPIOG = STM32_RCC_CLK(STM32_RCC_REG_AHB2, RCC_AHB2ENR_GPIOGEN_Pos),
    STM32_RCC_CLK_GPIOH = STM32_RCC_CLK(STM32_RCC_REG_AHB2, RCC_AHB2ENR_GPIOHEN_Pos),
    STM32_RCC_CLK_GPIOI = STM32_RCC_CLK(STM32_RCC_REG_AHB2, RCC_AHB2ENR_GPIOIEN_Pos),

    // The uarts.
    STM32_RCC_CLK_USART1 = STM32_RCC_CLK(STM32_RCC_REG_APB2, RCC_APB2ENR_USART1EN_Pos),
    STM32_RCC_CLK_USART2 = STM32_RCC_CLK(STM32_RCC_REG_APB1L, RCC_APB1LENR_USART2EN_Pos),
    STM32_RCC_CLK_USART3 = STM32_RCC_CLK(STM32_RCC_REG_APB1L, RCC_APB1LENR_USART3EN_Pos),
    STM32_RCC_CLK_LPUART1 = STM32_RCC_CLK(STM32_RCC_REG_APB3, RCC_APB3ENR_LPUART1EN_Pos),
} stm32_rcc_clk_t;

void stm32_rcc_set_enable(stm32_rcc_clk_t clock, bool enable);
