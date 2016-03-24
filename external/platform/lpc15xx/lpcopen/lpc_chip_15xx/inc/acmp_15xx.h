/*
 * @brief LPC15xx Analog comparator driver
 *
 * @note
 * Copyright(C) NXP Semiconductors, 2014
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

#ifndef __ACMP_15XX_H_
#define __ACMP_15XX_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup ACMP_15XX CHIP: LPC15xx Analog Comparator driver
 * @ingroup CHIP_15XX_Drivers
 * @{
 */

/**
 * @brief Analog Comparator channel register block structure
 */
typedef struct {
	__IO uint32_t  CMP;			/*!< Individual Comparator control register */
	__IO uint32_t  CMPFILTR;	/*!< Individual Comparator Filter registers */
} CMP_REG_T;

/**
 * @brief Analog Comparator register block structure
 */
typedef struct {					/*!< ACMP Structure */
	__IO uint32_t  CTRL;		/*!< Comparator block control register */
	__IO CMP_REG_T ACMP[4];		/*!< Individual Comparator registers */
} LPC_CMP_T;

/* Bit definitions for block control register */
#define ACMP_ROSCCTL_BIT     (1 << 8)		/* Ring Oscillator control bit */
#define ACMP_EXTRESET_BIT    (1 << 9)		/* Reset source for ring oscillator 0 - Internal, 1 - External pin */

/* Bit definitions for compare register */
#define ACMP_CMPEN_BIT       (1 << 0)		/* Comparator enable bit */
#define ACMP_INTEN_BIT       (1 << 2)		/* Comparator Interrupt enable bit */
#define ACMP_STATUS_BIT      (1 << 3)		/* Comparator status, reflects the state of the comparator output */
#define ACMP_COMPVMSEL_MASK  (0x7 << 4)		/* Mask for VM Input selection */
#define ACMP_COMPVPSEL_MASK  (0x7 << 8)		/* Mask for VP Input selection */
#define ACMP_HYSTERESIS_MASK (0x3 << 13)	/* Mask for Hysterisis Control */
#define ACMP_INTPOL_BIT      (1 << 15)		/* Polarity of CMP output for interrupt 0 - Not Inverted, 1 - Inverted */
#define ACMP_INTTYPE_BIT     (1 << 16)		/* Interrupt Type 0 - Edge, 1 - Level */
#define ACMP_INTEDGE_MASK    (0x3 << 17)	/* Mask for Interrupt edge selection */
#define ACMP_INTFLAG_BIT     (1 << 19)		/* Interrupt Flag bit */
#define ACMP_LADENAB_BIT     (1 << 20)		/* Voltage ladder enable bit */
#define ACMP_LADREF_BIT      (1 << 22)		/* Voltage reference select bit for voltage ladder */
#define ACMP_LADSEL_MASK     (0x1F << 24)	/* Reference voltage selection mask for ladder */
#define ACMP_PROPDLY_MASK    (0x3 << 29)	/* Propogation delay mask */

/* Bit definitions for comparator filter register */
#define ACMP_SMODE_MASK      (0x3 << 0)		/* Mask for digital filter sample mode */
#define ACMP_CLKDIV_MASK     (0x7 << 2)		/* Mask for comparator clock */

/** Edge selection for comparator */
typedef enum {
	ACMP_EDGESEL_FALLING = (0 << 17),	/* Set the COMPEDGE bit on falling edge */
	ACMP_EDGESEL_RISING  = (1 << 17),	/* Set the COMPEDGE bit on rising edge */
	ACMP_EDGESEL_BOTH    = (2 << 17)	/* Set the COMPEDGE bit on falling and rising edges */
} CHIP_ACMP_EDGESEL_T;

/** Hysteresis selection for comparator */
typedef enum {
	ACMP_HYS_NONE = (0 << 13),	/* No hysteresis (the output will switch as the voltages cross) */
	ACMP_HYS_5MV  = (1 << 13),	/* 5mV hysteresis */
	ACMP_HYS_10MV = (2 << 13),	/* 10mV hysteresis */
	ACMP_HYS_15MV = (3 << 13)	/* 20mV hysteresis */
} CHIP_ACMP_HYS_T;

/**
 * Analog Comparator positive input values
 */
