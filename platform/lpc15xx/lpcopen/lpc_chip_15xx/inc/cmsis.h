/*
 * @brief Basic CMSIS include file for LPC15xx
 *
 * @note
 * Copyright(C) NXP Semiconductors, 2014
 * All rights reserved.
 *
 * @par
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
 * @par
 * Permission to use, copy, modify, and distribute this software and its
 * documentation is hereby granted, under NXP Semiconductors' and its
 * licensor's relevant copyrights in the software, without fee, provided that it
 * is used in conjunction with NXP Semiconductors microcontrollers.  This
 * copyright, permission, and disclaimer notice must appear in all copies of
 * this code.
 */

#ifndef __CMSIS_15XX_H_
#define __CMSIS_15XX_H_

#include "lpc_types.h"
#include "sys_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup CMSIS_LPC15XX CHIP: LPC15xx CMSIS include file
 * @ingroup CHIP_15XX_CMSIS_Drivers
 * @{
 */

#if defined(__ARMCC_VERSION)
// Kill warning "#pragma push with no matching #pragma pop"
  #pragma diag_suppress 2525
  #pragma push
  #pragma anon_unions
#elif defined(__CWCC__)
  #pragma push
  #pragma cpp_extensions on
#elif defined(__GNUC__)
/* anonymous unions are enabled by default */
#elif defined(__IAR_SYSTEMS_ICC__)
//  #pragma push // FIXME not usable for IAR
  #pragma language=extended
#else
  #error Not supported compiler type
#endif

/*
 * ==========================================================================
 * ---------- Interrupt Number Definition -----------------------------------
 * ==========================================================================
 */

#if !defined(CHIP_LPC15XX)
#error Incorrect or missing device variant (CHIP_LPC15XX)
#endif

/** @defgroup CMSIS_15XX_IRQ CHIP: LPC15xx peripheral interrupt numbers
 * @{
 */

typedef enum IRQn {
	Reset_IRQn                    = -15,	/*!< Reset Vector, invoked on Power up and warm reset */
	NonMaskableInt_IRQn           = -14,	/*!< Non maskable Interrupt, cannot be stopped or preempted */
	HardFault_IRQn                = -13,	/*!< Hard Fault, all classes of Fault               */
	MemoryManagement_IRQn         = -12,	/*!< Memory Management, MPU mismatch, including Access Violation and No Match */
	BusFault_IRQn                 = -11,	/*!< Bus Fault, Pre-Fetch-, Memory Access Fault, other address/memory related Fault */
	UsageFault_IRQn               = -10,	/*!< Usage Fault, i.e. Undef Instruction, Illegal State Transition */
	SVCall_IRQn                   =  -5,	/*!< System Service Call via SVC instruction         */
	DebugMonitor_IRQn             =  -4,	/*!< Debug Monitor                                   */
	PendSV_IRQn                   =  -2,	/*!< Pendable request for system service             */
	SysTick_IRQn                  =  -1,	/*!< System Tick Timer                               */

	WDT_IRQn                      = 0,		/*!< Watchdog timer Interrupt                         */
	WWDT_IRQn                     = WDT_IRQn,	/*!< Watchdog timer Interrupt alias for WDT_IRQn    */
	BOD_IRQn                      = 1,		/*!< Brown Out Detect(BOD) Interrupt                  */
	FMC_IRQn                      = 2,		/*!< FLASH Interrupt                                  */
	FLASHEEPROM_IRQn              = 3,		/*!< EEPROM controller interrupt                      */
	DMA_IRQn                      = 4,		/*!< DMA Interrupt                                    */
	GINT0_IRQn                    = 5,		/*!< GPIO group 0 Interrupt                           */
	GINT1_IRQn                    = 6,		/*!< GPIO group 1 Interrupt                           */
	PIN_INT0_IRQn                 = 7,		/*!< Pin Interrupt 0                                  */
	PIN_INT1_IRQn                 = 8,		/*!< Pin Interrupt 1                                  */
	PIN_INT2_IRQn                 = 9,		/*!< Pin Interrupt 2                                  */
	PIN_INT3_IRQn                 = 10,		/*!< Pin Interrupt 3                                  */
	PIN_INT4_IRQn                 = 11,		/*!< Pin Interrupt 4                                  */
	PIN_INT5_IRQn                 = 12,		/*!< Pin Interrupt 5                                  */
	PIN_INT6_IRQn                 = 13,		/*!< Pin Interrupt 6                                  */
	PIN_INT7_IRQn                 = 14,		/*!< Pin Interrupt 7                                  */
	RITIMER_IRQn                  = 15,		/*!< RITIMER interrupt                                */
	SCT0_IRQn                     = 16,		/*!< SCT0 interrupt                                   */
	SCT_IRQn                      = SCT0_IRQn,	/*!< Optional alias for SCT0_IRQn                  */
	SCT1_IRQn                     = 17,		/*!< SCT1 interrupt                                   */
	SCT2_IRQn                     = 18,		/*!< SCT2 interrupt                                   */
	SCT3_IRQn                     = 19,		/*!< SCT3 interrupt                                   */
	MRT_IRQn                      = 20,		/*!< MRT interrupt                                    */
	UART0_IRQn                    = 21,		/*!< UART0 Interrupt                                  */
	UART1_IRQn                    = 22,		/*!< UART1 Interrupt                                  */
	UART2_IRQn                    = 23,		/*!< UART2 Interrupt                                  */
	I2C0_IRQn                     = 24,		/*!< I2C0 Interrupt                                   */
	I2C_IRQn                      = I2C0_IRQn,	/*!< Optional alias for I2C0_IRQn                  */
	SPI0_IRQn                     = 25,		/*!< SPI0 Interrupt                                   */
	SPI1_IRQn                     = 26,		/*!< SPI1 Interrupt                                   */
	CAN_IRQn                      = 27,		/*!< CAN Interrupt                                    */
	USB0_IRQn                     = 28,		/*!< USB IRQ interrupt                                */
	USB_IRQn                      = USB0_IRQn,	/*!< Optional alias for USB0_IRQn                  */
	USB0_FIQ_IRQn                 = 29,		/*!< USB FIQ interrupt                                */
	USB_FIQ_IRQn                  = USB0_FIQ_IRQn,	/*!< Optional alias for USB0_FIQ_IRQn         */
	USB_WAKEUP_IRQn               = 30,		/*!< USB wake-up interrupt Interrupt                  */
	ADC0_SEQA_IRQn                = 31,		/*!< ADC0_A sequencer Interrupt                       */
	ADC0_A_IRQn                   = ADC0_SEQA_IRQn,	/*!< Optional alias for ADC0_SEQA_IRQn        */
	ADC_A_IRQn                    = ADC0_SEQA_IRQn,	/*!< Optional alias for ADC0_SEQA_IRQn        */
	ADC0_SEQB_IRQn                = 32,		/*!< ADC0_B sequencer Interrupt                       */
	ADC0_B_IRQn                   = ADC0_SEQB_IRQn,	/*!< Optional alias for ADC0_SEQB_IRQn        */
	ADC_B_IRQn                    = ADC0_SEQB_IRQn,	/*!< Optional alias for ADC0_SEQB_IRQn        */
	ADC0_THCMP                    = 33,		/*!< ADC0 threshold compare interrupt                 */
	ADC0_OVR                      = 34,		/*!< ADC0 overrun interrupt                           */
	ADC1_SEQA_IRQn                = 35,		/*!< ADC1_A sequencer Interrupt                       */
	ADC1_A_IRQn                   = ADC1_SEQA_IRQn,	/*!< Optional alias for ADC1_SEQA_IRQn        */
	ADC1_SEQB_IRQn                = 36,		/*!< ADC1_B sequencer Interrupt                       */
	ADC1_B_IRQn                   = ADC1_SEQB_IRQn,	/*!< Optional alias for ADC1_SEQB_IRQn        */
	ADC1_THCMP                    = 37,		/*!< ADC1 threshold compare interrupt                 */
	ADC1_OVR                      = 38,		/*!< ADC1 overrun interrupt                           */
	DAC_IRQ                       = 39,		/*!< DAC interrupt                                    */
	CMP0_IRQ                      = 40,		/*!< Analog comparator 0 interrupt                    */
	CMP_IRQn                      = CMP0_IRQ,	/*!< Optional alias for CMP0_IRQ                    */
	CMP1_IRQ                      = 41,		/*!< Analog comparator 1 interrupt                    */
	CMP2_IRQ                      = 42,		/*!< Analog comparator 2 interrupt                    */
	CMP3_IRQ                      = 43,		/*!< Analog comparator 3 interrupt                    */
	QEI_IRQn                      = 44,		/*!< QEI interrupt                                    */
	RTC_ALARM_IRQn                = 45,		/*!< RTC alarm interrupt                              */
	RTC_WAKE_IRQn                 = 46,		/*!< RTC wake-up interrupt                            */
} IRQn_Type;

/**
 * @}
 */

/*
 * ==========================================================================
 * ----------- Processor and Core Peripheral Section ------------------------
 * ==========================================================================
 */

/** @defgroup CMSIS_15XX_COMMON CHIP: LPC15xx Cortex CMSIS definitions
 * @{
 */

#define __CM3_REV               0x0201		/*!< Cortex-M3 Core Revision                          */
#define __MPU_PRESENT             0			/*!< MPU present or not                    */
#define __NVIC_PRIO_BITS          3			/*!< Number of Bits used for Priority Levels */
#define __Vendor_SysTickConfig    0			/*!< Set to 1 if different SysTick Config is used */
#define __FPU_PRESENT             0			/*!< FPU present or not                    */

/**
 * @}
 */

#include "core_cm3.h"						/*!< Cortex-M3 processor and core peripherals */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __CMSIS_15XX_H_ */
