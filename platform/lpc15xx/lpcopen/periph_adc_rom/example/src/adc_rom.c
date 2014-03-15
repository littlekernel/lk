/*
 * @brief LPC15xx ADC example
 *
 * @note
 * Copyright(C) NXP Semiconductors, 2014
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

#define TICKRATE_HZ (100)	/* 100 SysTicks per second */

#if defined(BOARD_NXP_LPCXPRESSO_1549)
/* ADC is connected to the pot on LPCXPresso base boards */
#define BOARD_ADC_CH 1
/* ADC Clock set to 36MHz for this board as SysClk is 72MHz and max ADC clock is 50MHz*/
#define ADC_CLOCK_RATE (36000000)

#else
#warning "Using ADC channel 8 for this example, please select for your board. Also setup ADC Clock"
#define BOARD_ADC_CH 8
#endif

/* RAM Block Sizes for ADC ROM API */
#define RAMBLOCK_H 60
#define BUFFER_SIZE 12

static volatile int ticks;
static bool sequence0Complete, sequence1Complete, threshold1Crossed;

static uint32_t                 size_in_bytes;
static const ADCD_API_T *pAdcApi;	/* define pointer to type API function addr table */
static ADC_HANDLE_T         adc_handle[2];	/* handle to ADC API */
static ADC_PARAM_T          param[2];	/* Param Structure for ADC ROM API */
static ADC_CONFIG_T         adc_cfg[2];	/* Config Structure for ADC ROM API */
static uint32_t                 start_of_ram_block[2][RAMBLOCK_H];		/* RAM Block */
static uint32_t                 adc_buffer[2][BUFFER_SIZE];	/* Buffer for ADC data */
static ErrorCode_t          err_code;

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Private functions
 ****************************************************************************/

void showValudeADC(LPC_ADC_T *pADC)
{
	uint32_t rawSample;

	if (pADC == LPC_ADC0) {
		rawSample = adc_buffer[0][0];
		DEBUGOUT("ADC%d_%d: Sample value = 0x%x (Data sample %d)\r\n", 0, 0, rawSample, 0);
	}
	else {
		rawSample = adc_buffer[1][BOARD_ADC_CH];
		DEBUGOUT("ADC%d_%d: Sample value = 0x%x (Data sample %d)\r\n", 1, BOARD_ADC_CH, rawSample, BOARD_ADC_CH);
	}
}

void  adc_seq0_callback(ADC_HANDLE_T handle)
{
	ADC_DRIVER_T *driver = (ADC_DRIVER_T *) handle;

	if ((ErrorCode_t) driver->error_code == LPC_OK) {
		if (driver->comp_flags & 0x01) {
			sequence0Complete = true;
		}
	}
}

void  adc_seq1_callback(ADC_HANDLE_T handle)
{
	ADC_DRIVER_T *driver = (ADC_DRIVER_T *) handle;

	if ((ErrorCode_t) driver->error_code == LPC_OK) {
		if (driver->comp_flags & (1 << BOARD_ADC_CH)) {
			sequence1Complete = true;
		}
	}
}

