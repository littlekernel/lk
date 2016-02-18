/*
 * @brief Programming API used with Virtual Communication port
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

#ifndef __CDC_VCOM_H_
#define __CDC_VCOM_H_

#include "app_usbd_cfg.h"

#ifdef __cplusplus
extern "C"
{
#endif

/** @ingroup EXAMPLES_USBDROM_15XX_COMPOSITE
 * @{
 */

#define VCOM_RX_BUF_SZ      512
#define VCOM_TX_CONNECTED   _BIT(8)		/* connection state is for both RX/Tx */
#define VCOM_TX_BUSY        _BIT(0)
#define VCOM_RX_DONE        _BIT(0)
#define VCOM_RX_BUF_FULL    _BIT(1)
#define VCOM_RX_BUF_QUEUED  _BIT(2)
#define VCOM_RX_DB_QUEUED   _BIT(3)

/**
 * Structure containing Virtual Comm port control data
 */
typedef struct VCOM_DATA {
	USBD_HANDLE_T hUsb;
	USBD_HANDLE_T hCdc;
	uint8_t *rx_buff;
	uint16_t rx_rd_count;
	uint16_t rx_count;
	volatile uint16_t tx_flags;
	volatile uint16_t rx_flags;
} VCOM_DATA_T;

/**
 * Virtual Comm port control data instance.
 */
extern VCOM_DATA_T g_vCOM;

/**
 * @brief	Virtual com port init routine
 * @param	hUsb		: Handle to USBD stack instance
 * @param	pDesc		: Pointer to configuration descriptor
 * @param	pUsbParam	: Pointer USB param structure returned by previous init call
 * @return	Always returns LPC_OK.
 */
ErrorCode_t vcom_init (USBD_HANDLE_T hUsb, USB_CORE_DESCS_T *pDesc, USBD_API_INIT_PARAM_T *pUsbParam);

/**
 * @brief	Virtual com port buffered read routine
 * @param	pBuf	: Pointer to buffer where read data should be copied
 * @param	buf_len	: Length of the buffer passed
 * @return	Return number of bytes read.
 */
uint32_t vcom_bread (uint8_t *pBuf, uint32_t buf_len);

/**
 * @brief	Virtual com port read routine
 * @param	pBuf	: Pointer to buffer where read data should be copied
 * @param	buf_len	: Length of the buffer passed
 * @return	Always returns LPC_OK.
 */
ErrorCode_t vcom_read_req (uint8_t *pBuf, uint32_t buf_len);

/**
 * @brief	Gets current read count.
 * @return	Returns current read count.
 */
uint32_t vcom_read_cnt(void);

/**
 * @brief	Check if Vcom is connected
 * @return	Returns non-zero value if connected.
 */
static INLINE uint32_t vcom_connected(void) {
	return g_vCOM.tx_flags & VCOM_TX_CONNECTED;
}

/**
 * @brief	Virtual com port write routine
 * @param	pBuf	: Pointer to buffer to be written
 * @param	buf_len	: Length of the buffer passed
 * @return	Number of bytes written
 */
uint32_t vcom_write (uint8_t *pBuf, uint32_t buf_len);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __CDC_VCOM_H_ */
