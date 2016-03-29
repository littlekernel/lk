/*
 * @brief LPC15xx SCTIPU driver
 *
 * @note
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

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/* Sets up an configuration and input source for a SCTIPU output channel */
void Chip_SCTIPU_ConfigSample(uint8_t ch, uint8_t useb, uint8_t sampIn, uint8_t useLatch)
{
	uint32_t reg;

	/* Get current sample control register states and mask off channel
	   specific bits. Set reserved bits states to 0. */
	reg = LPC_SCTIPU->SAMPLE_CTRL & ~(SCTIPU_CTRL_INSELMASK(ch) |
									  SCTIPU_CTRL_SAMPENDMASK(ch) | SCTIPU_CTRL_LATCHENMASK(ch) |
									  SCTIPU_RESERVED_BITS);

	/* Setup channel specific configuration */
	reg |= SCTIPU_CTRL_INSEL(ch, useb) | SCTIPU_CTRL_SAMPENDSEL(ch, sampIn) |
		   SCTIPU_CTRL_LATCHENSEL(ch, useLatch);
	LPC_SCTIPU->SAMPLE_CTRL = reg;
}
