/*
 * @brief LPC15xx ROM API declarations and functions
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

#ifndef __ROMAPI_15XX_H_
#define __ROMAPI_15XX_H_

#include "iap.h"
#include "eeprom.h"
#include "error.h"
#include "rom_i2c_15xx.h"
#include "rom_pwr_15xx.h"
#include "rom_uart_15xx.h"
#include "rom_can_15xx.h"
#include "rom_dma_15xx.h"
#include "rom_spi_15xx.h"
#include "rom_adc_15xx.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup ROMAPI_15XX CHIP: LPC15xx ROM API declarations and functions
 * @ingroup CHIP_15XX_Drivers
 * @{
 */

/**
 * @brief LPC15XX High level ROM API structure
 */
typedef struct {
	const uint32_t pUSBD;					/*!< USBD API function table base address */
	const uint32_t reserved0;				/*!< Reserved */
	const CAND_API_T *pCAND;				/*!< C_CAN API function table base address */
	const PWRD_API_T *pPWRD;				/*!< Power API function table base address */
	const uint32_t reserved1;				/*!< Reserved */
	const I2CD_API_T *pI2CD;				/*!< I2C driver API function table base address */
	const DMAD_API_T *pDMAD;				/*!< DMA driver API function table base address */
	const SPID_API_T *pSPID;				/*!< I2C driver API function table base address */
	const ADCD_API_T *pADCD;				/*!< ADC driver API function table base address */
	const UARTD_API_T *pUARTD;				/*!< UART driver API function table base address */
} LPC_ROM_API_T;

/* Pointer to ROM API function address */
#define LPC_ROM_API_BASE_LOC    0x03000200UL
#define LPC_ROM_API     (*(LPC_ROM_API_T * *) LPC_ROM_API_BASE_LOC)

/* Pointer to @ref CAND_API_T functions in ROM */
#define LPC_CAND_API    ((LPC_ROM_API)->pCAND)

/* Pointer to @ref PWRD_API_T functions in ROM */
#define LPC_PWRD_API    ((LPC_ROM_API)->pPWRD)

/* Pointer to @ref I2CD_API_T functions in ROM */
#define LPC_I2CD_API    ((LPC_ROM_API)->pI2CD)

/* Pointer to @ref DMAD_API_T functions in ROM for DMA */
#define LPC_DMAD_API    ((LPC_ROM_API)->pDMAD)

/* Pointer to @ref SPID_API_T functions in ROM for DMA */
#define LPC_SPID_API    ((LPC_ROM_API)->pSPID)

/* Pointer to @ref ADCD_API_T functions in ROM for pADCD */
#define LPC_ADCD_API    ((LPC_ROM_API)->pADCD)

/* Pointer to @ref UARTD_API_T functions in ROM for UARTs */
#define LPC_UARTD_API   ((LPC_ROM_API)->pUARTD)

/* Pointer to ROM IAP entry functions */
#define IAP_ENTRY_LOCATION        0x03000205UL

/**
 * @brief LPC15XX IAP_ENTRY API function type
 */
static INLINE void iap_entry(unsigned int cmd_param[5], unsigned int status_result[4])
{
	((IAP_ENTRY_T) IAP_ENTRY_LOCATION)(cmd_param, status_result);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __ROMAPI_11U6X_H_ */
