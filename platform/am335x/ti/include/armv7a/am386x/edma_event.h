/**
 *  \file   edma_event.h
 *
 *  \brief  EDMA event enumeration
 *          Placeholder (TODO: update for new part)
 *
*/

/* Copyright (C) 2012 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED */

#ifndef _EDMAEVENT_H
#define _EDMAEVENT_H

#include "hw_types.h"

#ifdef __cplusplus
extern "C" {
#endif
/****************************************************************************
**                       MACRO DEFINITIONS
****************************************************************************/

/********************* Direct Mapped Events ********************************/
/* Events for McASP 1*/
#define EDMA3_CHA_MCASP1_TX               (10u)
#define EDMA3_CHA_MCASP1_RX               (11u)

/* MCSPI0 Channel 0 transmit event */
#define EDMA3_CHA_MCSPI0_CH0_TX           (16u)
/* MCSPI0 Channel 0 receive event */
#define EDMA3_CHA_MCSPI0_CH0_RX           (17u)
/* MCSPI0 Channel 1 transmit event */
#define EDMA3_CHA_MCSPI0_CH1_TX           (18u)
/* MCSPI0 Channel 1 receive event */
#define EDMA3_CHA_MCSPI0_CH1_RX           (19u)

/* UART0 Transmit Event. */
#define EDMA3_CHA_UART0_TX                (26u)
/* UART0 Receive Event. */
#define EDMA3_CHA_UART0_RX                (27u)

/* MCSPI1 Channel 0 transmit event */
#define EDMA3_CHA_MCSPI1_CH0_TX           (42u)
/* MCSPI1 Channel 0 receive event */
#define EDMA3_CHA_MCSPI1_CH0_RX           (43u)
/* MCSPI1 Channel 1 transmit event */
#define EDMA3_CHA_MCSPI1_CH1_TX           (44u)
/* MCSPI1 Channel 1 receive event */
#define EDMA3_CHA_MCSPI1_CH1_RX           (45u)
/* I2C0 Transmit Event */
#define EDMA3_CHA_I2C0_TX                 (58u)
/* I2C0 Receive Event */
#define EDMA3_CHA_I2C0_RX                 (59u)
/* I2C1 Receive Event */
#define EDMA3_CHA_I2C1_TX                 (60u)
/* I2C1 Transmit Event */
#define EDMA3_CHA_I2C1_RX                 (61u)



/********************** Cross Mapped Events ********************************/
/* I2C2 Receive Event */
#define EDMA3_CHA_HSI2C2_RX               (4u)
/* I2C2 Transmit Event */
#define EDMA3_CHA_HSI2C2_TX               (3u)



/*
**EDMA Event number list
*/
#ifdef __cplusplus
}
#endif

#endif

