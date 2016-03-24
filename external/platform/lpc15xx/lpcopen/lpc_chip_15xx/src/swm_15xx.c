/*
 * @brief LPC15xx Switch Matrix driver
 *
 * @note
 * Copyright(C) NXP Semiconductors, 2013
 * All rights reserved.
 *
 * @par
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * LPC products.  This software is supplied "AS IS" without any warranties of
 * any kind, and NXP Semiconductors and its licenser disclaim any and
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

#define PINASSIGN_IDX(movable)  (((movable) >> 4))
#define PINSHIFT(movable)       (8 * ((movable) & (0xF)))

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/* Assign movable pin function to physical pin in Switch Matrix */
void Chip_SWM_MovablePinAssign(CHIP_SWM_PIN_MOVABLE_T movable, uint8_t pin)
{
	uint32_t temp;
	int pinshift = PINSHIFT(movable), regIndex = PINASSIGN_IDX(movable);

	temp = LPC_SWM->PINASSIGN[regIndex] & (~(0xFF << pinshift));
	LPC_SWM->PINASSIGN[regIndex] = temp | (pin << pinshift);
}

/* Enables a fixed function pin in the Switch Matrix */
void Chip_SWM_EnableFixedPin(CHIP_SWM_PIN_FIXED_T pin)
{
	uint32_t regOff, pinPos;

	pinPos = ((uint32_t) pin) & 0x1F;
	regOff = ((uint32_t) pin) >> 7;

	/* Set low to enable fixed pin */
	LPC_SWM->PINENABLE[regOff] &= ~(1 << pinPos);
}

/* Disables a fixed function pin in the Switch Matrix */
void Chip_SWM_DisableFixedPin(CHIP_SWM_PIN_FIXED_T pin)
{
	uint32_t regOff, pinPos;

	pinPos = ((uint32_t) pin) & 0x1F;
	regOff = ((uint32_t) pin) >> 7;

	/* Set low to enable fixed pin */
	LPC_SWM->PINENABLE[regOff] |= (1 << pinPos);
}

/* Enables or disables a fixed function pin in the Switch Matrix */
void Chip_SWM_FixedPinEnable(CHIP_SWM_PIN_FIXED_T pin, bool enable)
{
	if (enable) {
		Chip_SWM_EnableFixedPin(pin);
	}
	else {
		Chip_SWM_DisableFixedPin(pin);
	}
}

/* Tests whether a fixed function pin is enabled or disabled in the Switch Matrix */
bool Chip_SWM_IsFixedPinEnabled(CHIP_SWM_PIN_FIXED_T pin)
{
	uint32_t regOff, pinPos;

	pinPos = ((uint32_t) pin) & 0x1F;
	regOff = ((uint32_t) pin) >> 7;

	return (bool) ((LPC_SWM->PINENABLE[regOff] & (1 << pinPos)) == 0);
}
