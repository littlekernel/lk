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
#include <stm32f2xx.h>
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

DEFAULT_HANDLER(WWDG_IRQ);
DEFAULT_HANDLER(PVD_IRQ);
DEFAULT_HANDLER(TAMP_STAMP_IRQ);
DEFAULT_HANDLER(RTC_WKUP_IRQ);
DEFAULT_HANDLER(FLASH_IRQ);
DEFAULT_HANDLER(RCC_IRQ);
DEFAULT_HANDLER(EXTI0_IRQ);
DEFAULT_HANDLER(EXTI1_IRQ);
DEFAULT_HANDLER(EXTI2_IRQ);
DEFAULT_HANDLER(EXTI3_IRQ);
DEFAULT_HANDLER(EXTI4_IRQ);

DEFAULT_HANDLER(DMA1_Stream0_IRQ);
DEFAULT_HANDLER(DMA1_Stream1_IRQ);
DEFAULT_HANDLER(DMA1_Stream2_IRQ);
DEFAULT_HANDLER(DMA1_Stream3_IRQ);
DEFAULT_HANDLER(DMA1_Stream4_IRQ);
DEFAULT_HANDLER(DMA1_Stream5_IRQ);
DEFAULT_HANDLER(DMA1_Stream6_IRQ);

DEFAULT_HANDLER(ADC_IRQ);
DEFAULT_HANDLER(CAN1_TX_IRQ);
DEFAULT_HANDLER(CAN1_RX0_IRQ);
DEFAULT_HANDLER(CAN1_RX1_IRQ);
DEFAULT_HANDLER(CAN1_SCE_IRQ);
DEFAULT_HANDLER(EXTI9_5_IRQ);

DEFAULT_HANDLER(TIM1_BRK_TIM9_IRQ);
DEFAULT_HANDLER(TIM1_UP_TIM10_IRQ);
DEFAULT_HANDLER(TIM1_TRG_COM_TIM11_IRQ);
DEFAULT_HANDLER(TIM1_CC_IRQ);
DEFAULT_HANDLER(TIM2_IRQ);
DEFAULT_HANDLER(TIM3_IRQ);
DEFAULT_HANDLER(TIM4_IRQ);

DEFAULT_HANDLER(I2C1_EV_IRQ);
DEFAULT_HANDLER(I2C1_ER_IRQ);
DEFAULT_HANDLER(I2C2_EV_IRQ);
DEFAULT_HANDLER(I2C2_ER_IRQ);

DEFAULT_HANDLER(SPI1_IRQ);
DEFAULT_HANDLER(SPI2_IRQ);

DEFAULT_HANDLER(USART1_IRQ);
DEFAULT_HANDLER(USART2_IRQ);
DEFAULT_HANDLER(USART3_IRQ);

DEFAULT_HANDLER(EXTI15_10_IRQ);
DEFAULT_HANDLER(RTC_Alarm_IRQ);
DEFAULT_HANDLER(OTG_FS_WKUP_IRQ);
DEFAULT_HANDLER(TIM8_BRK_TIM12_IRQ);
DEFAULT_HANDLER(TIM8_UP_TIM13_IRQ);
DEFAULT_HANDLER(TIM8_TRG_COM_TIM14_IRQ);
DEFAULT_HANDLER(TIM8_CC_IRQ);
DEFAULT_HANDLER(DMA1_Stream7_IRQ);
DEFAULT_HANDLER(FSMC_IRQ);
DEFAULT_HANDLER(SDIO_IRQ);
DEFAULT_HANDLER(TIM5_IRQ);
DEFAULT_HANDLER(SPI3_IRQ);
DEFAULT_HANDLER(UART4_IRQ);
DEFAULT_HANDLER(UART5_IRQ);
DEFAULT_HANDLER(TIM6_DAC_IRQ);
DEFAULT_HANDLER(TIM7_IRQ);

