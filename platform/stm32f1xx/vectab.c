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
#include <compiler.h>
#include <stm32f10x.h>
#include <platform/stm32.h>
#include <target/debugconfig.h>

extern void stm32_tim2_irq(void);
extern void stm32_tim3_irq(void);
extern void stm32_tim4_irq(void);
extern void stm32_tim5_irq(void);
extern void stm32_tim6_irq(void);
extern void stm32_tim7_irq(void);

void stm32_USART1_IRQ(void)
{
	if (DEBUG_UART == USART1)
		stm32_debug_rx_irq();
	else
		PANIC_UNIMPLEMENTED;
}

void stm32_USART2_IRQ(void)
{
	if (DEBUG_UART == USART2)
		stm32_debug_rx_irq();
	else
		PANIC_UNIMPLEMENTED;
}

void stm32_USART3_IRQ(void)
{
	if (DEBUG_UART == USART3)
		stm32_debug_rx_irq();
	else
		PANIC_UNIMPLEMENTED;
}

/* appended to the end of the main vector table */
const void * const __SECTION(".text.boot.vectab2") vectab2[] =
{
    [TIM2_IRQn] = stm32_tim2_irq,
    [TIM3_IRQn] = stm32_tim3_irq,
    [TIM4_IRQn] = stm32_tim4_irq,

#if defined(STM32F10X_HD) || defined(STM32F10X_HD_VL) || defined(STM32F10X_XL) || defined(STM32F10X_CL)
    [TIM5_IRQn] = stm32_tim5_irq,
    [TIM6_IRQn] = stm32_tim6_irq,
    [TIM7_IRQn] = stm32_tim7_irq,
#endif

    [USART1_IRQn] = stm32_USART1_IRQ,
    [USART2_IRQn] = stm32_USART2_IRQ,
    [USART3_IRQn] = stm32_USART3_IRQ,
    [NUM_IRQn] = 0,
};

