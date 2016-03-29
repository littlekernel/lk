/*
 * @brief LPC15xx DMA ROM API declarations and functions
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

#ifndef __ROM_DMA_15XX_H_
#define __ROM_DMA_15XX_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup DMAROM_15XX CHIP: LPC15xx DMA ROM API declarations and functions
 * @ingroup ROMAPI_15XX
 * @{
 */

/* Bit definitions for DMA ROM Channel Configuration Structure */
#define DMA_ROM_CH_EVENT_SWTRIG                         ((uint8_t) 0)
#define DMA_ROM_CH_EVENT_PERIPH                         ((uint8_t) 1)
#define DMA_ROM_CH_EVENT_HWTRIG                         ((uint8_t) 2)
#define DMA_ROM_CH_HWTRIG_BURSTPOWER_1      ((uint8_t) 0 << 0)
#define DMA_ROM_CH_HWTRIG_BURSTPOWER_2      ((uint8_t) 1 << 0)
#define DMA_ROM_CH_HWTRIG_BURSTPOWER_4      ((uint8_t) 2 << 0)
#define DMA_ROM_CH_HWTRIG_BURSTPOWER_8      ((uint8_t) 3 << 0)
#define DMA_ROM_CH_HWTRIG_BURSTPOWER_16     ((uint8_t) 4 << 0)
#define DMA_ROM_CH_HWTRIG_BURSTPOWER_32     ((uint8_t) 5 << 0)
#define DMA_ROM_CH_HWTRIG_BURSTPOWER_64     ((uint8_t) 6 << 0)
#define DMA_ROM_CH_HWTRIG_BURSTPOWER_128    ((uint8_t) 7 << 0)
#define DMA_ROM_CH_HWTRIG_BURSTPOWER_256    ((uint8_t) 8 << 0)
#define DMA_ROM_CH_HWTRIG_BURSTPOWER_512    ((uint8_t) 9 << 0)
#define DMA_ROM_CH_HWTRIG_BURSTPOWER_1024   ((uint8_t) 10 << 0)
#define DMA_ROM_CH_HWTRIG_SRC_WRAP_EN               ((uint8_t) 1 << 4)
#define DMA_ROM_CH_HWTRIG_DEST_WRAP_EN          ((uint8_t) 1 << 5)
#define DMA_ROM_CH_HWTRIG_BURST_EN                  ((uint8_t) 1 << 6)
/* Bit definitions for DMA ROM Task Configuration Structure */
#define DMA_ROM_TASK_CFG_PING_PONG_EN               ((uint8_t) 1 << 0)
#define DMA_ROM_TASK_CFG_SW_TRIGGER                 ((uint8_t) 1 << 1)
#define DMA_ROM_TASK_CFG_CLR_TRIGGER                ((uint8_t) 1 << 2)
#define DMA_ROM_TASK_CFG_SEL_INTA                       ((uint8_t) 1 << 3)
#define DMA_ROM_TASK_CFG_SEL_INTB                       ((uint8_t) 1 << 4)
#define DMA_ROM_TASK_DATA_WIDTH_8                       ((uint8_t) 0 )
#define DMA_ROM_TASK_DATA_WIDTH_16                  ((uint8_t) 1 )
#define DMA_ROM_TASK_DATA_WIDTH_32                  ((uint8_t) 2 )
#define DMA_ROM_TASK_SRC_INC_0                          ((uint8_t) 0 << 2)
#define DMA_ROM_TASK_SRC_INC_1                          ((uint8_t) 1 << 2)
#define DMA_ROM_TASK_SRC_INC_2                          ((uint8_t) 2 << 2)
#define DMA_ROM_TASK_SRC_INC_4                          ((uint8_t) 3 << 2)
#define DMA_ROM_TASK_DEST_INC_0                         ((uint8_t) 0 << 4)
#define DMA_ROM_TASK_DEST_INC_1                         ((uint8_t) 1 << 4)
#define DMA_ROM_TASK_DEST_INC_2                         ((uint8_t) 2 << 4)
#define DMA_ROM_TASK_DEST_INC_4                         ((uint8_t) 3 << 4)
/**
 * @brief DMA handle type
 */
typedef void *DMA_HANDLE_T;

/**
 * @brief DMA channel callback function type
 * @param	res0: error code
 * @param	res1: 0 = INTA is issued, 1 = INTB is issued
 */
typedef void (*CALLBK_T)(uint32_t res0, uint32_t res1);

/**
 * @brief DMA ROM drivers channel control structure
 */
