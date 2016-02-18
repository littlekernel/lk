/*
 * @brief LPC15xx Input Mux Registers and Driver
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

#ifndef __INMUX_15XX_H_
#define __INMUX_15XX_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup INMUX_15XX CHIP: LPC15xx Input Mux Registers and Driver
 * @ingroup CHIP_15XX_Drivers
 * @{
 */

/**
 * @brief LPC15xx Input Mux Register Block Structure
 */
typedef struct {						/*!< INMUX Structure */
	__IO uint32_t SCT0_INMUX[7];		/*!< Input mux registers for SCT0 inputs */
	__I  uint32_t  RESERVED1[1];
	__IO uint32_t SCT1_INMUX[7];		/*!< Input mux registers for SCT1 inputs */
	__I  uint32_t  RESERVED2[1];
	__IO uint32_t SCT2_INMUX[3];		/*!< Input mux registers for SCT2 inputs */
	__I  uint32_t  RESERVED3[5];
	__IO uint32_t SCT3_INMUX[3];		/*!< Input mux registers for SCT3 inputs */
	__I  uint32_t  RESERVED4[5];
	__I  uint32_t  RESERVED4A[16];
	__IO uint32_t PINTSEL[8];			/*!< Pin interrupt select registers */
	__IO uint32_t DMA_ITRIG_INMUX[18];	/*!< Input mux register for DMA trigger inputs */
	__I  uint32_t  RESERVED5[6];
	__IO uint32_t DMA_INMUX[4];			/*!< Input mux register for DMA trigger inputs */
	__I  uint32_t  RESERVED6[4];
	__IO uint32_t FREQMEAS_REF;			/*!< Clock selection for frequency measurement ref clock */
	__IO uint32_t FREQMEAS_TARGET;		/*!< Clock selection for frequency measurement target clock */
} LPC_INMUX_T;

/* SCT input mux mapping selections for SCT0 inputs 0-6 */
typedef enum {
	SCT0_INMUX_PIO0_2 = 0,
	SCT0_INMUX_PIO0_3,
	SCT0_INMUX_PIO0_17,
	SCT0_INMUX_PIO0_30,
	SCT0_INMUX_PIO1_6,
	SCT0_INMUX_PIO1_7,
	SCT0_INMUX_PIO1_12,
	SCT0_INMUX_PIO1_13,
	SCT0_INMUX_SCT1_OUT4,
	SCT0_INMUX_SCT2_OUT4,
	SCT0_INMUX_SCT2_OUT5,
	SCT0_INMUX_ADC0_THCMP_IRQ,
	SCT0_INMUX_ADC1_THCMP_IRQ,
	SCT0_INMUX_ACMP0_OUT,
	SCT0_INMUX_ACMP1_OUT,
	SCT0_INMUX_ACMP2_OUT,
	SCT0_INMUX_ACMP3_OUT,
	SCT0_INMUX_SCTIPU_ABORT,
	SCT0_INMUX_SCTIPU_SAMPLE0,
	SCT0_INMUX_SCTIPU_SAMPLE1,
	SCT0_INMUX_SCTIPU_SAMPLE2,
	SCT0_INMUX_SCTIPU_SAMPLE3,
	SCT0_INMUX_DEBUG_HALTED
} SCT0_INMUX_T;

/**
 * @brief	Selects an input source for SCT0 input 0 to 6
 * @param	input	: SCT0 input to use, 0 - 6
 * @param	src		: Source to map to the SCT input
 * @return	Nothing
 */
STATIC INLINE void Chip_INMUX_SelectSCT0Src(uint8_t input, SCT0_INMUX_T src)
{
	LPC_INMUX->SCT0_INMUX[input] = (uint32_t) src;
}

/* SCT input mux mapping selections for SCT1 inputs 0-6 */
typedef enum {
	SCT1_INMUX_PIO0_15 = 0,
	SCT1_INMUX_PIO0_16,
	SCT1_INMUX_PIO0_21,
	SCT1_INMUX_PIO0_31,
	SCT1_INMUX_PIO1_4,
	SCT1_INMUX_PIO1_5,
	SCT1_INMUX_PIO1_15,
	SCT1_INMUX_PIO1_16,
	SCT1_INMUX_SCT0_OUT4,
	SCT1_INMUX_SCT3_OUT4,
	SCT1_INMUX_SCT3_OUT5,
	SCT1_INMUX_ADC0_THCMP_IRQ,
	SCT1_INMUX_ADC1_THCMP_IRQ,
	SCT1_INMUX_ACMP0_OUT,
	SCT1_INMUX_ACMP1_OUT,
	SCT1_INMUX_ACMP2_OUT,
	SCT1_INMUX_ACMP3_OUT,
	SCT1_INMUX_SCTIPU_ABORT,
	SCT1_INMUX_SCTIPU_SAMPLE0,
	SCT1_INMUX_SCTIPU_SAMPLE1,
	SCT1_INMUX_SCTIPU_SAMPLE2,
	SCT1_INMUX_SCTIPU_SAMPLE3,
	SCT1_INMUX_DEBUG_HALTED
} SCT1_INMUX_T;