void adc_thcmp_callback(ErrorCode_t error_code, uint32_t num_channel)
{
	if (error_code == LPC_OK) {
		if (num_channel & (1 << BOARD_ADC_CH)) {
			threshold1Crossed = true;
		}
	}
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
	static uint32_t count;

	/* Every 1/2 second */
	count++;
	if (count >= (TICKRATE_HZ / 2)) {
		count = 0;
		/* ADC0 Param for SEQA read */
		param[0].buffer = (uint32_t *) adc_buffer[0];
		param[0].driver_mode = 0x01;/* Interrupt Mode */
		param[0].seqa_hwtrig = 0;
		param[0].adc_cfg = &adc_cfg[0];	/* ADC0 Config */
		param[0].comp_flags = 0;
		param[0].seqa_callback_pt = adc_seq0_callback;	/* Call back for SeqA Interrupt */
		/* ADC1 Param for SEQA read */
		param[1].buffer = (uint32_t *) adc_buffer[1];
		param[1].driver_mode = 0x01;/* Interrupt Mode */
		param[1].seqa_hwtrig = 0;
		param[1].adc_cfg = &adc_cfg[1];	/* ADC1 Config */
		param[1].comp_flags = 0;
		param[1].seqa_callback_pt = adc_seq1_callback;	/* Call back for SeqA Interrupt */
		param[1].thcmp_callback_pt = adc_thcmp_callback;/* Call back for Threshold Compare Interrupt */
		err_code = LPC_OK;
		err_code |= (ErrorCode_t) pAdcApi->adc_seqa_read(adc_handle[0], &param[0]);
		err_code |= (ErrorCode_t) pAdcApi->adc_seqa_read(adc_handle[1], &param[1]);
		if (err_code != LPC_OK) {
			Board_LED_Toggle(0);
		}
	}
}

/**
 * @brief	Handle interrupt from ADC0 sequencer A
 * @return	Nothing
 */
void ADC0A_IRQHandler(void)
{
	pAdcApi->adc_seqa_isr(adc_handle[0]);
}

/**
 * @brief	Handle interrupt from ADC1 sequencer A
 * @return	Nothing
 */
void ADC1A_IRQHandler(void)
{
	pAdcApi->adc_seqa_isr(adc_handle[1]);
}

/**
 * @brief	Handle threshold interrupt from ADC1
 * @return	Nothing
 */
void ADC1_THCMP_IRQHandler(void)
{
	pAdcApi->adc_thcmp_isr(adc_handle[1]);
}

/**
 * @brief	main routine for ADC example
 * @return	Function should not exit
 */
int main(void)
{
	SystemCoreClockUpdate();
	Board_Init();
	DEBUGSTR("ADC ROM sequencer demo\r\n");

	/* Power up, enable clock and reset ADC0 */
	Chip_SYSCTL_PowerUp(SYSCTL_POWERDOWN_ADC0_PD);
	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_ADC0);
	Chip_SYSCTL_PeriphReset(RESET_ADC0);
	/* Power up, enable clock and reset ADC1 */
	Chip_SYSCTL_PowerUp(SYSCTL_POWERDOWN_ADC1_PD);
	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_ADC1);
	Chip_SYSCTL_PeriphReset(RESET_ADC1);
	/* Power up the internal temperature sensor */
	Chip_SYSCTL_PowerUp(SYSCTL_POWERDOWN_TS_PD);

#if defined(BOARD_NXP_LPCXPRESSO_1549)
	/* Disables pullups/pulldowns and disable digital mode */
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 9, (IOCON_MODE_INACT | IOCON_ADMODE_EN));

	/* Assign ADC1_1 to PIO0_9 via SWM (fixed pin) */
	Chip_SWM_EnableFixedPin(SWM_FIXED_ADC1_1);

