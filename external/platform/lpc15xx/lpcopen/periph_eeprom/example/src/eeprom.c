/*
 * @brief EEPROM example
 * This example show how to use the EEPROM interface
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
#include "string.h"

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

/* Systick interrupt rate */
#define TICKRATE_HZ (10)	/* 10 ticks per second */

/* EEPROM Address used for storage */
#define EEPROM_ADDRESS      0x00000100

/* Write count */
#define IAP_NUM_BYTES_TO_READ_WRITE 32

/* Tag for checking if a string already exists in EEPROM */
#define CHKTAG          "NxP"
#define CHKTAG_SIZE     3

/* ASCII ESC character code */
#define ESC_CHAR        27

/* Read/write buffer (32-bit aligned) */
uint32_t buffer[IAP_NUM_BYTES_TO_READ_WRITE / sizeof(uint32_t)];

/* Test string for no DEBUG */
#define TESTSTRING "12345678"

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/* Show current string stored in UART */
static void ShowString(char *str) {
	int stSize;

	/* Is data tagged with check pattern? */
	if (strncmp(str, CHKTAG, CHKTAG_SIZE) == 0) {
		/* Get next byte, which is the string size in bytes */
		stSize = (uint32_t) str[3];
		if (stSize > 32) {
			stSize = 32;
		}

		/* Add terminator */
		str[4 + stSize] = '\0';

		/* Show string stored in EEPROM */
		DEBUGSTR("Stored string found in EEEPROM\r\n");
		DEBUGSTR("-->");
		DEBUGSTR((char *) &str[4]);
		DEBUGSTR("<--\r\n");
	}
	else {
		DEBUGSTR("No string stored in the EEPROM\r\n");
	}
}

/* Get a string to save from the UART */
static uint32_t MakeString(uint8_t *str)
{
	int index, byte;
	char strOut[2];

	/* Get a string up to 32 bytes to write into EEPROM */
	DEBUGSTR("\r\nEnter a string to write into EEPROM\r\n");
	DEBUGSTR("Up to 32 bytes in length, press ESC to accept\r\n");

	/* Setup header */
	strncpy((char *) str, CHKTAG, CHKTAG_SIZE);

#if defined(DEBUG_ENABLE)
	/* Read until escape, but cap at 32 characters */
	index = 0;
	strOut[1] = '\0';
	byte = DEBUGIN();
	while ((index < 32) && (byte != ESC_CHAR)) {
		if (byte != EOF) {
			strOut[0] = str[4 + index] = (uint8_t) byte;
			DEBUGSTR(strOut);
			index++;
		}

		byte = DEBUGIN();
	}
#else
	/* Suppress warnings */
	(void) byte;
	(void) strOut;

	/* Debug input not enabled, so use a pre-setup string */
	strcpy((char *) &str[4], TESTSTRING);
	index = strlen(TESTSTRING);
#endif

	str[3] = (uint8_t) index;

	return (uint32_t) index;
}

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
}

/**
 * @brief	main routine for EEPROM example
 * @return	Always returns 0
 */
int main(void)
{
	uint8_t *ptr = (uint8_t *) buffer;
	uint8_t ret_code;

	/* Generic Initialization */
	SystemCoreClockUpdate();
	Board_Init();

	/* Enable SysTick Timer */
	SysTick_Config(SystemCoreClock / TICKRATE_HZ);

	/* Enable EEPROM clock and reset EEPROM controller */
	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_EEPROM);
	Chip_SYSCTL_PeriphReset(RESET_EEPROM);

	/* Data to be read from EEPROM */
	ret_code = Chip_EEPROM_Read(EEPROM_ADDRESS, ptr, IAP_NUM_BYTES_TO_READ_WRITE);

	/* Error checking */
	if (ret_code != IAP_CMD_SUCCESS) {
		DEBUGOUT("Command failed to execute, return code is: %x\r\n", ret_code);
	}

	/* Check and display string if it exists */
	ShowString((char *) ptr);

	/* Get a string to save */
	MakeString(ptr);

	/* Data to be written to EEPROM */
	ret_code = Chip_EEPROM_Write(EEPROM_ADDRESS, ptr, IAP_NUM_BYTES_TO_READ_WRITE);

	/* Error checking */
	if (ret_code == IAP_CMD_SUCCESS) {
		DEBUGSTR("EEPROM write passed\r\n");
	}
	else {
		DEBUGOUT("EEPROM write failed, return code is: %x\r\n", ret_code);
	}

	return 0;
}
