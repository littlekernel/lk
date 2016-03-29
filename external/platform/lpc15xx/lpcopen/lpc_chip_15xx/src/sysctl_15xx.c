/*
 * @brief LPC15XX System Control functions
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

#include "chip.h"

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

/* PDWAKECFG register mask */
#define PDWAKEUPUSEMASK 0x00000000
#define PDWAKEUPMASKTMP 0x01FFFF78

/* PDRUNCFG register mask */
#define PDRUNCFGUSEMASK 0x00000000
#define PDRUNCFGMASKTMP 0x01FFFF78

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/* Returns the computed value for a frequency measurement cycle */
uint32_t Chip_SYSCTL_GetCompFreqMeas(uint32_t refClockRate)
{
	uint32_t capval, clkrate = 0;

	/* Get raw capture value */
	capval = Chip_SYSCTL_GetRawFreqMeasCapval();

	/* Limit CAPVAL check */
	if (capval > 2) {
		clkrate = ((capval - 2) * refClockRate) / 0x4000;
	}

	return clkrate;
}

/* De-assert reset for a peripheral */
void Chip_SYSCTL_AssertPeriphReset(CHIP_SYSCTL_PERIPH_RESET_T periph)
{
	if (periph >= 32) {
		LPC_SYSCTL->PRESETCTRL[1] |= (1 << ((uint32_t) periph - 32));
	}
	else {
		LPC_SYSCTL->PRESETCTRL[0] |= (1 << (uint32_t) periph);
	}
}

/* Assert reset for a peripheral */
void Chip_SYSCTL_DeassertPeriphReset(CHIP_SYSCTL_PERIPH_RESET_T periph)
{
	if (periph >= 32) {
		LPC_SYSCTL->PRESETCTRL[1] &= ~(1 << ((uint32_t) periph - 32));
	}
	else {
		LPC_SYSCTL->PRESETCTRL[0] &= ~(1 << (uint32_t) periph);
	}
}

/* Setup wakeup behaviour from deep sleep */
void Chip_SYSCTL_SetWakeup(uint32_t wakeupmask)
{
	/* Update new value */
	LPC_SYSCTL->PDWAKECFG = PDWAKEUPUSEMASK | (wakeupmask & PDWAKEUPMASKTMP);
}

/* Power down one or more blocks or peripherals */
void Chip_SYSCTL_PowerDown(uint32_t powerdownmask)
{
	uint32_t pdrun;

	pdrun = LPC_SYSCTL->PDRUNCFG & PDRUNCFGMASKTMP;
	pdrun |= (powerdownmask & PDRUNCFGMASKTMP);

	LPC_SYSCTL->PDRUNCFG = (pdrun | PDRUNCFGUSEMASK);
}

/* Power up one or more blocks or peripherals */
void Chip_SYSCTL_PowerUp(uint32_t powerupmask)
{
	uint32_t pdrun;

	pdrun = LPC_SYSCTL->PDRUNCFG & PDRUNCFGMASKTMP;
	pdrun &= ~(powerupmask & PDRUNCFGMASKTMP);

	LPC_SYSCTL->PDRUNCFG = (pdrun | PDRUNCFGUSEMASK);
}
