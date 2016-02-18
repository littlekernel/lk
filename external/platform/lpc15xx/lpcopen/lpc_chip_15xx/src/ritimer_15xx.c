/*
 * @brief LPC15xx RITimer driver
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

#include "chip.h"

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/* Initialize the RIT */
void Chip_RIT_Init(LPC_RITIMER_T *pRITimer)
{
	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_RIT);
	Chip_SYSCTL_PeriphReset(RESET_RIT);
	pRITimer->CTRL  = 0x04;
}

/* DeInitialize the RIT */
void Chip_RIT_DeInit(LPC_RITIMER_T *pRITimer)
{
	pRITimer->CTRL  = 0x00;
	Chip_Clock_DisablePeriphClock(SYSCTL_CLOCK_RIT);
}

/* Safely sets CTRL register bits */
void Chip_RIT_SetCTRL(LPC_RITIMER_T *pRITimer, uint32_t val)
{
	uint32_t reg;

	reg = pRITimer->CTRL & 0xF;
	pRITimer->CTRL = reg | val;
}

/* Safely clears CTRL register bits */
void Chip_RIT_ClearCTRL(LPC_RITIMER_T *pRITimer, uint32_t val)
{
	uint32_t reg;

	reg = pRITimer->CTRL & 0xF;
	pRITimer->CTRL = reg & ~val;
}

/* Set a tick value for the interrupt to time out */
void Chip_RIT_SetCompareValue(LPC_RITIMER_T *pRITimer, uint64_t val)
{
	pRITimer->COMPVAL = (uint32_t) val;
	pRITimer->COMPVAL_H = (uint32_t) (val >> 32);
}

/* Returns the current timer compare value */
uint64_t Chip_RIT_GetCompareValue(LPC_RITIMER_T *pRITimer)
{
	uint64_t val;

	val = (uint64_t) pRITimer->COMPVAL_H;
	val = val << 32;
	val |= (uint64_t) pRITimer->COMPVAL;

	return val;
}

/* Sets a mask value used for bit based compare */
void Chip_RIT_SetMaskValue(LPC_RITIMER_T *pRITimer, uint64_t mask)
{
	pRITimer->MASK = (uint32_t) mask;
	pRITimer->MASK_H = (uint32_t) (mask >> 32);
}

/* Returns the mask value used for bit based compare */
uint64_t Chip_RIT_GetMaskValue(LPC_RITIMER_T *pRITimer)
{
	uint64_t val;

	val = (uint64_t) pRITimer->MASK_H;
	val = val << 32;
	val |= (uint64_t) pRITimer->MASK;

	return val;
}

/* Sets the current timer Counter value */
void Chip_RIT_SetCounter(LPC_RITIMER_T *pRITimer, uint64_t count)
{
	pRITimer->COUNTER = (uint32_t) count;
	pRITimer->COUNTER_H = (uint32_t) (count >> 32);
}

/* Returns the current timer Counter value */
uint64_t Chip_RIT_GetCounter(LPC_RITIMER_T *pRITimer)
{
	uint64_t val;

	val = (uint64_t) pRITimer->COUNTER_H;
	val = val << 32;
	val |= (uint64_t) pRITimer->COUNTER;

	return val;
}

/* Set timer interval value in Hz (frequency) */
void Chip_RIT_SetTimerIntervalHz(LPC_RITIMER_T *pRITimer, uint32_t freq)
{
	uint64_t cmp_value;

	/* Determine approximate compare value based on clock rate and passed interval */
	cmp_value = (uint64_t) Chip_Clock_GetSystemClockRate();
	cmp_value = cmp_value / (uint64_t) freq;

	/* Set timer compare value and periodic mode */
	Chip_RIT_SetCompareValue(pRITimer, cmp_value);
	Chip_RIT_EnableCompClear(pRITimer);
}