typedef enum CHIP_ACMP_POS_INPUT {
	ACMP_POSIN_VREF_DIV  = (0 << 8),	/*!< Voltage ladder output */
	ACMP_POSIN_ACMP_I1   = (1 << 8),	/*!< ACMP_I1 pin */
	ACMP_POSIN_ACMP_I2   = (2 << 8),	/*!< ACMP_I2 pin */
	ACMP_POSIN_ACMP_I3   = (3 << 8),	/*!< ACMP_I3 pin */
	ACMP_POSIN_ACMP_I4   = (4 << 8),	/*!< ACMP_I4 pin */
	ACMP_POSIN_INT_REF   = (5 << 8),	/*!< Internal reference voltage */
	ACMP_POSIN_ADCIN_1   = (6 << 8),	/*!< ADC Input or Temperature sensor varies with comparator */
	ACMP_POSIN_ADCIN_2   = (7 << 8)		/*!< ADC Input varies with comparator */
} CHIP_ACMP_POS_INPUT_T;

/**
 * Analog Comparator negative input values
 */
typedef enum CHIP_ACMP_NEG_INPUT {
	ACMP_NEGIN_VREF_DIV  = (0 << 4),	/*!< Voltage ladder output */
	ACMP_NEGIN_ACMP_I1   = (1 << 4),	/*!< ACMP_I1 pin */
	ACMP_NEGIN_ACMP_I2   = (2 << 4),	/*!< ACMP_I2 pin */
	ACMP_NEGIN_ACMP_I3   = (3 << 4),	/*!< ACMP_I3 pin */
	ACMP_NEGIN_ACMP_I4   = (4 << 4),	/*!< ACMP_I4 pin */
	ACMP_NEGIN_INT_REF   = (5 << 4),	/*!< Internal reference voltage */
	ACMP_NEGIN_ADCIN_1   = (6 << 4),	/*!< ADC Input or Temperature sensor varies with comparator */
	ACMP_NEGIN_ADCIN_2   = (7 << 4)		/*!< ADC Input varies with comparator */
} CHIP_ACMP_NEG_INPUT_T;

/**
 * Analog Comparator sample mode values
 */
typedef enum {
	ACMP_SMODE_0 = 0,	/*!< Bypass filter */
	ACMP_SMODE_1,		/*!< Reject pulses shorter than 1 filter clock cycle */
	ACMP_SMODE_2,		/*!< Reject pulses shorter than 2 filter clock cycle */
	ACMP_SMODE_3		/*!< Reject pulses shorter than 3 filter clock cycle */
} CHIP_ACMP_SMODE_T;

/**
 * Analog Comparator clock divider values
 */
typedef enum {
	ACMP_CLKDIV_1  =  (0x0 << 2),	/*!< Use CMP_PCLK */
	ACMP_CLKDIV_2  =  (0x1 << 2),	/*!< Use CMP_PCLK/2 */
	ACMP_CLKDIV_4  =  (0x2 << 2),	/*!< Use CMP_PCLK/4 */
	ACMP_CLKDIV_8  =  (0x3 << 2),	/*!< Use CMP_PCLK/8 */
	ACMP_CLKDIV_16 =  (0x4 << 2),	/*!< Use CMP_PCLK/16 */
	ACMP_CLKDIV_32 =  (0x5 << 2),	/*!< Use CMP_PCLK/32 */
	ACMP_CLKDIV_64 =  (0x6 << 2)	/*!< Use CMP_PCLK/64 */
} CHIP_ACMP_CLKDIV_T;

/**
 * @brief	Initializes the ACMP
 * @param	pACMP	: Pointer to Analog Comparator block
 * @return	Nothing
 */
void Chip_ACMP_Init(LPC_CMP_T *pACMP);

/**
 * @brief	Deinitializes the ACMP
 * @param	pACMP	: Pointer to Analog Comparator block
 * @return	Nothing
 */
void Chip_ACMP_Deinit(LPC_CMP_T *pACMP);

/**
 * @brief	Enable the comparator
 * @param	pACMP	: Pointer to Analog Comparator block
 * @param	index	: index to the comparator (0 - 3)
 * @return	Nothing
 */
STATIC INLINE void Chip_ACMP_EnableComp(LPC_CMP_T *pACMP, uint8_t index)
{
	/* Make sure interrupt flag is not set during read write operation */
	pACMP->ACMP[index].CMP = (pACMP->ACMP[index].CMP & ~ACMP_INTFLAG_BIT) | ACMP_CMPEN_BIT;
}

/**
 * @brief	Disable the comparator
 * @param	pACMP	: Pointer to Analog Comparator block
 * @param	index	: index to the comparator (0 - 3)
 * @return	Nothing
 */
STATIC INLINE void Chip_ACMP_DisableComp(LPC_CMP_T *pACMP, uint8_t index)
{
	pACMP->ACMP[index].CMP = (pACMP->ACMP[index].CMP & ~ACMP_INTFLAG_BIT) & ~ACMP_CMPEN_BIT;
}

