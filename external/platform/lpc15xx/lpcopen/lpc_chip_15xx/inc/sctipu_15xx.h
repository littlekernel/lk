/*
 * @brief LPC15xx SCTIPU driver
 *
 * @note
 * Copyright(C) NXP Semiconductors, 2013
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

#ifndef __SCTIPU_15XX_H_
#define __SCTIPU_15XX_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup SCTIPU_15XX CHIP: LPC15xx SCT Input Processing Unit (SCTIPU) driver
 * @ingroup CHIP_15XX_Drivers
 * @{
 */

/**
 * @brief LPC15XX SCTIPU abort enable/source register block structure
 */
typedef struct {			/*!< LPC15XX abort enable/source structure */
	__IO uint32_t  ABORT_ENABLE;	/*!< SCTIPU abort enable register */
	__IO uint32_t  ABORT_SOURCE;	/*!< SCTIPU abort source register */
	__I  uint32_t  RESERVED[6];
} LPC_SCTIPU_ABT_T;

/**
 * @brief LPC15XX SCTIPU register block structure
 */
typedef struct {			/*!< LPC15XX SCTIPU Structure */
	__IO uint32_t  SAMPLE_CTRL;	/*!< SCTIPU sample control register */
	__I  uint32_t  RESERVED[7];
	LPC_SCTIPU_ABT_T ABORT[4];	/*!< SCTIPU abort enable/source registers */
} LPC_SCTIPU_T;

/**
 * @brief	Initialize the SCTIPU
 * @return	Nothing
 * @note	Must be called prior to any other SCTIPU function. Sets up clocking and
 * initial SCTIPU states.
 */
STATIC INLINE void Chip_SCTIPU_Init(void)
{
	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_SCTIPU);
	Chip_SYSCTL_PeriphReset(RESET_SCTIPU);
}

/**
 * @brief	De-Initialize the SCTIPU
 * @return	Nothing
 */
STATIC INLINE void Chip_SCTIPU_DeInit(void)
{
	Chip_Clock_DisablePeriphClock(SYSCTL_CLOCK_SCTIPU);
}

/**
 * SCTIPU sample control register bit definitions
 */
#define SCTIPU_CTRL_INSEL(ch, src)  ((src) << (ch))	/*!< Select SCTIPU sample (src) source for output channel ch */
#define SCTIPU_CTRL_INSELMASK(ch)   (1 << (ch))		/*!< SCTIPU sample (src) source mask for output channel ch */
#define SCTIPU_CTRL_SAMPENA         (0)				/*!< Selects Sample_Enable_A as the latch/sample-enable control for the Sample_Output latch */
#define SCTIPU_CTRL_SAMPENB         (1)				/*!< Selects Sample_Enable_A as the latch/sample-enable control for the Sample_Output latch */
#define SCTIPU_CTRL_SAMPENC         (2)				/*!< Selects Sample_Enable_A as the latch/sample-enable control for the Sample_Output latch */
#define SCTIPU_CTRL_SAMPEND         (3)				/*!< Selects Sample_Enable_A as the latch/sample-enable control for the Sample_Output latch */
#define SCTIPU_CTRL_SAMPENDSEL(ch, src) ((src) << (2 + (ch * 2)))	/*!< Select SCTIPU sample (src) source for output channel ch */
#define SCTIPU_CTRL_SAMPENDMASK(ch) (0x3 << (2 + (ch * 2)))	/*!< SCTIPU sample (src) source mask for output channel ch */
#define SCTIPU_CTRL_LATCHENSEL(ch, ltc) ((ltc) << (12 + ch))	/*!< Select SCTIPU latched mode for output channel ch */
#define SCTIPU_CTRL_LATCHENMASK(ch) (1 << (12 + ch))	/*!< SCTIPU latched mode mask for output channel ch */
#define SCTIPU_RESERVED_BITS        0xFFFF0000

/**
 * @brief	Sets up an configuration and input source for a SCTIPU output channel
 * @param	ch			: SCTIPU channel, 0-3
 * @param	useb		: 0 to use SAMPLE_IN_A for the channel, or 1 for SAMPLE_IN_B
 * @param	sampIn		: Sample enable input, must be SCTIPU_CTRL_SAMPENA via SCTIPU_CTRL_SAMPEND
 * @param	useLatch	: 0 to transparent mode. for the channel, or 1 for latched mode
 * @return	Nothing
 * @note	Example: Chip_SCTIPU_ConfigSample(0, true, SCTIPU_CTRL_SAMPENC, true);
 */
void Chip_SCTIPU_ConfigSample(uint8_t ch, uint8_t useb, uint8_t sampIn, uint8_t useLatch);

/**
 * SCTIPU abort enable sources
 */
#define SCTIPU_ABTENA_SCT_ABORT0        (1 << 0)	/*!< Enable abort source SCT_ABORT0. Select pin from switch matrix */
#define SCTIPU_ABTENA_SCT_ABORT1        (1 << 1)	/*!< Enable abort source SCT_ABORT1. Select pin from switch matrix */
#define SCTIPU_ABTENA_SCT0_OUT9         (1 << 2)	/*!< Enable abort source SCT0_OUT9 */
#define SCTIPU_ABTENA_ADC0_THCMP_IRQ    (1 << 3)	/*!< Enable abort source ADC0_THCMP_IRQ */
#define SCTIPU_ABTENA_ADC1_THCMP_IRQ    (1 << 4)	/*!< Enable abort source ADC1_THCMP_IRQ */
#define SCTIPU_ABTENA_ACMP0_O           (1 << 5)		/*!< Enable abort source ACMP0_O */
#define SCTIPU_ABTENA_ACMP1_O           (1 << 6)		/*!< Enable abort source ACMP1_O */
#define SCTIPU_ABTENA_ACMP2_O           (1 << 7)		/*!< Enable abort source ACMP2_O */
#define SCTIPU_ABTENA_ACMP3_O           (1 << 8)		/*!< Enable abort source ACMP3_O */

/**
 * @brief	Selects multiple abort input enables that will be enabled to contribute to the ORed output
 * @param	ch			: SCTIPU channel, 0-3
 * @param	srcAbort	: Or'ed values of SCTIPU_ABTENA_* defintions used for OR'ed abort enables
 * @return	Nothing
 * @note	Example: Chip_SCTIPU_ConfigSample(0, SCTIPU_ABTENA_ACMP0_O | SCTIPU_ABTENA_ACMP1_O);<br>
 */
STATIC INLINE void Chip_SCTIPU_AbortInputEnable(uint8_t ch, uint32_t srcAbort)
{
	LPC_SCTIPU->ABORT[ch].ABORT_ENABLE = srcAbort;
}

/**
 * @brief	Gets the activated SCT abort sources
 * @param	ch	: SCTIPU channel, 0-3
 * @return	Nothing
 * @note	To determine if a source is active, mask the return value with a
 * SCTIPU_ABTENA_* definition.
 */
STATIC INLINE uint32_t Chip_SCTIPU_GetActiveAbortSrc(uint8_t ch)
{
	return LPC_SCTIPU->ABORT[ch].ABORT_SOURCE;
}

/**
 * @brief	Clears activated SCT abort sources
 * @param	ch			: SCTIPU channel, 0-3
 * @param	srcClear	: Or'ed values of SCTIPU_ABTENA_* defintions used for clearing activated states
 * @return	Nothing
 */
STATIC INLINE void Chip_SCTIPU_ClearActiveAbortSrc(uint8_t ch, uint32_t srcClear)
{
	LPC_SCTIPU->ABORT[ch].ABORT_SOURCE = srcClear;
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __SCTIPU_15XX_H_ */
