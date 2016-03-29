/*
 * @brief LPC15xx D/A conversion driver
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

#ifndef __DAC_15XX_H_
#define __DAC_15XX_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup DAC_15XX CHIP: LPC15xx D/A conversion driver
 * @ingroup CHIP_15XX_Drivers
 * @{
 */

/**
 * @brief DAC register block structure
 */
typedef struct {				/*!< DAC Structure */
	__IO uint32_t  VAL;		/*!< DAC register. Holds the conversion data */
	__IO uint32_t  CTRL;	/*!< DAC control register */
	__IO uint32_t  CNTVAL;	/*!< DAC counter value register */
} LPC_DAC_T;

/** After this field is written with a
   new VALUE, the voltage on the AOUT pin (with respect to VSSA)
   is VALUE*((VREFP_DAC - VREFN)/4095) + VREFN */
#define DAC_VALUE(n)        ((uint32_t) ((n & 0x0FFF) << 4))

/* Bit Definitions for DAC Control register */
#define DAC_INT_DMA_FLAG    (1 << 0)
#define DAC_TRIG_SRC_MASK   (0x7 << 1)
#define DAC_TRIG_SRC_BIT    (1 << 1)
#define DAC_POLARITY        (1 << 4)
#define DAC_SYNC_BYPASS     (1 << 5)
#define DAC_TIM_ENA_BIT     (1 << 6)
#define DAC_DBLBUF_ENA      (1 << 7)
#define DAC_SHUTOFF_ENA     (1 << 8)
#define DAC_SHUTOFF_FLAG    (1 << 9)
#define DAC_DACCTRL_MASK    ((uint32_t) 0xFF << 1)
#define DAC_CTRL_UNUSED     ((uint32_t) 0x7FFFF << 13)

/** Value to reload interrupt/DMA timer */
#define DAC_CNT_VALUE(n)  ((uint32_t) ((n) & 0xffff))

/**
 * @brief	Initial DAC configuration - Value to AOUT is 0
 * @param	pDAC	: pointer to LPC_DAC_T
 * @return	Nothing
 */
void Chip_DAC_Init(LPC_DAC_T *pDAC);

/**
 * @brief	Shutdown DAC
 * @param	pDAC	: pointer to LPC_DAC_T
 * @return	Nothing
 */
void Chip_DAC_DeInit(LPC_DAC_T *pDAC);

/**
 * @brief	Update value to DAC buffer
 * @param	pDAC		: pointer to LPC_DAC_T
 * @param	dac_value	: 12 bit input value for conversion
 * @return	Nothing
 */
STATIC INLINE void Chip_DAC_UpdateValue(LPC_DAC_T *pDAC, uint32_t dac_value)
{
	pDAC->VAL = DAC_VALUE(dac_value);
}

/**
 * @brief	Get status for interrupt/DMA time out
 * @param	pDAC	: pointer to LPC_DAC_T
 * @return	TRUE if interrupt/DMA flag is set else returns FALSE
 */
STATIC INLINE bool Chip_DAC_GetIntStatus(LPC_DAC_T *pDAC)
{
	return (pDAC->CTRL & DAC_INT_DMA_FLAG) != 0;
}

/**
 * @brief	Set Interrupt/DMA trigger source as Internal timer, enable timer before this call
 * @param	pDAC	: pointer to LPC_DAC_T
 * @return	Nothing
 */
STATIC INLINE void Chip_DAC_SetTrgSrcInternal(LPC_DAC_T *pDAC)
{
	pDAC->CTRL &= ~(DAC_CTRL_UNUSED | DAC_TRIG_SRC_BIT);
}

/**
 * @brief	Set Interrupt/DMA trigger source as External Pin
 * @param	pDAC	: pointer to LPC_DAC_T
 * @return	Nothing
 */
STATIC INLINE void Chip_DAC_SetTrgSrcExternal(LPC_DAC_T *pDAC)
{
	pDAC->CTRL = (pDAC->CTRL & ~DAC_CTRL_UNUSED) | DAC_TRIG_SRC_BIT;
}

/**
 * @brief	Set Polarity for external trigger pin
 * @param	pDAC		: pointer to LPC_DAC_T
 * @param falling_edge	: If TRUE indicates that the trigger polarity is Falling edge
 *											else it is a Rising edge
 * @return	Nothing
 */
STATIC INLINE void Chip_DAC_SetExtTriggerPolarity(LPC_DAC_T *pDAC, bool falling_edge)
{
	if (falling_edge) {
		pDAC->CTRL = (pDAC->CTRL & ~DAC_CTRL_UNUSED) | DAC_POLARITY;
	}
	else {
		pDAC->CTRL &= ~(DAC_CTRL_UNUSED | DAC_POLARITY );
	}
}

/**
 * @brief	Enable Sync Bypass, only if external trigger is in sync with SysClk
 * @param	pDAC	: pointer to LPC_DAC_T
 * @return	Nothing
 */
STATIC INLINE void Chip_DAC_EnableSyncBypass(LPC_DAC_T *pDAC)
{
	pDAC->CTRL = (pDAC->CTRL & ~DAC_CTRL_UNUSED) | DAC_SYNC_BYPASS;
}

/**
 * @brief	Disable Sync Bypass
 * @param	pDAC	: pointer to LPC_DAC_T
 * @return	Nothing
 */