/**
 * @brief	Selects an input source for SCT1 input 0 to 6
 * @param	input	: SCT1 input to use, 0 - 6
 * @param	src		: Source to map to the SCT input
 * @return	Nothing
 */
STATIC INLINE void Chip_INMUX_SelectSCT1Src(uint8_t input, SCT1_INMUX_T src)
{
	LPC_INMUX->SCT1_INMUX[input] = (uint32_t) src;
}

/* SCT input mux mapping selections for SCT2 inputs 0-2 */
typedef enum {
	SCT2_INMUX_PIO0_4 = 0,
	SCT2_INMUX_PIO0_27,
	SCT2_INMUX_PIO1_18,
	SCT2_INMUX_PIO1_19,
	SCT2_INMUX_SCT0_OUT4,
	SCT2_INMUX_SCT0_OUT5,
	SCT2_INMUX_SCT0_OUT7,
	SCT2_INMUX_SCT0_OUT8,
	SCT2_INMUX_ADC0_THCMP_IRQ,
	SCT2_INMUX_ADC1_THCMP_IRQ,
	SCT2_INMUX_ACMP0_OUT,
	SCT2_INMUX_ACMP1_OUT,
	SCT2_INMUX_ACMP2_OUT,
	SCT2_INMUX_ACMP3_OUT,
	SCT2_INMUX_SCTIPU_ABORT,
	SCT2_INMUX_SCTIPU_SAMPLE0,
	SCT2_INMUX_SCTIPU_SAMPLE1,
	SCT2_INMUX_SCTIPU_SAMPLE2,
	SCT2_INMUX_SCTIPU_SAMPLE3,
	SCT2_INMUX_USB_FRAME_TOGGLE,
	SCT2_INMUX_DEBUG_HALTED
} SCT2_INMUX_T;

/**
 * @brief	Selects an input source for SCT2 input 0 to 2
 * @param	input	: SCT2 input to use, 0 - 2
 * @param	src		: Source to map to the SCT input
 * @return	Nothing
 */
STATIC INLINE void Chip_INMUX_SelectSCT2Src(uint8_t input, SCT2_INMUX_T src)
{
	LPC_INMUX->SCT2_INMUX[input] = (uint32_t) src;
}

/* SCT input mux mapping selections for SCT3 inputs 0-2 */
typedef enum {
	SCT3_INMUX_PIO0_7 = 0,
	SCT3_INMUX_PIO1_11,
	SCT3_INMUX_PIO1_21,
	SCT3_INMUX_PIO1_22,
	SCT3_INMUX_SCT1_OUT4,
	SCT3_INMUX_SCT1_OUT5,
	SCT3_INMUX_SCT1_OUT7,
	SCT3_INMUX_SCT1_OUT8,
	SCT3_INMUX_ADC0_THCMP_IRQ,
	SCT3_INMUX_ADC1_THCMP_IRQ,
	SCT3_INMUX_ACMP0_OUT,
	SCT3_INMUX_ACMP1_OUT,
	SCT3_INMUX_ACMP2_OUT,
	SCT3_INMUX_ACMP3_OUT,
	SCT3_INMUX_SCTIPU_ABORT3,
	SCT3_INMUX_SCTIPU_SAMPLE0,
	SCT3_INMUX_SCTIPU_SAMPLE1,
	SCT3_INMUX_SCTIPU_SAMPLE2,
	SCT3_INMUX_SCTIPU_SAMPLE3,
	SCT3_INMUX_USB_FRAME_TOGGLE,
	SCT3_INMUX_DEBUG_HALTED
} SCT3_INMUX_T;

/**
 * @brief	Selects an input source for SCT3 input 0 to 2
 * @param	input	: SCT3 input to use, 0 - 2
 * @param	src		: Source to map to the SCT input
 * @return	Nothing
 */
STATIC INLINE void Chip_INMUX_SelectSCT3Src(uint8_t input, SCT3_INMUX_T src)
{
	LPC_INMUX->SCT3_INMUX[input] = (uint32_t) src;
}

/**
 * @brief	GPIO Pin Interrupt Pin Select (sets PINTSEL register)
 * @param	pintSel	: GPIO PINTSEL interrupt, should be: 0 to 7
 * @param	portNum	: GPIO port number interrupt, should be: 0 to 1
 * @param	pinNum	: GPIO pin number Interrupt, should be: 0 to 31
 * @return	Nothing
 */
STATIC INLINE void Chip_INMUX_PinIntSel(uint8_t pintSel, uint8_t portNum, uint8_t pinNum)
{
	LPC_INMUX->PINTSEL[pintSel] = (portNum * 32) + pinNum;
}

