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
#include <stm32f0xx.h>
#include <arch/arm/cm.h>
#include <platform/stm32.h>
#include <target/debugconfig.h>
#include <lib/cbuf.h>

/* un-overridden irq handler */
void stm32_dummy_irq(void)
{
    arm_cm_irq_entry();

    panic("unhandled irq\n");
}

/* a list of default handlers that are simply aliases to the dummy handler */
#define DEFAULT_HANDLER(x) \
void stm32_##x(void) __WEAK_ALIAS("stm32_dummy_irq");

DEFAULT_HANDLER(WWDG_IRQ)
DEFAULT_HANDLER(PVD_IRQ)
DEFAULT_HANDLER(RTC_IRQ)
DEFAULT_HANDLER(FLASH_IRQ)
DEFAULT_HANDLER(RCC_IRQ)
DEFAULT_HANDLER(EXTI0_1_IRQ)
DEFAULT_HANDLER(EXTI2_3_IRQ)
DEFAULT_HANDLER(EXTI4_15_IRQ)
DEFAULT_HANDLER(TSC_IRQ)
DEFAULT_HANDLER(DMA1_Channel1_IRQ)
DEFAULT_HANDLER(DMA1_Channel2_3_IRQ)
DEFAULT_HANDLER(DMA1_Channel4_5_6_7_IRQ)
DEFAULT_HANDLER(ADC1_COMP_IRQ)
DEFAULT_HANDLER(TIM1_BRK_UP_TRG_COM_IRQ)
DEFAULT_HANDLER(TIM1_CC_IRQ)
DEFAULT_HANDLER(TIM2_IRQ)
DEFAULT_HANDLER(TIM3_IRQ)
DEFAULT_HANDLER(TIM6_DAC_IRQ)
DEFAULT_HANDLER(TIM7_IRQ)
DEFAULT_HANDLER(TIM14_IRQ)
DEFAULT_HANDLER(TIM15_IRQ)
DEFAULT_HANDLER(TIM16_IRQ)
DEFAULT_HANDLER(TIM17_IRQ)
DEFAULT_HANDLER(I2C1_IRQ)
DEFAULT_HANDLER(I2C2_IRQ)
DEFAULT_HANDLER(SPI1_IRQ)
DEFAULT_HANDLER(SPI2_IRQ)
DEFAULT_HANDLER(USART1_IRQ)
DEFAULT_HANDLER(USART2_IRQ)
DEFAULT_HANDLER(USART3_4_IRQ)
DEFAULT_HANDLER(CEC_CAN_IRQ)
DEFAULT_HANDLER(USB_IRQ)

#define VECTAB_ENTRY(x) [x##n] = stm32_##x

/* appended to the end of the main vector table */
const void *const __SECTION(".text.boot.vectab2") vectab2[] = {
    VECTAB_ENTRY(WWDG_IRQ),                 // Window WatchDog Interrupt
    VECTAB_ENTRY(PVD_IRQ),                  // PVD through EXTI Line detect Interrupt
    VECTAB_ENTRY(RTC_IRQ),              // RTC through EXTI Line Interrupt
    VECTAB_ENTRY(FLASH_IRQ),            // FLASH Interrupt
    VECTAB_ENTRY(RCC_IRQ),                  // RCC Interrupt
    VECTAB_ENTRY(EXTI0_1_IRQ),          // EXTI Line 0 and 1 Interrupts
    VECTAB_ENTRY(EXTI2_3_IRQ),          // EXTI Line 2 and 3 Interrupts
    VECTAB_ENTRY(EXTI4_15_IRQ),         // EXTI Line 4 to 15 Interrupts
    VECTAB_ENTRY(TSC_IRQ),                   // Touch sense controller Interrupt
    VECTAB_ENTRY(DMA1_Channel1_IRQ),    // DMA1 Channel 1 Interrupt
    VECTAB_ENTRY(DMA1_Channel2_3_IRQ),  // DMA1 Channel 2 and Channel 3 Interrupts
    VECTAB_ENTRY(DMA1_Channel4_5_6_7_IRQ),  // DMA1 Channels 4-7 Interrupts
    VECTAB_ENTRY(ADC1_COMP_IRQ),            // ADC1, COMP1 and COMP2 Interrupts
    VECTAB_ENTRY(TIM1_BRK_UP_TRG_COM_IRQ),  // TIM1 Break, Update, Trigger and Commutation Interrupts
    VECTAB_ENTRY(TIM1_CC_IRQ),          // TIM1 Capture Compare Interrupt
    VECTAB_ENTRY(TIM2_IRQ),                 // TIM2 Interrupt
    VECTAB_ENTRY(TIM3_IRQ),                 // TIM3 Interrupt
    VECTAB_ENTRY(TIM6_DAC_IRQ),         // TIM6 and DAC Interrupts
    VECTAB_ENTRY(TIM7_IRQ),                 // TIM7 Interrupts
    VECTAB_ENTRY(TIM14_IRQ),            // TIM14 Interrupt
    VECTAB_ENTRY(TIM15_IRQ),            // TIM15 Interrupt
    VECTAB_ENTRY(TIM16_IRQ),            // TIM16 Interrupt
    VECTAB_ENTRY(TIM17_IRQ),            // TIM17 Interrupt
    VECTAB_ENTRY(I2C1_IRQ),                 // I2C1 Interrupt
    VECTAB_ENTRY(I2C2_IRQ),                 // I2C2 Interrupt
    VECTAB_ENTRY(SPI1_IRQ),                 // SPI1 Interrupt
    VECTAB_ENTRY(SPI2_IRQ),                 // SPI2 Interrupt
    VECTAB_ENTRY(USART1_IRQ),               // USART1 Interrupt
    VECTAB_ENTRY(USART2_IRQ),           // USART2 Interrupt
    VECTAB_ENTRY(USART3_4_IRQ),         // USART3 and USART4 Interrupts
    VECTAB_ENTRY(CEC_CAN_IRQ),              // CEC Interrupt
    VECTAB_ENTRY(USB_IRQ),                  // USB Low Priority global Interrupt
};
