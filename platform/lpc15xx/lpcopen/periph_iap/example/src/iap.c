/*
 * @brief IAP example using IAP FLASH programming
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
#include <stdio.h>

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

#define TICKRATE_HZ (10)	/* 10 ticks per second */

/* SystemTick Counter */
static volatile uint32_t sysTick;

#define IAP_NUM_BYTES_TO_WRITE 256
/* Address of the last sector on flash */
#define last_sector_flash   0x0000F000
/* LAST SECTOR */
#define IAP_LAST_SECTOR 15

/* IAP defines*/
/* Prepare sector for write operation command */
#define IAP_PREWRRITE_CMD 50

/* Write Sector command */
#define IAP_WRISECTOR_CMD 51

/* Erase Sector command */
#define IAP_ERSSECTOR_CMD 52

/* Read PartID command */
#define IAP_REPID_CMD 54

/* IAP command variables */
static unsigned int command[5], result[4];

/* Data to write and write count */
#define WRITECOUNT (IAP_NUM_BYTES_TO_WRITE / 32)
static uint32_t array_data[WRITECOUNT];

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
	Board_LED_Toggle(0);
	sysTick++;
}

/**
 * @brief	main routine for blinky example
 * @return	Function should not exit.
 */
int main(void)
{
	int i;

	/* Generic Initialization */
	SystemCoreClockUpdate();
	Board_Init();
	Board_LED_Set(0, false);

	/* Enable SysTick Timer */
	SysTick_Config(SystemCoreClock / TICKRATE_HZ);
	
	/* Initialize the array data to be written to FLASH */
	for (i = 0; i < WRITECOUNT; i++) {
		array_data[i] = 0x11223340 + i;
	}
	
	/* Read Part Identification Number*/
	command[0] = IAP_REPID_CMD;								/* Read ID command code */
	iap_entry(command, result);

	/* Reinvoke ISP mode so that reprogamming of Flash possible */
	__disable_irq();

	command[0] = IAP_REPID_CMD;
	iap_entry(command, result);

	/* Prepare to write/erase the last sector */
	command[0] = IAP_PREWRRITE_CMD;						/* Prepare to write/erase command code */
	command[1] = IAP_LAST_SECTOR;							/* Start Sector Number */
	command[2] = IAP_LAST_SECTOR;							/* End Sector Number */
	iap_entry(command, result);

	/* Erase the last sector */
	command[0] = IAP_ERSSECTOR_CMD;						/* Erase command code*/
	command[1] = IAP_LAST_SECTOR;							/* Start Sector Number */
	command[2] = IAP_LAST_SECTOR;							/* Start Sector Number */
	iap_entry(command, result);

	/* Prepare to write/erase the last sector */
	command[0] = IAP_PREWRRITE_CMD;						/* Prepare to write/erase command code */
	command[1] = IAP_LAST_SECTOR;							/* Start Sector Number */
	command[2] = IAP_LAST_SECTOR;							/* Start Sector Number */
	iap_entry(command, result);

	/* Write to the last sector */
	command[0] = IAP_WRISECTOR_CMD;								/* Write command code */
	command[1] = (uint32_t) last_sector_flash;		/* Destination Flash Address */
	command[2] = (uint32_t) &array_data;					/* Source RAM Address */
	command[3] = IAP_NUM_BYTES_TO_WRITE;					/* Number of Bytes to be written */
	command[4] = SystemCoreClock / 1000;					/* System clock frequency */
	iap_entry(command, result);

	/* Re-enable interrupt mode */
	__enable_irq();

	while (1) {
		__WFI();
	}

	return 0;
}