/**
 * @brief	Enable the interrupt for the comparator
 * @param	pACMP	: Pointer to Analog Comparator block
 * @param	index	: index to the comparator (0 - 3)
 * @return	Nothing
 */
STATIC INLINE void Chip_ACMP_EnableCompInt(LPC_CMP_T *pACMP, uint8_t index)
{
	/* Make sure interrupt flag is not set during read write operation */
	pACMP->ACMP[index].CMP = (pACMP->ACMP[index].CMP & ~ACMP_INTFLAG_BIT) | ACMP_INTEN_BIT;
}

/**
 * @brief	Disable the interrupt for the comparator
 * @param	pACMP	: Pointer to Analog Comparator block
 * @param	index	: index to the comparator (0 - 3)
 * @return	Nothing
 */
STATIC INLINE void Chip_ACMP_DisableCompInt(LPC_CMP_T *pACMP, uint8_t index)
{
	/* Make sure interrupt flag is not set during read write operation */
	pACMP->ACMP[index].CMP = (pACMP->ACMP[index].CMP & ~ACMP_INTFLAG_BIT) & ~ACMP_INTEN_BIT;
}

/**
 * @brief	Returns the current comparator status
 * @param	pACMP	: Pointer to Analog Comparator block
 * @param	index	: index to the comparator (0 - 3)
 * @return TRUE if ACMP_STATUS_BIT is set else returns FALSE
 */
STATIC INLINE bool Chip_ACMP_GetCompStatus(LPC_CMP_T *pACMP, uint8_t index)
{
	return (pACMP->ACMP[index].CMP & ACMP_STATUS_BIT) != 0;
}

/**
 * @brief	Selects positive voltage input
 * @param	pACMP	: Pointer to Analog Comparator block
 * @param	index	: index to the comparator (0 - 3)
 * @param	Posinput: one of the positive input voltage sources
 * @return	Nothing
 */
void Chip_ACMP_SetPosVoltRef(LPC_CMP_T *pACMP, uint8_t index, CHIP_ACMP_POS_INPUT_T Posinput);

/**
 * @brief	Selects negative voltage input
 * @param	pACMP	: Pointer to Analog Comparator block
 * @param	index	: index to the comparator (0 - 3)
 * @param	Neginput: one of the negative input voltage sources
 * @return	Nothing
 */
void Chip_ACMP_SetNegVoltRef(LPC_CMP_T *pACMP, uint8_t index, CHIP_ACMP_NEG_INPUT_T Neginput);

/**
 * @brief	Selects hysteresis level
 * @param	pACMP	: Pointer to Analog Comparator block
 * @param	index	: index to the comparator (0 - 3)
 * @param   hys : Selected Hysteresis level
 * @return	Nothing
 */
void Chip_ACMP_SetHysteresis(LPC_CMP_T *pACMP, uint8_t index, CHIP_ACMP_HYS_T hys);

/**
 * @brief	Set the ACMP interrupt polarity (INTPOL bit)
 * @param	pACMP	: Pointer to Analog Comparator block
 * @param	index	: index to the comparator (0 - 3)
 * @return	Nothing
 */
STATIC INLINE void Chip_ACMP_SetIntPolarity(LPC_CMP_T *pACMP, uint8_t index)
{
	/* Make sure interrupt flag is not set during read write operation */
	pACMP->ACMP[index].CMP = (pACMP->ACMP[index].CMP & ~ACMP_INTFLAG_BIT) | ACMP_INTPOL_BIT;
}

/**
 * @brief	Clear the ACMP interrupt polarity (INTPOL bit)
 * @param	pACMP	: Pointer to Analog Comparator block
 * @param	index	: index to the comparator (0 - 3)
 * @return	Nothing
 */
STATIC INLINE void Chip_ACMP_ClearIntPolarity(LPC_CMP_T *pACMP, uint8_t index)
{
	/* Make sure interrupt flag is not set during read write operation */
	pACMP->ACMP[index].CMP = (pACMP->ACMP[index].CMP & ~ACMP_INTFLAG_BIT) & ~ACMP_INTPOL_BIT;
}

/**
 * @brief	Set the ACMP interrupt type as edge (INTTYPE bit)
 * @param	pACMP	: Pointer to Analog Comparator block
 * @param	index	: index to the comparator (0 - 3)
 * @return	Nothing
 */
STATIC INLINE void Chip_ACMP_SetIntTypeEdge(LPC_CMP_T *pACMP, uint8_t index)
{
	/* Make sure interrupt flag is not set during read write operation */
	pACMP->ACMP[index].CMP = (pACMP->ACMP[index].CMP & ~ACMP_INTFLAG_BIT) & ~ACMP_INTTYPE_BIT;
}

