/*
 * Copyright (c) 2016 Erik Gilling
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
#ifndef __PLATFORM_STM32_RCC_H
#define __PLATFORM_STM32_RCC_H

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

#include <stm32f0xx.h>

enum {
    STM32_RCC_REG_AHB = 0,
    STM32_RCC_REG_APB1 = 1,
    STM32_RCC_REG_APB2 = 2,
};

#define STM32_RCC_CLK(reg, index)  (((reg) << 16) | (index))
#define STM32_RCC_CLK_AHB(index)   STM32_RCC_CLK(STM32_RCC_REG_AHB, index)
#define STM32_RCC_CLK_APB1(index)  STM32_RCC_CLK(STM32_RCC_REG_APB1, index)
#define STM32_RCC_CLK_APB2(index)  STM32_RCC_CLK(STM32_RCC_REG_APB2, index)

#define STM32_RCC_CLK_REG(clk) ((clk) >> 16)
#define STM32_RCC_CLK_INDEX(clk) ((clk) & 0xffff)

typedef enum {
    // AHB clocks.
    STM32_RCC_CLK_DMA =   STM32_RCC_CLK_AHB(0),
    STM32_RCC_CLK_DMA2 =  STM32_RCC_CLK_AHB(1),
    STM32_RCC_CLK_SRAM =  STM32_RCC_CLK_AHB(2),
    STM32_RCC_CLK_FLITF = STM32_RCC_CLK_AHB(4),
    STM32_RCC_CLK_CRC =   STM32_RCC_CLK_AHB(6),
    STM32_RCC_CLK_IOPA =  STM32_RCC_CLK_AHB(17),
    STM32_RCC_CLK_IOPB =  STM32_RCC_CLK_AHB(18),
    STM32_RCC_CLK_IOPC =  STM32_RCC_CLK_AHB(19),
    STM32_RCC_CLK_IOPD =  STM32_RCC_CLK_AHB(20),
    STM32_RCC_CLK_IOPE =  STM32_RCC_CLK_AHB(21),
    STM32_RCC_CLK_IOPF =  STM32_RCC_CLK_AHB(22),
    STM32_RCC_CLK_TSC =   STM32_RCC_CLK_AHB(24),

    // APB1 clocks.
    STM32_RCC_CLK_TIM2 =   STM32_RCC_CLK_APB1(0),
    STM32_RCC_CLK_TIM3 =   STM32_RCC_CLK_APB1(1),
    STM32_RCC_CLK_TIM6 =   STM32_RCC_CLK_APB1(4),
    STM32_RCC_CLK_TIM7 =   STM32_RCC_CLK_APB1(5),
    STM32_RCC_CLK_TIM14 =  STM32_RCC_CLK_APB1(8),
    STM32_RCC_CLK_WWDG =   STM32_RCC_CLK_APB1(11),
    STM32_RCC_CLK_SPI2 =   STM32_RCC_CLK_APB1(14),
    STM32_RCC_CLK_USART2 = STM32_RCC_CLK_APB1(17),
    STM32_RCC_CLK_USART3 = STM32_RCC_CLK_APB1(18),
    STM32_RCC_CLK_USART4 = STM32_RCC_CLK_APB1(19),
    STM32_RCC_CLK_USART5 = STM32_RCC_CLK_APB1(20),
    STM32_RCC_CLK_I2C1 =   STM32_RCC_CLK_APB1(21),
    STM32_RCC_CLK_I2C2 =   STM32_RCC_CLK_APB1(22),
    STM32_RCC_CLK_USB =    STM32_RCC_CLK_APB1(23),
    STM32_RCC_CLK_CAN =    STM32_RCC_CLK_APB1(25),
    STM32_RCC_CLK_CRS =    STM32_RCC_CLK_APB1(27),
    STM32_RCC_CLK_PWR =    STM32_RCC_CLK_APB1(28),
    STM32_RCC_CLK_DAC =    STM32_RCC_CLK_APB1(29),
    STM32_RCC_CLK_CEC =    STM32_RCC_CLK_APB1(30),

    // APB2 clocks.
    STM32_RCC_CLK_SYSCFGCOMP = STM32_RCC_CLK_APB2(0),
    STM32_RCC_CLK_USART6 =     STM32_RCC_CLK_APB2(5),
    STM32_RCC_CLK_USART7 =     STM32_RCC_CLK_APB2(6),
    STM32_RCC_CLK_USART8 =     STM32_RCC_CLK_APB2(7),
    STM32_RCC_CLK_ADC =        STM32_RCC_CLK_APB2(9),
    STM32_RCC_CLK_TIM1 =       STM32_RCC_CLK_APB2(11),
    STM32_RCC_CLK_SPI1 =       STM32_RCC_CLK_APB2(12),
    STM32_RCC_CLK_USART1 =     STM32_RCC_CLK_APB2(14),
    STM32_RCC_CLK_TIM15 =      STM32_RCC_CLK_APB2(16),
    STM32_RCC_CLK_TIM16 =      STM32_RCC_CLK_APB2(17),
    STM32_RCC_CLK_TIM17 =      STM32_RCC_CLK_APB2(18),
    STM32_RCC_CLK_DBG_MUC =    STM32_RCC_CLK_APB2(22),
} stm32_rcc_clk_t;

void stm32_rcc_set_enable(stm32_rcc_clk_t clock, bool enable);
void stm32_rcc_set_reset(stm32_rcc_clk_t clock, bool reset);
#endif  // __PLATFORM_STM32_RCC_H
