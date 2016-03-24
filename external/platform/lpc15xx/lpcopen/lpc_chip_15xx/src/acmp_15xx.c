/*
 * @brief LPC15xx Analog comparator driver
 *
 * Copyright(C) NXP Semiconductors, 2014
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

/* Initializes the ACMP */
void Chip_ACMP_Init(LPC_CMP_T *pACMP)
{
	Chip_SYSCTL_PowerUp(SYSCTL_POWERDOWN_ACMP0_PD | SYSCTL_POWERDOWN_ACMP1_PD |
						SYSCTL_POWERDOWN_ACMP2_PD | SYSCTL_POWERDOWN_ACMP3_PD);
	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_ACMP);
	Chip_SYSCTL_PeriphReset(RESET_ACMP);
}

/* De-initializes the ACMP */
void Chip_ACMP_Deinit(LPC_CMP_T *pACMP)
{
	Chip_Clock_DisablePeriphClock(SYSCTL_CLOCK_ACMP);
	Chip_SYSCTL_PowerDown(SYSCTL_POWERDOWN_ACMP0_PD | SYSCTL_POWERDOWN_ACMP1_PD |
						  SYSCTL_POWERDOWN_ACMP2_PD | SYSCTL_POWERDOWN_ACMP3_PD);
}

/*Sets up ACMP edge selection */
void Chip_ACMP_SetIntEdgeSelection(LPC_CMP_T *pACMP, uint8_t index, CHIP_ACMP_EDGESEL_T edgeSel)
{
	/* Make sure interrupt flag is not set during read OR/AND and write operation */
	uint32_t reg = (pACMP->ACMP[index].CMP & ~ACMP_INTFLAG_BIT) & ~ACMP_INTEDGE_MASK;

	pACMP->ACMP[index].CMP = reg | (uint32_t) edgeSel;
}

/*Selects positive voltage input */
void Chip_ACMP_SetPosVoltRef(LPC_CMP_T *pACMP, uint8_t index, CHIP_ACMP_POS_INPUT_T Posinput)
{
	/* Make sure interrupt flag is not set during read OR/AND and write operation */
	uint32_t reg = (pACMP->ACMP[index].CMP & ~ACMP_INTFLAG_BIT) & ~ACMP_COMPVPSEL_MASK;

	/* Select positive input */
	pACMP->ACMP[index].CMP = reg | (uint32_t) Posinput;
}

/*Selects negative voltage input */
void Chip_ACMP_SetNegVoltRef(LPC_CMP_T *pACMP, uint8_t index, CHIP_ACMP_NEG_INPUT_T Neginput)
{
	/* Make sure interrupt flag is not set during read OR/AND and write operation */
	uint32_t reg = (pACMP->ACMP[index].CMP & ~ACMP_INTFLAG_BIT) & ~ACMP_COMPVMSEL_MASK;

	/* Select negative input */
	pACMP->ACMP[index].CMP = reg | (uint32_t) Neginput;
}

/*Selects hysteresis level */
void Chip_ACMP_SetHysteresis(LPC_CMP_T *pACMP, uint8_t index, CHIP_ACMP_HYS_T hys)
{
	/* Make sure interrupt flag is not set during read OR/AND and write operation */
	uint32_t reg = (pACMP->ACMP[index].CMP & ~ACMP_INTFLAG_BIT) & ~ACMP_HYSTERESIS_MASK;

	pACMP->ACMP[index].CMP = reg | (uint32_t) hys;
}

/*Helper function for setting up ACMP voltage settings */
void Chip_ACMP_SetupACMPRefs(LPC_CMP_T *pACMP, uint8_t index, CHIP_ACMP_POS_INPUT_T Posinput,
							 CHIP_ACMP_NEG_INPUT_T Neginput, CHIP_ACMP_HYS_T hys)
{
	/* Make sure interrupt flag is not set during read OR/AND and write operation */
	uint32_t reg = (pACMP->ACMP[index].CMP & ~ACMP_INTFLAG_BIT) & ~(ACMP_HYSTERESIS_MASK |
																	ACMP_COMPVMSEL_MASK | ACMP_COMPVPSEL_MASK);

	pACMP->ACMP[index].CMP = reg | (uint32_t) Posinput | (uint32_t) Neginput | (uint32_t) hys;
}

/*Helper function for setting up ACMP interrupt settings */
void Chip_ACMP_SetupACMPInt(LPC_CMP_T *pACMP, uint8_t index, bool level,
							bool invert, CHIP_ACMP_EDGESEL_T edgeSel)
{
	/* Make sure interrupt flag is not set during read OR/AND and write operation */
	uint32_t reg = (pACMP->ACMP[index].CMP & ~ACMP_INTFLAG_BIT) & ~(ACMP_INTPOL_BIT |
																	ACMP_INTTYPE_BIT | ACMP_INTEDGE_MASK);
	/* For Level triggered interrupt, invert sets the polarity
	     For edge triggered interrupt edgeSel sets the edge type */
	if (level) {
		reg |= ACMP_INTTYPE_BIT;
		if (invert) {
			reg |= ACMP_INTPOL_BIT;
		}
	}
	else {
		reg |= (uint32_t) edgeSel;
	}

	pACMP->ACMP[index].CMP = reg;
}

/*Sets up voltage ladder */
void Chip_ACMP_SetupVoltLadder(LPC_CMP_T *pACMP, uint8_t index, uint32_t ladsel, bool ladrefVDDCMP)
{
	/* Make sure interrupt flag is not set during read OR/AND and write operation */
	uint32_t reg = (pACMP->ACMP[index].CMP & ~ACMP_INTFLAG_BIT) & ~(ACMP_LADSEL_MASK | ACMP_LADREF_BIT);

	/* Setup voltage ladder and ladder reference */
	if (!ladrefVDDCMP) {
		reg |= ACMP_LADREF_BIT;
	}
	pACMP->ACMP[index].CMP = reg | (ladsel << 24);
}
