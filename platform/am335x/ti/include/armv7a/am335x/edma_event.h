/*
 *  \file   edma_event.h
 *
 *  \brief  EDMA event enumeration
 *
*/

/*
* Copyright (C) 2010 Texas Instruments Incorporated - http://www.ti.com/
*/
/*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*    Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*
*    Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the
*    distribution.
*
*    Neither the name of Texas Instruments Incorporated nor the names of
*    its contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
*  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
*  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
*  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/


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