typedef struct {
	uint8_t event;		/*!< event type selection for DMA transfer
						   - 0: software request
						   - 1: peripheral request
						   - 2: hardware trigger
						   - others: reserved */
	uint8_t hd_trigger;	/*!< In case hardware trigger is enabled, the trigger burst is setup here.
						   NOTE: Rising edge triggered is fixed
						   - bit0~bit3: burst size
						    - 0: burst size =1, 1: 2^1, 2: 2^2,... 10: 1024, others: reserved.
						   - bit4: Source Burst Wrap
						    - 0: Source burst wrapping is not enabled
						    - 1: Source burst wrapping is enabled
						   - bit5: Destination Burst Wrap
						    - 0: Destination burst wrapping is not enabled
						    - 1: Destination burst wrapping is enabled
						   - bit6: Trigger Burst
						    - 0: Hardware trigger cause a single transfer
						    - 1: Hardware trigger cause a burst transfer
						   - bit7: reserved */
	uint8_t priority;	/*!< priority level
						   - 0 -> 7: Highest priority ->  Lowest priority.
						   - other: reserved. */
	uint8_t reserved0;
	CALLBK_T cb_func;	/*!< callback function, Callback function is
						            only invoked when INTA or INTB is enabled. */
}  DMA_CHANNEL_T;

/**
 * @brief DMA ROM driver's TASK parameter structure
 */
typedef struct {
	uint8_t ch_num;		/*!< DMA channel number */
	uint8_t config;		/*!< configuration of this task
						   - bit0: Ping_Pong transfer
						    - 0: Not Ping_Pong transfer
						    - 1: Linked with previous task for Ping_Pong transfer
						   - bit1: Software Trigger.
						    - 0: the trigger for this channel is not set.
						    - 1: the trigger for this channel is set immediately.
						   - bit2:  Clear Trigger
						    - 0: The trigger is not cleared when this task is finished.
						    - 1: The trigger is cleared when this task is finished.
						   - bit3:  Select INTA
						    - 0: No IntA.
						    - 1: The IntB flag for this channel will be set when this task is finished.
						   bit4:  Select INTB
						    0: No IntB.
						    1: The IntB flag for this channel will be set when this task is finished.
						   bit5~bit7: reserved
						 */

	uint8_t data_type;	/*!<
						    - bit0~bit1: Data width. 0: 8-bit, 1: 16-bit, 2: 32-bit, 3: reserved
						    - bit2~bit3: How is source address incremented?
						        - 0: The source address is not incremented for each transfer.
						        1: The source address is incremented by the amount specified by Width for each transfer.
						        2: The source address is incremented by 2 times the amount specified by Width for each transfer.
						        3: The source address is incremented by 4 times the amount specified by Width for each transfer.
						    - bit4~bit5: How is the destination address incremented?
						        0: The destination address is not incremented for each transfer.
						        1: The destination address is incremented by the amount specified by Width for each transfer.
						        2: The destination address is incremented by 2 times the amount specified by Width for each transfer.
						        3: The destination address is incremented by 4 times the amount specified by Width for each transfer.
						    - bit6~bit7: reserved. */
	uint8_t  reserved0;
	uint16_t data_length;	/*!< 0: 1 transfer, 1: 2 transfer, ..., 1023: 1024 transfer. Others: reserved.*/
	uint16_t reserved1;
	uint32_t src;			/*!< Source data end address */
	uint32_t dst;			/*!< Destination end address */
	uint32_t task_addr;		/*!< the address of RAM for saving this task.
							   (NOTE: each task need 16 bytes RAM for storing configuration,
							   and DMA API could set it according user input parameter,
							   but it is responsible of user to allocate this RAM space and
							   make sure that the base address must be 16-byte alignment.
							   And if user has setup the next_task(!=0), the dma_task_link
							   must be called for this task setup, otherwise unpredictable error will happen.) */
} DMA_TASK_T;

/**
 * @brief DMA ROM API structure
 * The DMA API handles DMA set-up and transfers.
 */
typedef struct DMAD_API {
	/** DMA ISR routine */
	void (*dma_isr)(DMA_HANDLE_T *handle);
	/** Get memory size needed for DMA. */
	uint32_t (*dma_get_mem_size)(void);
	/** Set up DMA. */
	DMA_HANDLE_T * (*dma_setup)(uint32_t base_addr, uint8_t * ram);
	/** Enable DMA channel and set-up basic DMA transfer. */
	ErrorCode_t (*dma_init)(DMA_HANDLE_T *handle, DMA_CHANNEL_T *channel, DMA_TASK_T *task);
	/** Create linked transfer. */
	ErrorCode_t (*dma_link)(DMA_HANDLE_T *handle, DMA_TASK_T *task, uint8_t valid);
	/** Set a task to valid. */
	ErrorCode_t (*dma_set_valid)(DMA_HANDLE_T *handle, uint8_t chl_num);
	/** Pause DMA transfer on a given channel. */
	ErrorCode_t (*dma_pause)(DMA_HANDLE_T *handle, uint8_t chl_num);
	/** Resume DMA transfer. */
	ErrorCode_t (*dma_unpause)(DMA_HANDLE_T *handle, uint8_t chl_num);
	/** Cancel DMA transfer on a given channel.*/
	ErrorCode_t (*dma_abort)(DMA_HANDLE_T *handle, uint8_t chl_num);
} DMAD_API_T;

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __ROM_DMA_15XX_H_ */
