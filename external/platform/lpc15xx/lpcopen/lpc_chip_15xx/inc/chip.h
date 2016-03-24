/*
 * @brief LPC15xx basic chip inclusion file
 *
 * Copyright(C) NXP Semiconductors, 2013
 * All rights reserved.
 *
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * LPC products.  This software is supplied "AS IS" without any warranties of
 * any kind, and NXP Semiconductors and its licensor disclaim any and
 * all warranties, express or implied, including all implied warranties of
 * merchantability, fitness for a particular purpose and non-infringement of
 * intellectual property rights.  NXP Semiconductors assumes no responsibility
 * or liability for the use of the software, conveys no license or rights under any
 * patent, copyright, mask work right, or any other intellectual property rights in
 * or to any products. NXP Semiconductors reserves the right to make changes
 * in the software without notification. NXP Semiconductors also makes no
 * representation or warranty that such application will be suitable for the
 * specified use without further testing or modification.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation is hereby granted, under NXP Semiconductors' and its
 * licensor's relevant copyrights in the software, without fee, provided that it
 * is used in conjunction with NXP Semiconductors microcontrollers.  This
 * copyright, permission, and disclaimer notice must appear in all copies of
 * this code.
 */

#ifndef __CHIP_H_
#define __CHIP_H_

#include "lpc_types.h"
#include "sys_config.h"
#include "cmsis.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CORE_M3
#error CORE_M3 is not defined for the LPC15xx architecture
#error CORE_M3 should be defined as part of your compiler define list
#endif

#if !defined(CHIP_LPC15XX)
#error CHIP_LPC15XX is not defined!
#endif

/** @defgroup PERIPH_15XX_BASE CHIP: LPC15xx Peripheral addresses and register set declarations
 * @ingroup CHIP_15XX_Drivers
 * @{
 */

#define LPC_ADC0_BASE             0x40000000
#define LPC_DAC_BASE              0x40004000
#define LPC_CMP_BASE              0x40008000
#define LPC_INMUX_BASE            0x40014000
#define LPC_RTC_BASE              0x40028000
#define LPC_WWDT_BASE             0x4002C000
#define LPC_SWM_BASE              0x40038000
#define LPC_PMU_BASE              0x4003C000
#define LPC_USART0_BASE           0x40040000
#define LPC_USART1_BASE           0x40044000
#define LPC_SPI0_BASE             0x40048000
#define LPC_SPI1_BASE             0x4004C000
#define LPC_I2C_BASE              0x40050000
#define LPC_QEI_BASE              0x40058000
#define LPC_SYSCTL_BASE           0x40074000
#define LPC_ADC1_BASE             0x40080000
#define LPC_MRT_BASE              0x400A0000
#define LPC_PIN_INT_BASE          0x400A4000
#define LPC_GPIO_GROUP_INT0_BASE  0x400A8000
#define LPC_GPIO_GROUP_INT1_BASE  0x400AC000
#define LPC_RITIMER_BASE          0x400B4000
#define LPC_SCTIPU_BASE           0x400B8000
#define LPC_FLASH_BASE            0x400BC000
#define LPC_USART2_BASE           0x400C0000
#define TBD_BASE                  0x400E8000
#define LPC_C_CAN0_BASE           0x400F0000
#define LPC_IOCON_BASE            0x400F8000
#define LPC_EEPROM_BASE           0x400FC000
#define LPC_GPIO_PIN_INT_BASE     0x1C000000
#define LPC_DMA_BASE              0x1C004000
#define LPC_USB0_BASE             0x1C00C000
#define LPC_CRC_BASE              0x1C010000
#define LPC_SCTLARGE_0_BASE       0x1C018000
#define LPC_SCTLARGE_1_BASE       0x1C01C000
#define LPC_SCTSMALL_0_BASE       0x1C020000
#define LPC_SCTSMALL_1_BASE       0x1C024000