DEFAULT_HANDLER(DMA2_Stream0_IRQ);
DEFAULT_HANDLER(DMA2_Stream1_IRQ);
DEFAULT_HANDLER(DMA2_Stream2_IRQ);
DEFAULT_HANDLER(DMA2_Stream3_IRQ);
DEFAULT_HANDLER(DMA2_Stream4_IRQ);

DEFAULT_HANDLER(ETH_IRQ);
DEFAULT_HANDLER(ETH_WKUP_IRQ);
DEFAULT_HANDLER(CAN2_TX_IRQ);
DEFAULT_HANDLER(CAN2_RX0_IRQ);
DEFAULT_HANDLER(CAN2_RX1_IRQ);
DEFAULT_HANDLER(CAN2_SCE_IRQ);
DEFAULT_HANDLER(OTG_FS_IRQ);
DEFAULT_HANDLER(DMA2_Stream5_IRQ);
DEFAULT_HANDLER(DMA2_Stream6_IRQ);
DEFAULT_HANDLER(DMA2_Stream7_IRQ);
DEFAULT_HANDLER(USART6_IRQ);
DEFAULT_HANDLER(I2C3_EV_IRQ);
DEFAULT_HANDLER(I2C3_ER_IRQ);
DEFAULT_HANDLER(OTG_HS_EP1_OUT_IRQ);
DEFAULT_HANDLER(OTG_HS_EP1_IN_IRQ);
DEFAULT_HANDLER(OTG_HS_WKUP_IRQ);
DEFAULT_HANDLER(OTG_HS_IRQ);
DEFAULT_HANDLER(DCMI_IRQ);
DEFAULT_HANDLER(CRYP_IRQ);
DEFAULT_HANDLER(HASH_RNG_IRQ);

#define VECTAB_ENTRY(x) [x##n] = stm32_##x