STATIC INLINE void Chip_DAC_DisableSyncBypass(LPC_DAC_T *pDAC)
{
	pDAC->CTRL &= ~(DAC_CTRL_UNUSED | DAC_SYNC_BYPASS);
}

/**
 * @brief	Enable Internal Timer, CNTVAL should be loaded before enabling timer
 * @param	pDAC	: pointer to LPC_DAC_T
 * @return	Nothing
 */
STATIC INLINE void Chip_DAC_EnableIntTimer(LPC_DAC_T *pDAC)
{
	pDAC->CTRL = (pDAC->CTRL & ~DAC_CTRL_UNUSED) | DAC_TIM_ENA_BIT;
}

/**
 * @brief	Disable Internal Timer
 * @param	pDAC	: pointer to LPC_DAC_T
 * @return	Nothing
 */
STATIC INLINE void Chip_DAC_DisableIntTimer(LPC_DAC_T *pDAC)
{
	pDAC->CTRL &= ~(DAC_CTRL_UNUSED | DAC_TIM_ENA_BIT);
}

/**
 * @brief	Enable DAC Double Buffer
 * @param	pDAC	: pointer to LPC_DAC_T
 * @return	Nothing
 */
STATIC INLINE void Chip_DAC_EnableDoubleBuffer(LPC_DAC_T *pDAC)
{
	pDAC->CTRL = (pDAC->CTRL & ~DAC_CTRL_UNUSED) | DAC_DBLBUF_ENA;
}

/**
 * @brief	Disable DAC Double Buffer
 * @param	pDAC	: pointer to LPC_DAC_T
 * @return	Nothing
 */
STATIC INLINE void Chip_DAC_DisableDoubleBuffer(LPC_DAC_T *pDAC)
{
	pDAC->CTRL &= ~(DAC_CTRL_UNUSED | DAC_DBLBUF_ENA);
}

/**
 * @brief	Enable DAC Shut Off
 * @param	pDAC	: pointer to LPC_DAC_T
 * @return	Nothing
 */
STATIC INLINE void Chip_DAC_EnableShutOff(LPC_DAC_T *pDAC)
{
	pDAC->CTRL = (pDAC->CTRL & ~DAC_CTRL_UNUSED) | DAC_SHUTOFF_ENA;
}

/**
 * @brief	Disable DAC Shut Off
 * @param	pDAC	: pointer to LPC_DAC_T
 * @return	Nothing
 */
STATIC INLINE void Chip_DAC_DisableShutOff(LPC_DAC_T *pDAC)
{
	pDAC->CTRL &= ~(DAC_CTRL_UNUSED | DAC_SHUTOFF_ENA);
}

/**
 * @brief	Get status of DAC Shut Off
 * @param	pDAC	: pointer to LPC_DAC_T
 * @return	TRUE if DAC is shut off else returns FALSE
 */
STATIC INLINE bool Chip_DAC_GetShutOffStatus(LPC_DAC_T *pDAC)
{
	return (pDAC->CTRL & DAC_SHUTOFF_FLAG) != 0;
}

/**
 * @brief	Enables the DMA operation and controls DMA timer
 * @param	pDAC		: pointer to LPC_DAC_T
 * @param	dacFlags	: An Or'ed value of the following DAC values:
 *                  - DAC_TRIG_SRC_BIT  :set trigger source for Interrupt/DMA
 *																			 0 - Internal timer trigger, 1 - External trigger
 *                  - DAC_POLARITY          :polarity of the trigger if it is external
 *                  - DAC_SYNC_BYPASS   :Synchronize selection is trigger is external
 *                  - DAC_TIM_ENA_BIT   :enable/disable internal timer
 *                  - DAC_DBLBUF_ENA        :enable/disable DAC double buffering feature
 *                  - DAC_SHUTOFF_ENA   :enable/disable DAC Shutoff Pin
 * @return	Nothing
 * @note	Pass an Or'ed value of the DAC flags to enable those options.
 */
STATIC INLINE void Chip_DAC_ConfigDMAConverterControl(LPC_DAC_T *pDAC, uint32_t dacFlags)
{
	uint32_t temp;

	temp = pDAC->CTRL & ~(DAC_CTRL_UNUSED | DAC_DACCTRL_MASK);
	pDAC->CTRL = temp | dacFlags;
}

/**
 * @brief	Set reload value for interrupt/DMA timer
 * @param	pDAC		: pointer to LPC_DAC_T
 * @param	time_out	: time out to reload for interrupt/DMA timer.
 *									time out rate will be SysClk/(time_out + 1)
 * @return	Nothing
 */
STATIC INLINE void Chip_DAC_SetDMATimeOut(LPC_DAC_T *pDAC, uint32_t time_out)
{
	pDAC->CNTVAL = DAC_CNT_VALUE(time_out);
}

/**
 * @brief	Set reload value for interrupt/DMA timer to trigger periodic interrupts
 * @param	pDAC		: pointer to LPC_DAC_T
 * @param	periodHz	: Frequency of Timer interrupts in Hz
 * @return	Nothing
 */
STATIC INLINE void Chip_DAC_SetReqInterval(LPC_DAC_T *pDAC, uint32_t periodHz)
{
	uint32_t time_out = Chip_Clock_GetSystemClockRate() / periodHz - 1;
	pDAC->CNTVAL = DAC_CNT_VALUE(time_out);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __DAC_15XX_H_ */