/* DMA triggers that can mapped to DMA channels */
typedef enum {
	DMATRIG_ADC0_SEQA_IRQ = 0,			/*!< ADC0 sequencer A interrupt as trigger */
	DMATRIG_ADC0_SEQB_IRQ,				/*!< ADC0 sequencer B interrupt as trigger */
	DMATRIG_ADC1_SEQA_IRQ,				/*!< ADC1 sequencer A interrupt as trigger */
	DMATRIG_ADC1_SEQB_IRQ,				/*!< ADC1 sequencer B interrupt as trigger */
	DMATRIG_SCT0_DMA0,					/*!< SCT 0, DMA 0 as trigger */
	DMATRIG_SCT0_DMA1,					/*!< SCT 1, DMA 1 as trigger */
	DMATRIG_SCT1_DMA0,					/*!< SCT 0, DMA 0 as trigger */
	DMATRIG_SCT1_DMA1,					/*!< SCT 1, DMA 1 as trigger */
	DMATRIG_SCT2_DMA0,					/*!< SCT 2, DMA 0 as trigger */
	DMATRIG_SCT2_DMA1,					/*!< SCT 2, DMA 1 as trigger */
	DMATRIG_SCT3_DMA0,					/*!< SCT 3, DMA 0 as trigger */
	DMATRIG_SCT3_DMA1,					/*!< SCT 3, DMA 1 as trigger */
	DMATRIG_ACMP0_OUT,					/*!< Analog comparator 0 output as trigger */
	DMATRIG_ACMP1_OUT,					/*!< Analog comparator 1 output as trigger */
	DMATRIG_ACMP2_OUT,					/*!< Analog comparator 2 output as trigger */
	DMATRIG_ACMP3_OUT,					/*!< Analog comparator 3 output as trigger */
	DMATRIG_OUTMUX0,					/*!< DMA trigger tied to this source, Select with Chip_INMUX_SetDMAOutMux */
	DMATRIG_OUTMUX1,					/*!< DMA trigger tied to this source, Select with Chip_INMUX_SetDMAOutMux */
	DMATRIG_OUTMUX2,					/*!< DMA trigger tied to this source, Select with Chip_INMUX_SetDMAOutMux */
	DMATRIG_OUTMUX3						/*!< DMA trigger tied to this source, Select with Chip_INMUX_SetDMAOutMux */
} DMA_TRIGSRC_T;

/**
 * @brief	Select a trigger source for a DMA channel
 * @param	ch		: DMA channel number
 * @param	trig	: Trigger source for the DMA channel
 * @return	Nothing
 */
STATIC INLINE void Chip_INMUX_SetDMATrigger(uint8_t ch, DMA_TRIGSRC_T trig)
{
	LPC_INMUX->DMA_ITRIG_INMUX[ch] = (uint32_t) trig;
}

/**
 * @brief	Selects a DMA trigger source for the DMATRIG_OUTMUXn IDs
 * @param	index	: Select 0 to 3 to sets the source for DMATRIG_OUTMUX0 to DMATRIG_OUTMUX3
 * @param	dmaCh	: DMA channel to select for DMATRIG_OUTMUXn source
 * @return	Nothing
 * @note	This function sets the DMA trigger (out) source used with the DMATRIG_OUTMUXn
 *			trigger source.
 */
STATIC INLINE void Chip_INMUX_SetDMAOutMux(uint8_t index, uint8_t dmaCh)
{
	LPC_INMUX->DMA_INMUX[index] = (uint32_t) dmaCh;
}

/* Freqeuency mearure reference and target clock sources */
typedef enum {
	FREQMSR_MAIN_OSC = 0,			/*!< System oscillator */
	FREQMSR_IRC,					/*!< Internal RC (IRC) oscillator */
	FREQMSR_WDOSC,					/*!< Watchdog oscillator */
	FREQMSR_32KHZOSC,				/*!< 32KHz (RTC) oscillator rate */
	FREQMSR_USB_FTOGGLE,			/*!< USB FTOGGLE rate */
	FREQMSR_PIO0_5,					/*!< External pin PIO0_5 as input rate */
	FREQMSR_PIO0_19,				/*!< External pin PIO0_19 as input rate */
	FREQMSR_PIO0_30,				/*!< External pin PIO0_30 as input rate */
	FREQMSR_PIO1_27					/*!< External pin PIO1_27 as input rate */
} FREQMSR_SRC_T;

/**
 * @brief	Selects a reference clock used with the frequency measure function
 * @param	ref	: Frequency measure function reference clock
 * @return	Nothing
 */
STATIC INLINE void Chip_INMUX_SetFreqMeasRefClock(FREQMSR_SRC_T ref)
{
	LPC_INMUX->FREQMEAS_REF = (uint32_t) ref;
}

/**
 * @brief	Selects a target clock used with the frequency measure function
 * @param	targ	: Frequency measure function reference clock
 * @return	Nothing
 */
STATIC INLINE void Chip_INMUX_SetFreqMeasTargClock(FREQMSR_SRC_T targ)
{
	LPC_INMUX->FREQMEAS_TARGET = (uint32_t) targ;
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __INMUX_15XX_H_ */
