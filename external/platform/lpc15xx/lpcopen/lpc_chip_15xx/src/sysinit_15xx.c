/*
 * @brief LPC15xx Chip specific SystemInit
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

/* Clock and PLL initialization based on the internal oscillator */
void Chip_SetupIrcClocking(void)
{
	volatile int i;

	/* Powerup main IRC (likely already powered up) */
	Chip_SYSCTL_PowerUp(SYSCTL_POWERDOWN_IRC_PD);
	Chip_SYSCTL_PowerUp(SYSCTL_POWERDOWN_IRCOUT_PD);

	/* Set system PLL input to IRC */
	Chip_Clock_SetSystemPLLSource(SYSCTL_PLLCLKSRC_IRC);

	/* Power down PLL to change the PLL divider ratio */
	Chip_SYSCTL_PowerDown(SYSCTL_POWERDOWN_SYSPLL_PD);

	/* Setup PLL for main oscillator rate (FCLKIN = 12MHz) * 6 = 72MHz
	   MSEL = 5 (this is pre-decremented), PSEL = 1 (for P = 2)
	   FCLKOUT = FCLKIN * (MSEL + 1) = 12MHz * 6 = 72MHz
	   FCCO = FCLKOUT * 2 * P = 72MHz * 2 * 2 = 288MHz (within FCCO range) */
	Chip_Clock_SetupSystemPLL(5, 2);

	/* Powerup system PLL */
	Chip_SYSCTL_PowerUp(SYSCTL_POWERDOWN_SYSPLL_PD);

	/* Wait for PLL to lock */
	while (!Chip_Clock_IsSystemPLLLocked()) {}

	/* Set system clock divider to 1 */
	Chip_Clock_SetSysClockDiv(1);

	/* Setup FLASH access timing for 72MHz */
	Chip_FMC_SetFLASHAccess(SYSCTL_FLASHTIM_72MHZ_CPU);

	/* Set main clock source to the system PLL. This will drive 72MHz
	   for the main clock */
	Chip_Clock_SetMainClockSource(SYSCTL_MAINCLKSRC_SYSPLLOUT);
}

/* Clock and PLL initialization based on the external oscillator */
void Chip_SetupXtalClocking(void)
{
	volatile int i;

	/* Powerup main oscillator */
	Chip_SYSCTL_PowerUp(SYSCTL_POWERDOWN_SYSOSC_PD);

	/* Wait 200us for OSC to be stablized, no status
	   indication, dummy wait. */
	for (i = 0; i < 0x200; i++) {}

	/* Set system PLL input to main oscillator */
	Chip_Clock_SetSystemPLLSource(SYSCTL_PLLCLKSRC_MAINOSC);

	/* Power down PLL to change the PLL divider ratio */
	Chip_SYSCTL_PowerDown(SYSCTL_POWERDOWN_SYSPLL_PD);

	/* Setup PLL for main oscillator rate (FCLKIN = 12MHz) * 6 = 72MHz
	   MSEL = 5 (this is pre-decremented), PSEL = 1 (for P = 2)
	   FCLKOUT = FCLKIN * (MSEL + 1) = 12MHz * 6 = 72MHz
	   FCCO = FCLKOUT * 2 * P = 72MHz * 2 * 2 = 288MHz (within FCCO range) */
	Chip_Clock_SetupSystemPLL(5, 2);

	/* Powerup system PLL */
	Chip_SYSCTL_PowerUp(SYSCTL_POWERDOWN_SYSPLL_PD);

	/* Wait for PLL to lock */
	while (!Chip_Clock_IsSystemPLLLocked()) {}

	/* Set system clock divider to 1 */
	Chip_Clock_SetSysClockDiv(1);

	/* Setup FLASH access timing for 72MHz */
	Chip_FMC_SetFLASHAccess(SYSCTL_FLASHTIM_72MHZ_CPU);

	/* Set main clock source to the system PLL. This will drive 72MHz
	   for the main clock */
	Chip_Clock_SetMainClockSource(SYSCTL_MAINCLKSRC_SYSPLLOUT);
}

/* Set up and initialize hardware prior to call to main */
void Chip_SystemInit(void)
{
	/* Initial internal clocking */
	Chip_SetupIrcClocking();
}
