/*
 * @brief This file contains millisecond timer routines.
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

#include "board.h"

#ifndef _MS_TIMER_H
#define _MS_TIMER_H

#ifdef __cplusplus
extern "C"
{
#endif

/** @ingroup EXAMPLES_USBDROM_15XX_HID_KEYBOARD
 * @{
 */

/**
 * @brief	Structure to hold millisecond timer data
 */

typedef struct  {
	int32_t     start;			/*!< SysTick count when current timer started. */
	int32_t     interval;		/*!< Timer interval. */
} ms_timer_t;

/* exported data */
extern volatile uint32_t g_msTicks;			/*!<  1 millisecond counter incremented in SysTick_IRQ() */

/**
 * @brief	Setup Systick to generate 1 msec interrupt.
 * @return	Nothing
 */
static INLINE void systick_init(void)
{
	if (SysTick_Config(Chip_Clock_GetSystemClockRate() / 1000)) {	/* Setup SysTick Timer for 1 msec interrupts  */
		while (1) {									/* Capture error */
		}
	}
	NVIC_SetPriority(SysTick_IRQn, 0x0);
}

/**
 * @brief	Gets millisecond count from board startup.
 * @return	Nothing
 */
static INLINE uint32_t systick_Count(void)
{
	return g_msTicks;
}

/**
 * @brief	Initialize the given timer object and also start the timer.
 * @param	t		: Pointer to timer object
 * @param	interval: Timer interval in milliseconds
 * @return	Nothing
 */
static INLINE void ms_timerInit(ms_timer_t *t, int32_t interval)
{
	t->interval = interval;
	t->start = (int32_t) g_msTicks;
}

/**
 * @brief	Initialize the given timer object without starting the timer.
 * @param	t		: Pointer to timer object
 * @param	interval: Timer interval in milliseconds
 * @return	Nothing
 */
static INLINE void ms_timerInitOnly(ms_timer_t *t, int32_t interval)
{
	t->interval = interval;
	t->start = 0;
}

/**
 * @brief	Stop the timer.
 * @param	t	: Pointer to timer object
 * @return	Nothing
 */
static INLINE void ms_timerStop(ms_timer_t *t)
{
	t->start = 0;
}

/**
 * @brief	Check if the timer has started.
 * @param	t	: Pointer to timer object
 * @return	true if the timer has started, otherwise false
 */
static INLINE bool ms_timerStarted(ms_timer_t *t)
{
	return (bool) (t->start != 0);
}

/**
 * @brief	Start the timer.
 * @param	t	: Pointer to timer object
 * @return	Nothing
 */
static INLINE void ms_timerStart(ms_timer_t *t)
{
	t->start = (g_msTicks == 0) ? 1 : (int32_t) g_msTicks;
}

/**
 * @brief	Check if timer has expired.
 * @param	t	: Pointer to timer object
 * @return	true if the timer has expired, otherwise false
 */
static INLINE bool ms_timerExpired(ms_timer_t *t)
{
	return (bool) (((int32_t) g_msTicks - t->start) >= t->interval);
}

/**
 * @brief	Waits for given number of milliseconds
 * @param	n	: Number of milliseconds
 * @return	Nothing
 */
extern void ms_timerDelay(uint32_t n);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
#endif /* end _MS_TIMER_H */