#else
#warning "No ADC setup for this example"
#endif
	/* Initialize ROM API base address for ADC */
	pAdcApi = LPC_ADCD_API;
	size_in_bytes = pAdcApi->adc_get_mem_size();
	if (size_in_bytes / 4 > RAMBLOCK_H) {
		/* Adjust RAMBLOCK size in this case */
		return 1;
	}
	/* ADC Handle Setup*/
	adc_handle[0] = pAdcApi->adc_setup(LPC_ADC0_BASE, (uint8_t *) start_of_ram_block[0]);
	adc_handle[1] = pAdcApi->adc_setup(LPC_ADC1_BASE, (uint8_t *) start_of_ram_block[1]);
	/* ADC0 Config */
	adc_cfg[0].system_clock = SystemCoreClock;	/* System clock */
	adc_cfg[0].adc_clock = 500000;	/* ADC clock set to 500KHz for calibration*/
	/* ADC1 Config */
	adc_cfg[1].system_clock = SystemCoreClock;	/* System clock */
	adc_cfg[1].adc_clock = 500000;	/* ADC clock set to 500KHz for calibration*/
	pAdcApi->adc_calibration(adc_handle[0], &adc_cfg[0]);
	pAdcApi->adc_calibration(adc_handle[1], &adc_cfg[1]);

	/* ADC0 Config for Init */
	adc_cfg[0].system_clock = SystemCoreClock;	/* System clock */
	adc_cfg[0].adc_clock = ADC_CLOCK_RATE;	/* ADC clock */
	adc_cfg[0].async_mode = 0;	/* Synchronous mode */
	adc_cfg[0].tenbit_mode = 0;	/* 12 Bit ADC mode */
	adc_cfg[0].lpwr_mode = 0;	/* Disable low power mode */
	adc_cfg[0].input_sel = ADC_INSEL_TS;
	adc_cfg[0].seqa_ctrl = (ADC_SEQ_CTRL_CHANSEL(0) | ADC_SEQ_CTRL_MODE_EOS);
	adc_cfg[0].channel_num = 1;	/* Channel number is one higher than the maximum channel number used */
	/* ADC1 Config for Init */
	adc_cfg[1].system_clock = SystemCoreClock;	/* System clock */
	adc_cfg[1].adc_clock = ADC_CLOCK_RATE;	/* ADC clock */
	adc_cfg[1].async_mode = 0;	/* Synchronous mode */
	adc_cfg[1].tenbit_mode = 0;	/* 12 Bit ADC mode */
	adc_cfg[1].lpwr_mode = 0;	/* Disable low power mode */
	adc_cfg[1].seqa_ctrl = (ADC_SEQ_CTRL_CHANSEL(BOARD_ADC_CH) | ADC_SEQ_CTRL_MODE_EOS);
	adc_cfg[1].thrsel = 0;
	adc_cfg[1].thr0_low = ((1 * 0xFFF) / 4) << 4;
	adc_cfg[1].thr0_high = ((3 * 0xFFF) / 4) << 4;
	adc_cfg[1].thcmp_en = ADC_INTEN_CMP_ENABLE(ADC_INTEN_CMP_CROSSTH, BOARD_ADC_CH);
	adc_cfg[1].channel_num = BOARD_ADC_CH + 1;	/* Channel number is one higher than the maximum channel number used */
	pAdcApi->adc_init(adc_handle[0], &adc_cfg[0]);
	pAdcApi->adc_init(adc_handle[1], &adc_cfg[1]);
	
	/* When using ADC ROM API's lower the priority of ADC Sequence completion interrupt when compared to the threshold interrupt*/
	NVIC_SetPriority(ADC1_SEQA_IRQn, 1);
	/* Enable related ADC NVIC interrupts */
	NVIC_EnableIRQ(ADC0_SEQA_IRQn);
	NVIC_EnableIRQ(ADC1_SEQA_IRQn);
	NVIC_EnableIRQ(ADC1_THCMP);

	/* This example uses the periodic sysTick to manually trigger the ADC,
	   but a periodic timer can be used in a match configuration to start
	   an ADC sequence without software intervention. */
	SysTick_Config(Chip_Clock_GetSysTickClockRate() / TICKRATE_HZ);

	/* Endless loop */
	while (1) {
		/* Sleep until something happens */
		__WFI();

		if (threshold1Crossed) {
			threshold1Crossed = false;
			DEBUGSTR("********ADC1 threshold event********\r\n");
		}

		/* Is a conversion sequence complete? */
		if (sequence0Complete) {
			sequence0Complete = false;
			showValudeADC(LPC_ADC0);
		}
		if (sequence1Complete) {
			sequence1Complete = false;
			showValudeADC(LPC_ADC1);
		}
	}

	/* Should not run to here */
	return 0;
}
