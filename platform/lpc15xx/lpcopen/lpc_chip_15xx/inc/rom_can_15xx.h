/*
 * @brief LPC15xx CAN ROM API declarations and functions
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

#ifndef __ROM_CAN_15XX_H_
#define __ROM_CAN_15XX_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup CANROM_15XX CHIP: LPC15xx CAN ROM API declarations and functions
 * @ingroup ROMAPI_15XX
 * @{
 */

/* error status bits */
#define CAN_ERROR_NONE 0x00000000UL
#define CAN_ERROR_PASS 0x00000001UL
#define CAN_ERROR_WARN 0x00000002UL
#define CAN_ERROR_BOFF 0x00000004UL
#define CAN_ERROR_STUF 0x00000008UL
#define CAN_ERROR_FORM 0x00000010UL
#define CAN_ERROR_ACK 0x00000020UL
#define CAN_ERROR_BIT1 0x00000040UL
#define CAN_ERROR_BIT0 0x00000080UL
#define CAN_ERROR_CRC 0x00000100UL

typedef void *CAN_HANDLE_T;		/* define TYPE for CAN handle pointer */

typedef struct _CAN_MSG_OBJ {
	uint32_t mode_id;
	uint32_t mask;
	uint8_t data[8];
	uint8_t dlc;
	uint8_t msgobj;
} CAN_MSG_OBJ;

typedef struct _CAN_CALLBACKS {
	void (*CAN_rx)(uint8_t msg_obj);
	void (*CAN_tx)(uint8_t msg_obj);
	void (*CAN_error)(uint32_t error_info);
} CAN_CALLBACKS;

typedef struct _CAN_CFG {
	uint32_t clkdiv;
	uint32_t btr;
	uint32_t isr_ena;
} CAN_CFG;

typedef struct _CAN_ODCONSTENTRY {
	uint16_t index;
	uint8_t subindex;
	uint8_t len;
	uint32_t val;
} CAN_ODCONSTENTRY;

typedef struct _CAN_ODENTRY {
	uint16_t index;
	uint8_t subindex;
	uint8_t entrytype_len;
	uint8_t *val;
} CAN_ODENTRY;

typedef struct _CAN_CANOPENCFG {
	uint8_t node_id;
	uint8_t msgobj_rx;
	uint8_t msgobj_tx;
	uint8_t isr_handled;
	uint32_t od_const_num;
	CAN_ODCONSTENTRY *od_const_table;
	uint32_t od_num;
	CAN_ODENTRY *od_table;
} CAN_CANOPENCFG;

typedef struct _CANOPEN_CALLBACKS {
	uint32_t (*CANOPEN_sdo_read)(uint16_t index, uint8_t subindex);
	uint32_t (*CANOPEN_sdo_write)(uint16_t index, uint8_t subindex, uint8_t *dat_ptr);
	uint32_t (*CANOPEN_sdo_seg_read)(uint16_t index, uint8_t subindex, uint8_t
									 openclose, uint8_t *length, uint8_t *data, uint8_t *last);
	uint32_t (*CANOPEN_sdo_seg_write)(uint16_t index, uint8_t subindex, uint8_t
									  openclose, uint8_t length, uint8_t *data, uint8_t *fast_resp);
	uint8_t (*CANOPEN_sdo_req)(uint8_t length_req, uint8_t *req_ptr, uint8_t
							   *length_resp, uint8_t *resp_ptr);
} CANOPEN_CALLBACKS;

typedef struct _CAN_API_INIT_PARAM_T {
	uint32_t mem_base;			/* Address of user-space memory area to use */
	uint32_t can_reg_base;		/* Address of start of CAN controller register area */
	CAN_CFG *can_cfg;
	CAN_CALLBACKS *callbacks;
	CAN_CANOPENCFG *canopen_cfg;
	CANOPEN_CALLBACKS *co_callbacks;
} CAN_API_INIT_PARAM_T;

/**
 * @brief LPC15XX CAN ROM API structure
 * The CAN profile API provides functions to configure and manage the CAN sub-system.
 */
typedef struct _CAND_API_T {
	uint32_t (*hwCAN_GetMemSize)(CAN_API_INIT_PARAM_T *param);
	ErrorCode_t (*hwCAN_Init)(CAN_HANDLE_T *phCan, CAN_API_INIT_PARAM_T *param);
	void (*hwCAN_Isr)(CAN_HANDLE_T hCan);
	void (*hwCAN_ConfigRxmsgobj)(CAN_HANDLE_T hCan, CAN_MSG_OBJ *msg_obj);
	uint8_t (*hwCAN_MsgReceive)(CAN_HANDLE_T hCan, CAN_MSG_OBJ *msg_obj);
	void (*hwCAN_MsgTransmit)(CAN_HANDLE_T hCan, CAN_MSG_OBJ *msg_obj);
	void (*hwCAN_CANopenHandler)(CAN_HANDLE_T hCan);
} CAND_API_T;

uint32_t hwCAN_GetMemSize(CAN_API_INIT_PARAM_T *param);

ErrorCode_t hwCAN_Init(CAN_HANDLE_T *phCan, CAN_API_INIT_PARAM_T *param);

void hwCAN_Isr(CAN_HANDLE_T hCan);

void hwCAN_ConfigRxmsgobj(CAN_HANDLE_T hCan, CAN_MSG_OBJ *msg_obj);

uint8_t hwCAN_MsgReceive(CAN_HANDLE_T hCan, CAN_MSG_OBJ *msg_obj);

void hwCAN_MsgTransmit(CAN_HANDLE_T hCan, CAN_MSG_OBJ *msg_obj);

void hwCAN_CANopenHandler(CAN_HANDLE_T hCan);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __ROM_CAN_15XX_H_ */
