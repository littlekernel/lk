/*
 * @brief LPC15xx ROM ADC API declarations and functions
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

#ifndef __ROM_ADC_15XX_H_
#define __ROM_ADC_15XX_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup ADCROM_15XX CHIP: LPC15xx ADC ROM API declarations and functions
 * @ingroup ROMAPI_15XX
 * @{
 */

/**
 * @brief ADC handle type
 */
typedef void *ADC_HANDLE_T;

typedef void (*ADC_SEQ_CALLBK_T)(ADC_HANDLE_T handle);
typedef void (*ADC_CALLBK_T)(ErrorCode_t error_code, uint32_t num_channel);

/* Typedef for structure pointed by the ADC_HANDLE */
typedef struct   {	/* block of RAM allocated by the application program */
	uint32_t          base_addr;	/* adcr register base address */
	uint32_t          *seqa_buffer;	/* adc buffer */
	uint32_t          *seqb_buffer;	/* adc buffer */
	uint32_t          seqa_channel_num;	/* number of ADC channels */
	uint32_t          seqb_channel_num;	/* number of ADC channels */
	uint32_t          seqa_hwtrig;
	uint32_t          seqb_hwtrig;
	uint32_t          comp_flags;
	uint32_t          overrun_flags;
	uint32_t          thcmp_flags;
	uint32_t          error_code;	/* error code */
	ADC_SEQ_CALLBK_T  seqa_callback;	/* For interrupt, it's the end of the sequence A */
	ADC_SEQ_CALLBK_T  seqb_callback;	/* For interrupt, it's the end of the sequence B */
	ADC_CALLBK_T      overrun_callback;	/* For interrupt, it's the overrun */
	ADC_CALLBK_T      thcmp_callback;	/* For interrupt, it's over the threshold */
	uint32_t          error_en;	/* enable bits for error detection */
	uint32_t          thcmp_en;	/* enable bits for thcmp detection */
} ADC_DRIVER_T;	/* HEADER_TypeDef	 *********************************/

typedef struct {
	uint32_t system_clock;	/* System clock */
	uint32_t adc_clock;	/* ADC clock */
	uint32_t async_mode;
	uint32_t tenbit_mode;
	uint32_t lpwr_mode;
	uint32_t input_sel;
	uint32_t seqa_ctrl;
	uint32_t seqb_ctrl;
	uint32_t thrsel;
	uint32_t thr0_low;
	uint32_t thr0_high;
	uint32_t thr1_low;
	uint32_t thr1_high;
	uint32_t error_en;
	uint32_t thcmp_en;
	uint32_t channel_num;
} ADC_CONFIG_T;

typedef struct {
	uint32_t dma_adc_num;	/* DMA channel used for ADC data peripheral to memory transfer */
	uint32_t dma_pinmux_num; /* H/W trigger number. */
	uint32_t dma_handle; /* DMA handle passed to ADC */
	ADC_CALLBK_T dma_done_callback_pt;	/* DMA completion callback function */
} ADC_DMA_CFG_T;

typedef ErrorCode_t (*ADC_DMA_SETUP_T)(ADC_HANDLE_T handle, ADC_DMA_CFG_T *dma_cfg);

typedef struct {		/* params passed to adc driver function */
	uint32_t          *buffer;		/* Considering supporting DMA and non-DMA mode, 32-bit buffer is needed for DMA */
	uint32_t          driver_mode;	/* 0x00: Polling mode, function is blocked until transfer is finished. */
									/* 0x01: Interrupt mode, function exit immediately, callback function is invoked when transfer is finished. */
									/* 0x02: DMA mode, in case DMA block is available, data transferred by ADC is processed by DMA, 
									         and max buffer size is the total number ADC channels, DMA req function is called for ADC DMA
									         channel setup, then SEQx completion also used as DMA callback function when that ADC conversion/DMA transfer
									         is finished. */
	uint32_t          seqa_hwtrig;	/* H/W trigger for sequence A */
	uint32_t          seqb_hwtrig;	/* H/W trigger for sequence B */
	ADC_CONFIG_T      *adc_cfg;
	uint32_t          comp_flags;
	uint32_t          overrun_flags;
	uint32_t          thcmp_flags;
	ADC_DMA_CFG_T     *dma_cfg;
	ADC_SEQ_CALLBK_T  seqa_callback_pt;		/* SEQA callback function/the same callback on DMA completion if DMA is used for ADCx. */
	ADC_SEQ_CALLBK_T  seqb_callback_pt;		/* SEQb callback function/the same callback on DMA completion if DMA is used for ADCx. */
	ADC_CALLBK_T      overrun_callback_pt;	/* Overrun callback function */
	ADC_CALLBK_T      thcmp_callback_pt;	/* THCMP callback function */
	ADC_DMA_SETUP_T   dma_setup_func_pt;	/* ADC DMA channel setup function */
} ADC_PARAM_T;

/* Typedef Structure for ROM API's */
typedef struct ADCD_API {
	/* ADC Configuration functions */
	uint32_t (*adc_get_mem_size)(void);
	ADC_HANDLE_T (*adc_setup)(uint32_t base_addr, uint8_t *ram);
	void (*adc_calibration)(ADC_HANDLE_T handle, ADC_CONFIG_T *set);
	void (*adc_init)(ADC_HANDLE_T handle, ADC_CONFIG_T *set);
	
	/* ADC Conversion Functions */
	uint32_t (*adc_seqa_read)(ADC_HANDLE_T handle, ADC_PARAM_T *param);
	uint32_t (*adc_seqb_read)(ADC_HANDLE_T handle, ADC_PARAM_T *param);
	
	/* ADC Interrupt handlers */
	void (*adc_seqa_isr)(ADC_HANDLE_T handle);
	void (*adc_seqb_isr)(ADC_HANDLE_T handle);
	void (*adc_ovr_isr)(ADC_HANDLE_T handle);
	void (*adc_thcmp_isr)(ADC_HANDLE_T handle);
	
	uint32_t  (*adc_get_firmware_version)(void);
} ADCD_API_T;

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __ROM_ADC_15XX_H_ */
