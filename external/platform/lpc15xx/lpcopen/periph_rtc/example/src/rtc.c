/*
 * @brief RTC example with wakeup
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

#include <stdlib.h>
#include <string.h>
#include "board.h"

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

/* RTC interrupt flags */
static volatile bool rtcWake, rtcAlarm;

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/**
 * @brief	RTC alarm interrupt Handler
 * @return	None
 */
void RTC_ALARM_IRQHandler(void)
{
	uint32_t rtcStatus;

	Board_LED_Toggle(1);

	/* Get RTC status register */
	rtcStatus = Chip_RTC_GetStatus(LPC_RTC);

	/* Check RTC 1Khz match interrupt */
	if (rtcStatus & RTC_CTRL_ALARM1HZ) {
		/* Alarm */
		rtcAlarm = true;
	}

	/* Clear only latched RTC alarm status */
	Chip_RTC_EnableOptions(LPC_RTC, rtcStatus & RTC_CTRL_ALARM1HZ);
}

/**
 * @brief	RTC wake interrupt Handler
 * @return	None
 */
void RTC_WAKE_IRQHandler(void)
{
	uint32_t rtcStatus;

	Board_LED_Toggle(0);

	/* Get RTC status register */
	rtcStatus = Chip_RTC_GetStatus(LPC_RTC);

	/* Check RTC 1KHz match interrupt */
	if (rtcStatus & RTC_CTRL_WAKE1KHZ) {
		/* RTC high resultiuon wakeup interrupt */
		rtcWake = true;
	}

	/* Clear only latched RTC wake status */
	Chip_RTC_EnableOptions(LPC_RTC, rtcStatus & RTC_CTRL_WAKE1KHZ);
}

/**
 * @brief	Main program body
 * @return	int
 */
int main(void)
{
	int stateCounter = 0;
	uint32_t ticks = 0;

	/* Setup SystemCoreClock and any needed board code */
	SystemCoreClockUpdate();
	Board_Init();

	/* Enable the RTC oscillator, oscillator rate can be determined by
	   calling Chip_Clock_GetRTCOscRate()	*/
	Chip_Clock_EnableRTCOsc();

	/* Initialize RTC driver (enables RTC clocking) */
	Chip_RTC_Init(LPC_RTC);

	/* Enable RTC as a peripheral wakeup event */
	Chip_SYSCTL_EnableERP1PeriphWakeup(SYSCTL_ERP1_WAKEUP_RTCALARMINT |
									   SYSCTL_ERP1_WAKEUP_RTCWAKEINT);

	/* RTC reset */
	Chip_RTC_Reset(LPC_RTC);

	/* Start RTC at a count of 0 when RTC is disabled. If the RTC is enabled, you
	   need to disable it before setting the initial RTC count. */
	Chip_RTC_Disable(LPC_RTC);
	Chip_RTC_SetCount(LPC_RTC, 0);

	/* Set a long alarm time so the interrupt won't trigger */
	Chip_RTC_SetAlarm(LPC_RTC, 1000);

	/* Enable RTC and high resolution timer - this can be done in a single
	   call with Chip_RTC_EnableOptions(LPC_RTC, (RTC_CTRL_RTC1KHZ_EN | RTC_CTRL_RTC_EN)); */
	Chip_RTC_Enable1KHZ(LPC_RTC);
	Chip_RTC_Enable(LPC_RTC);

	/* Clear latched RTC interrupt statuses */
	Chip_RTC_ClearStatus(LPC_RTC, (RTC_CTRL_OFD | RTC_CTRL_ALARM1HZ | RTC_CTRL_WAKE1KHZ));

	/* Enable RTC wake and alarm interrupts */
	NVIC_EnableIRQ(RTC_ALARM_IRQn);
	NVIC_EnableIRQ(RTC_WAKE_IRQn);

	/* Enable RTC alarm interrupt */
	Chip_RTC_EnableWakeup(LPC_RTC, (RTC_CTRL_ALARMDPD_EN | RTC_CTRL_WAKEDPD_EN));

	/* Sleep and do all the work in the RTC interrupt handler */
	while (1) {
		ticks = 0;
		DEBUGOUT("Tick number: %d, 1KHZ int:%d, alarm int:%d\r\n",
				 stateCounter, rtcWake, rtcAlarm);
		rtcWake = rtcAlarm = false;

		/* 10 high resolution ticks that get slower each tick */
		if (stateCounter < 10) {
			/* Wakeup in 300, 400, 500, etc. milliSeconds */
			Chip_RTC_SetWake(LPC_RTC, (300 + (stateCounter * 100)));

			stateCounter++;
		}
		else {
			DEBUGOUT("Setting alarm to wake up in 4s\r\n");

			/* Set alarm to wakeup in 4 seconds */
			Chip_RTC_SetAlarm(LPC_RTC, Chip_RTC_GetCount(LPC_RTC) + 4);

			stateCounter = 0;
		}

		Chip_SYSCTL_SetWakeup(~(SYSCTL_SLPWAKE_IRCOUT_PD | SYSCTL_SLPWAKE_IRC_PD |
								SYSCTL_SLPWAKE_FLASH_PD | SYSCTL_SLPWAKE_SYSOSC_PD | SYSCTL_SLPWAKE_SYSPLL_PD));
		Chip_SYSCTL_EnableERP1PeriphWakeup(SYSCTL_ERP1_WAKEUP_RTCALARMINT | SYSCTL_ERP1_WAKEUP_RTCWAKEINT);
		
		/* Delay to allow serial transmission to complete*/
		while(ticks++ < 10000) {}

		/* Enter MCU Deep Sleep mode */
		Chip_PMU_DeepSleepState(LPC_PMU);
	}

	return 0;
}