/**
 * @brief	Set the ACMP interrupt type as level (INTTYPE bit)
 * @param	pACMP	: Pointer to Analog Comparator block
 * @param	index	: index to the comparator (0 - 3)
 * @return	Nothing
 */
STATIC INLINE void Chip_ACMP_SetIntTypeLevel(LPC_CMP_T *pACMP, uint8_t index)
{
	/* Make sure interrupt flag is not set during read write operation */
	pACMP->ACMP[index].CMP = (pACMP->ACMP[index].CMP & ~ACMP_INTFLAG_BIT) | ACMP_INTTYPE_BIT;
}

/**
 * @brief	Sets up ACMP edge selection
 * @param	pACMP	: Pointer to Analog Comparator block
 * @param	index	: index to the comparator (0 - 3)
 * @param	edgeSel	: Edge selection value
 * @return	Nothing
 */
void Chip_ACMP_SetIntEdgeSelection(LPC_CMP_T *pACMP, uint8_t index, CHIP_ACMP_EDGESEL_T edgeSel);

/**
 * @brief	Get the ACMP interrupt flag bit(INTFLAG bit)
 * @param	pACMP	: Pointer to Analog Comparator block
 * @param	index	: index to the comparator (0 - 3)
 * @return	TRUE if ACMP_INTFLAG_BIT is set else returns FALSE
 */
STATIC INLINE bool Chip_ACMP_GetIntFlag(LPC_CMP_T *pACMP, uint8_t index)
{
	return (pACMP->ACMP[index].CMP & ACMP_INTFLAG_BIT) != 0;
}

/**
 * @brief	Clears the ACMP interrupt flag bit (INTFLAG bit)
 * @param	pACMP	: Pointer to Analog Comparator block
 * @param	index	: index to the comparator (0 - 3)
 * @return	Nothing
 */
STATIC INLINE void Chip_ACMP_ClearIntFlag(LPC_CMP_T *pACMP, uint8_t index)
{
	pACMP->ACMP[index].CMP |= ACMP_INTFLAG_BIT;
}

/**
 * @brief	Helper function for setting up ACMP voltage settings
 * @param	pACMP		: Pointer to Analog Comparator block
 * @param	index		: index to the comparator (0 - 3)
 * @param	Posinput	: one of the positive input voltage sources
 * @param	Neginput	: one of the negative input voltage sources
 * @param	hys			: Selected Hysteresis level
 * @return	Nothing
 */
void Chip_ACMP_SetupACMPRefs(LPC_CMP_T *pACMP, uint8_t index,
							 CHIP_ACMP_POS_INPUT_T Posinput, CHIP_ACMP_NEG_INPUT_T Neginput,
							 CHIP_ACMP_HYS_T hys);

/**
 * @brief	Helper function for setting up ACMP interrupt settings
 * @param	pACMP	: Pointer to Analog Comparator block
 * @param	index	: index to the comparator (0 - 3)
 * @param	level	: interrupt type false - edge, true - level
 * @param	invert	: polarity of CMP output for interrupt, false - Not Inverted, true - Inverted
 * @param	edgeSel	: Edge selection value
 * @return	Nothing
 */
void Chip_ACMP_SetupACMPInt(LPC_CMP_T *pACMP, uint8_t index, bool level,
							bool invert, CHIP_ACMP_EDGESEL_T edgeSel);

/**
 * @brief	Sets up voltage ladder
 * @param	pACMP			: Pointer to Analog Comparator block
 * @param	index			: index to the comparator (0 - 3)
 * @param	ladsel			: Voltage ladder value (0 .. 31)
 * @param	ladrefVDDCMP	: Selects the reference voltage Vref for the voltage ladder
 *							        true for VDD, false for VDDCMP pin
 * @return	Nothing
 */
void Chip_ACMP_SetupVoltLadder(LPC_CMP_T *pACMP, uint8_t index, uint32_t ladsel, bool ladrefVDDCMP);

/**
 * @brief	Enables voltage ladder
 * @param	pACMP	: Pointer to Analog Comparator block
 * @param	index	: index to the comparator (0 - 3)
 * @return	Nothing
 */
STATIC INLINE void Chip_ACMP_EnableVoltLadder(LPC_CMP_T *pACMP, uint8_t index)
{
	/* Make sure interrupt flag is not set during read write operation */
	pACMP->ACMP[index].CMP = (pACMP->ACMP[index].CMP & ~ACMP_INTFLAG_BIT) | ACMP_LADENAB_BIT;
}

/**
 * @brief	Disables voltage ladder
 * @param	pACMP	: Pointer to Analog Comparator block
 * @param	index	: index to the comparator (0 - 3)
 * @return	Nothing
 */