#define LPC_PMU                   ((LPC_PMU_T              *) LPC_PMU_BASE)
#define LPC_IOCON                 ((LPC_IOCON_T            *) LPC_IOCON_BASE)
#define LPC_SYSCTL                ((LPC_SYSCTL_T           *) LPC_SYSCTL_BASE)
#define LPC_SYSCON                ((LPC_SYSCTL_T           *) LPC_SYSCTL_BASE)	/* Alias for LPC_SYSCTL */
#define LPC_GPIO                  ((LPC_GPIO_T             *) LPC_GPIO_PIN_INT_BASE)
#define LPC_GPIOGROUP             ((LPC_GPIOGROUPINT_T     *) LPC_GPIO_GROUP_INT0_BASE)
#define LPC_GPIO_PIN_INT          ((LPC_PIN_INT_T          *) LPC_PIN_INT_BASE)
#define LPC_USART0                ((LPC_USART_T            *) LPC_USART0_BASE)
#define LPC_USART1                ((LPC_USART_T            *) LPC_USART1_BASE)
#define LPC_USART2                ((LPC_USART_T            *) LPC_USART2_BASE)
#define LPC_I2C0                  ((LPC_I2C_T              *) LPC_I2C_BASE)
// #define LPC_I2C1                  ((LPC_I2C_T              *) LPC_I2C1_BASE)
// #define LPC_SSP0                  ((LPC_SSP_T              *) LPC_SSP0_BASE)
// #define LPC_SSP1                  ((LPC_SSP_T              *) LPC_SSP1_BASE)
#define LPC_USB                   ((LPC_USB_T              *) LPC_USB0_BASE)
#define LPC_ADC0                  ((LPC_ADC_T              *) LPC_ADC0_BASE)
#define LPC_ADC1                  ((LPC_ADC_T              *) LPC_ADC1_BASE)
// #define LPC_SCT0                  ((LPC_SCT_T              *) LPC_SCT0_BASE)
// #define LPC_SCT1                  ((LPC_SCT_T              *) LPC_SCT1_BASE)
// #define LPC_TIMER16_0             ((LPC_TIMER_T            *) LPC_TIMER16_0_BASE)
// #define LPC_TIMER16_1             ((LPC_TIMER_T            *) LPC_TIMER16_1_BASE)
// #define LPC_TIMER32_0             ((LPC_TIMER_T            *) LPC_TIMER32_0_BASE)
// #define LPC_TIMER32_1             ((LPC_TIMER_T            *) LPC_TIMER32_1_BASE)
#define LPC_RTC                   ((LPC_RTC_T              *) LPC_RTC_BASE)
#define LPC_WWDT                  ((LPC_WWDT_T             *) LPC_WWDT_BASE)
#define LPC_DMA                   ((LPC_DMA_T              *) LPC_DMA_BASE)
#define LPC_CRC                   ((LPC_CRC_T              *) LPC_CRC_BASE)
#define LPC_FMC                   ((LPC_FMC_T              *) LPC_FLASH_BASE)
#define LPC_MRT                   ((LPC_MRT_T              *) LPC_MRT_BASE)
#define LPC_SWM                   ((LPC_SWM_T              *) LPC_SWM_BASE)
#define LPC_RITIMER               ((LPC_RITIMER_T          *) LPC_RITIMER_BASE)
#define LPC_INMUX                 ((LPC_INMUX_T            *) LPC_INMUX_BASE)
#define LPC_SCTIPU                ((LPC_SCTIPU_T           *) LPC_SCTIPU_BASE)
#define LPC_CMP                   ((LPC_CMP_T              *) LPC_CMP_BASE)
#define LPC_DAC                   ((LPC_DAC_T              *) LPC_DAC_BASE)
#define LPC_SPI0                  ((LPC_SPI_T              *) LPC_SPI0_BASE)
#define LPC_SPI1                  ((LPC_SPI_T              *) LPC_SPI1_BASE)

/**
 * @}
 */

/** @ingroup CHIP_15XX_DRIVER_OPTIONS
 * @{
 */

/**
 * @brief	System oscillator rate
 * This value is defined externally to the chip layer and contains
 * the value in Hz for the external oscillator for the board. If using the
 * internal oscillator, this rate can be 0.
 */
extern const uint32_t OscRateIn;

/**
 * @brief	RTC oscillator rate
 * This value is defined externally to the chip layer and contains
 * the value in Hz for the RTC oscillator for the board. This is
 * usually 32KHz (32768). If not using the RTC, this rate can be 0.
 */
extern const uint32_t RTCOscRateIn;


/**
 * @}
 */

/* Include order is important! */
#include "romapi_15xx.h"
#include "sysctl_15xx.h"
#include "clock_15xx.h"
#include "iocon_15xx.h"
#include "swm_15xx.h"
#include "pmu_15xx.h"
#include "crc_15xx.h"
#include "gpio_15xx.h"
#include "pinint_15xx.h"
#include "gpiogroup_15xx.h"
// #include "timer_11u6x.h"
#include "uart_15xx.h"
// #include "ssp_11u6x.h"
#include "adc_15xx.h"
#include "mrt_15xx.h"
#include "ritimer_15xx.h"
#include "dma_15xx.h"
// #include "i2c_11u6x.h"
#include "usbd_15xx.h"
#include "sctipu_15xx.h"
// #include "sct_11u6x.h"
#include "rtc_15xx.h"
#include "wwdt_15xx.h"
#include "fmc_15xx.h"
#include "inmux_15xx.h"
#include "acmp_15xx.h"
#include "dac_15xx.h"
#include "spi_15xx.h"
#include "i2cm_15xx.h"
#include "i2cs_15xx.h"

/** @defgroup SUPPORT_15XX_FUNC CHIP: LPC15xx support functions
 * @ingroup CHIP_15XX_Drivers
 * @{
 */

/**
 * @brief	Current system clock rate, mainly used for sysTick
 */
extern uint32_t SystemCoreClock;

/**
 * @brief	Update system core clock rate, should be called if the
 *			system has a clock rate change
 * @return	None
 */
void SystemCoreClockUpdate(void);

/**
 * @brief	Set up and initialize hardware prior to call to main()
 * @return	None
 * @note	Chip_SystemInit() is called prior to the application and sets up
 * system clocking prior to the application starting.
 */
void Chip_SystemInit(void);

/**
 * @brief	USB clock initialization
 * Calling this function will initialize the USB PLL and clock divider
 * @return	None
 * @note	This function will assume that the chip is clocked by an
 * external crystal oscillator of frequency 12MHz and the Oscillator
 * is running.
 */
void Chip_USB_Init(void);

/**
 * @brief	Clock and PLL initialization based on the external oscillator
 * @return	None
 * @note	This function assumes an external crystal oscillator
 * frequency of 12MHz.
 */
void Chip_SetupXtalClocking(void);

/**
 * @brief	Clock and PLL initialization based on the internal oscillator
 * @return	None
 */
void Chip_SetupIrcClocking(void);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __CHIP_H_ */
