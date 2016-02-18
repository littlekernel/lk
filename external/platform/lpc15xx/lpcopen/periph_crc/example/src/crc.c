/*
 * @brief Cyclic Redundancy Check (CRC) generator example.
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

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

/* CRC data test patterns */
static const uint8_t bytes[]  = {0x38, 0x38, 0x38, 0x38};
static const uint16_t words[] = {0x3534, 0x3534, 0x3534, 0x3534};
static const uint32_t dwords[] = {0x33323130, 0x33323130, 0x33323130, 0x33323130};
static const  uint32_t expect[]  = {0x56AB, 0x7A89, 0xD7D6, 0x7D27, 0xA6669D7D};

#define TICKRATE_HZ (10)	/* 10 ticks per second */
static volatile uint32_t ticks;

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
 * @brief	Handle interrupt from SysTick timer
 * @return	Nothing
 */
void SysTick_Handler(void)
{
	ticks++;
	__SEV();
}

/**
 * @brief	Application main function
 * @return	Does not return
 * @note	This function will not return.
 */
int main(void)
{
	uint32_t result[6], gencrc;

	/* Board Initialization */
	SystemCoreClockUpdate();
	Board_Init();

	/* Chip Initialization */
	Chip_CRC_Init();

	/* Enable SysTick Timer */
	SysTick_Config(SystemCoreClock / TICKRATE_HZ);

	/* Loop tests with occasional forced error */
	while (1) {
		result[0] = Chip_CRC_CRC8(&bytes[0], 1);
		result[1] = Chip_CRC_CRC8(bytes, (sizeof(bytes) / sizeof(bytes[0])));
		result[2] = Chip_CRC_CRC16(&words[0], 1);
		if (ticks % 2 == 0) {
			/* introduce bit errors on every other tick */
			result[2] -= 1;
		}
		result[3] = Chip_CRC_CRC16(words, (sizeof(words) / sizeof(words[0])));
		result[4] = Chip_CRC_CRC32(&dwords[0], 1);
		result[5] = Chip_CRC_CRC32(dwords, (sizeof(dwords) / sizeof(dwords[0])));

		gencrc = Chip_CRC_CRC32((uint32_t *) result, 5);
		result[0] = Chip_CRC_CRC32((uint32_t *) expect, 5);
		if (result[0] != gencrc) {
			Board_LED_Set(0, true);
		}
		else {
			Board_LED_Set(0, false);
		}

		/* Wait for tick */
		__WFE();
	}
	return 0;
}