/* appended to the end of the main vector table */
const void * const __SECTION(".text.boot.vectab2") vectab2[] = {
	VECTAB_ENTRY(WWDG_IRQ),                 /* Window WatchDog Interrupt                                         */
	VECTAB_ENTRY(PVD_IRQ),                  /* PVD through EXTI Line detection Interrupt                         */
	VECTAB_ENTRY(TAMP_STAMP_IRQ),           /* Tamper and TimeStamp interrupts through the EXTI line             */
	VECTAB_ENTRY(RTC_WKUP_IRQ),             /* RTC Wakeup interrupt through the EXTI line                        */
	VECTAB_ENTRY(FLASH_IRQ),                /* FLASH global Interrupt                                            */
	VECTAB_ENTRY(RCC_IRQ),                  /* RCC global Interrupt                                              */
	VECTAB_ENTRY(EXTI0_IRQ),                /* EXTI Line0 Interrupt                                              */
	VECTAB_ENTRY(EXTI1_IRQ),                /* EXTI Line1 Interrupt                                              */
	VECTAB_ENTRY(EXTI2_IRQ),                /* EXTI Line2 Interrupt                                              */
	VECTAB_ENTRY(EXTI3_IRQ),                /* EXTI Line3 Interrupt                                              */
	VECTAB_ENTRY(EXTI4_IRQ),                /* EXTI Line4 Interrupt                                              */
	VECTAB_ENTRY(DMA1_Stream0_IRQ),         /* DMA1 Stream 0 global Interrupt                                    */
	VECTAB_ENTRY(DMA1_Stream1_IRQ),         /* DMA1 Stream 1 global Interrupt                                    */
	VECTAB_ENTRY(DMA1_Stream2_IRQ),         /* DMA1 Stream 2 global Interrupt                                    */
	VECTAB_ENTRY(DMA1_Stream3_IRQ),         /* DMA1 Stream 3 global Interrupt                                    */
	VECTAB_ENTRY(DMA1_Stream4_IRQ),         /* DMA1 Stream 4 global Interrupt                                    */
	VECTAB_ENTRY(DMA1_Stream5_IRQ),         /* DMA1 Stream 5 global Interrupt                                    */
	VECTAB_ENTRY(DMA1_Stream6_IRQ),         /* DMA1 Stream 6 global Interrupt                                    */
	VECTAB_ENTRY(ADC_IRQ),                  /* ADC1, ADC2 and ADC3 global Interrupts                             */
	VECTAB_ENTRY(CAN1_TX_IRQ),              /* CAN1 TX Interrupt                                                 */
	VECTAB_ENTRY(CAN1_RX0_IRQ),             /* CAN1 RX0 Interrupt                                                */
	VECTAB_ENTRY(CAN1_RX1_IRQ),             /* CAN1 RX1 Interrupt                                                */
	VECTAB_ENTRY(CAN1_SCE_IRQ),             /* CAN1 SCE Interrupt                                                */
	VECTAB_ENTRY(EXTI9_5_IRQ),              /* External Line[9:5] Interrupts                                     */
	VECTAB_ENTRY(TIM1_BRK_TIM9_IRQ),        /* TIM1 Break interrupt and TIM9 global interrupt                    */
	VECTAB_ENTRY(TIM1_UP_TIM10_IRQ),        /* TIM1 Update Interrupt and TIM10 global interrupt                  */
	VECTAB_ENTRY(TIM1_TRG_COM_TIM11_IRQ),   /* TIM1 Trigger and Commutation Interrupt and TIM11 global interrupt */
	VECTAB_ENTRY(TIM1_CC_IRQ),              /* TIM1 Capture Compare Interrupt                                    */
	VECTAB_ENTRY(TIM2_IRQ),                 /* TIM2 global Interrupt                                             */
	VECTAB_ENTRY(TIM3_IRQ),                 /* TIM3 global Interrupt                                             */
	VECTAB_ENTRY(TIM4_IRQ),                 /* TIM4 global Interrupt                                             */
	VECTAB_ENTRY(I2C1_EV_IRQ),              /* I2C1 Event Interrupt                                              */
	VECTAB_ENTRY(I2C1_ER_IRQ),              /* I2C1 Error Interrupt                                              */
	VECTAB_ENTRY(I2C2_EV_IRQ),              /* I2C2 Event Interrupt                                              */
	VECTAB_ENTRY(I2C2_ER_IRQ),              /* I2C2 Error Interrupt                                              */
	VECTAB_ENTRY(SPI1_IRQ),                 /* SPI1 global Interrupt                                             */
	VECTAB_ENTRY(SPI2_IRQ),                 /* SPI2 global Interrupt                                             */
	VECTAB_ENTRY(USART1_IRQ),               /* USART1 global Interrupt                                           */
	VECTAB_ENTRY(USART2_IRQ),               /* USART2 global Interrupt                                           */
	VECTAB_ENTRY(USART3_IRQ),               /* USART3 global Interrupt                                           */
	VECTAB_ENTRY(EXTI15_10_IRQ),            /* External Line[15:10] Interrupts                                   */
	VECTAB_ENTRY(RTC_Alarm_IRQ),            /* RTC Alarm (A and B) through EXTI Line Interrupt                   */
	VECTAB_ENTRY(OTG_FS_WKUP_IRQ),          /* USB OTG FS Wakeup through EXTI line interrupt                     */
	VECTAB_ENTRY(TIM8_BRK_TIM12_IRQ),       /* TIM8 Break Interrupt and TIM12 global interrupt                   */
	VECTAB_ENTRY(TIM8_UP_TIM13_IRQ),        /* TIM8 Update Interrupt and TIM13 global interrupt                  */
	VECTAB_ENTRY(TIM8_TRG_COM_TIM14_IRQ),   /* TIM8 Trigger and Commutation Interrupt and TIM14 global interrupt */
	VECTAB_ENTRY(TIM8_CC_IRQ),              /* TIM8 Capture Compare Interrupt                                    */
	VECTAB_ENTRY(DMA1_Stream7_IRQ),         /* DMA1 Stream7 Interrupt                                            */
	VECTAB_ENTRY(FSMC_IRQ),                 /* FSMC global Interrupt                                             */
	VECTAB_ENTRY(SDIO_IRQ),                 /* SDIO global Interrupt                                             */
	VECTAB_ENTRY(TIM5_IRQ),                 /* TIM5 global Interrupt                                             */
	VECTAB_ENTRY(SPI3_IRQ),                 /* SPI3 global Interrupt                                             */
	VECTAB_ENTRY(UART4_IRQ),                /* UART4 global Interrupt                                            */
	VECTAB_ENTRY(UART5_IRQ),                /* UART5 global Interrupt                                            */
	VECTAB_ENTRY(TIM6_DAC_IRQ),             /* TIM6 global and DAC1&2 underrun error  interrupts                 */
	VECTAB_ENTRY(TIM7_IRQ),                 /* TIM7 global interrupt                                             */
	VECTAB_ENTRY(DMA2_Stream0_IRQ),         /* DMA2 Stream 0 global Interrupt                                    */
	VECTAB_ENTRY(DMA2_Stream1_IRQ),         /* DMA2 Stream 1 global Interrupt                                    */
	VECTAB_ENTRY(DMA2_Stream2_IRQ),         /* DMA2 Stream 2 global Interrupt                                    */
	VECTAB_ENTRY(DMA2_Stream3_IRQ),         /* DMA2 Stream 3 global Interrupt                                    */
	VECTAB_ENTRY(DMA2_Stream4_IRQ),         /* DMA2 Stream 4 global Interrupt                                    */
	VECTAB_ENTRY(ETH_IRQ),                  /* Ethernet global Interrupt                                         */
	VECTAB_ENTRY(ETH_WKUP_IRQ),             /* Ethernet Wakeup through EXTI line Interrupt                       */
	VECTAB_ENTRY(CAN2_TX_IRQ),              /* CAN2 TX Interrupt                                                 */
	VECTAB_ENTRY(CAN2_RX0_IRQ),             /* CAN2 RX0 Interrupt                                                */
	VECTAB_ENTRY(CAN2_RX1_IRQ),             /* CAN2 RX1 Interrupt                                                */
	VECTAB_ENTRY(CAN2_SCE_IRQ),             /* CAN2 SCE Interrupt                                                */
	VECTAB_ENTRY(OTG_FS_IRQ),               /* USB OTG FS global Interrupt                                       */
	VECTAB_ENTRY(DMA2_Stream5_IRQ),         /* DMA2 Stream 5 global interrupt                                    */
	VECTAB_ENTRY(DMA2_Stream6_IRQ),         /* DMA2 Stream 6 global interrupt                                    */
	VECTAB_ENTRY(DMA2_Stream7_IRQ),         /* DMA2 Stream 7 global interrupt                                    */
	VECTAB_ENTRY(USART6_IRQ),               /* USART6 global interrupt                                           */
	VECTAB_ENTRY(I2C3_EV_IRQ),              /* I2C3 event interrupt                                              */
	VECTAB_ENTRY(I2C3_ER_IRQ),              /* I2C3 error interrupt                                              */
	VECTAB_ENTRY(OTG_HS_EP1_OUT_IRQ),       /* USB OTG HS End Point 1 Out global interrupt                       */
	VECTAB_ENTRY(OTG_HS_EP1_IN_IRQ),        /* USB OTG HS End Point 1 In global interrupt                        */
	VECTAB_ENTRY(OTG_HS_WKUP_IRQ),          /* USB OTG HS Wakeup through EXTI interrupt                          */
	VECTAB_ENTRY(OTG_HS_IRQ),               /* USB OTG HS global interrupt                                       */
	VECTAB_ENTRY(DCMI_IRQ),                 /* DCMI global interrupt                                             */
	VECTAB_ENTRY(CRYP_IRQ),                 /* CRYP crypto global interrupt                                      */
	VECTAB_ENTRY(HASH_RNG_IRQ),             /* Hash and Rng global interrupt                                     */
};