STATIC INLINE void Chip_ACMP_DisableVoltLadder(LPC_CMP_T *pACMP, uint8_t index)
{
	/* Make sure interrupt flag is not set during read write operation */
	pACMP->ACMP[index].CMP = (pACMP->ACMP[index].CMP & ~ACMP_INTFLAG_BIT) & ~ACMP_LADENAB_BIT;
}

/**
 * @brief	Set propogation delay for comparator output
 * @param	pACMP	: Pointer to Analog Comparator block
 * @param	index	: index to the comparator (0 - 3)
 * @param	delay	: propogation delay (0 - 2), 0 is short delay more power consumption
 * @return	Nothing
 */
STATIC INLINE void Chip_ACMP_SetPropagationDelay(LPC_CMP_T *pACMP, uint8_t index, uint8_t delay)
{
	/* Make sure interrupt flag is not set during read write operation */
	pACMP->ACMP[index].CMP =
		((pACMP->ACMP[index].CMP & ~ACMP_INTFLAG_BIT) & ~ACMP_PROPDLY_MASK) | ((uint32_t) delay << 29);
}

/**
 * @brief	Set filter sample mode
 * @param	pACMP	: Pointer to Analog Comparator block
 * @param	index	: index to the comparator (0 - 3)
 * @param	mode	: sample mode enum value
 * @return	Nothing
 */
STATIC INLINE void Chip_ACMP_SetSampleMode(LPC_CMP_T *pACMP, uint8_t index, CHIP_ACMP_SMODE_T mode)
{
	pACMP->ACMP[index].CMPFILTR = (pACMP->ACMP[index].CMPFILTR & ~ACMP_SMODE_MASK) | (uint32_t) mode;
}

/**
 * @brief	Set clock divider
 * @param	pACMP	: Pointer to Analog Comparator block
 * @param	index	: index to the comparator (0 - 3)
 * @param	div		: SysClk divider enum value
 * @return	Nothing
 */
STATIC INLINE void Chip_ACMP_SetClockDiv(LPC_CMP_T *pACMP, uint8_t index, CHIP_ACMP_CLKDIV_T div)
{
	pACMP->ACMP[index].CMPFILTR = (pACMP->ACMP[index].CMPFILTR & ~ACMP_CLKDIV_MASK) | (uint32_t) div;
}

/**
 * @brief	Setup Comparator filter register
 * @param	pACMP	: Pointer to Analog Comparator block
 * @param	index	: index to the comparator (0 - 3)
 * @param	mode	: sample mode enum value
 * @param	div		: SysClk divider enum value
 * @return	Nothing
 */
STATIC INLINE void Chip_ACMP_SetCompFiltReg(LPC_CMP_T *pACMP,
											uint8_t index,
											CHIP_ACMP_SMODE_T mode,
											CHIP_ACMP_CLKDIV_T div)
{
	pACMP->ACMP[index].CMPFILTR = (uint32_t) mode | (uint32_t) div;
}

/**
 * @brief	Set Ring Oscillator control bit, ROSC output is set by ACMP1 and reset by ACMP0
 * @param	pACMP	: Pointer to Analog Comparator block
 * @return	Nothing
 */
STATIC INLINE void Chip_ACMP_SetRingOscCtl(LPC_CMP_T *pACMP)
{
	pACMP->CTRL |= ACMP_ROSCCTL_BIT;
}

/**
 * @brief	Clear Ring Oscillator control bit, ROSC output is set by ACMP0 and reset by ACMP1
 * @param	pACMP	: Pointer to Analog Comparator block
 * @return	Nothing
 */
STATIC INLINE void Chip_ACMP_ClearRingOscCtl(LPC_CMP_T *pACMP)
{
	pACMP->CTRL &= ~ACMP_ROSCCTL_BIT;
}

/**
 * @brief	Set Ring Oscillator Reset Source to Internal
 * @param	pACMP	: Pointer to Analog Comparator block
 * @return	Nothing
 */
STATIC INLINE void Chip_ACMP_SetROscResetSrcInternal(LPC_CMP_T *pACMP)
{
	pACMP->CTRL &= ~ACMP_EXTRESET_BIT;
}

/**
 * @brief	Set Ring Oscillator Reset Source to External
 * @param	pACMP	: Pointer to Analog Comparator block
 * @return	Nothing
 */
STATIC INLINE void Chip_ACMP_SetROscResetSrcExternal(LPC_CMP_T *pACMP)
{
	pACMP->CTRL |= ACMP_EXTRESET_BIT;
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __ACMP_15XX_H_ */
