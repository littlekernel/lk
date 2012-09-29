/*
 * \file   beaglebone.h
 *
 * \brief  This file contains prototype declarations of functions which 
 *         performs EVM configurations.
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


#ifndef _BEALGEBONE_H_
#define _BEAGLEBONE_H_

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
**                    FUNCTION PROTOTYPES
*****************************************************************************/
extern void GPIO1ModuleClkConfig(void);
extern void GPIO1Pin23PinMuxSetup(void);
extern void GPIO0ModuleClkConfig(void);
extern void UART0ModuleClkConfig(void);
extern void UARTPinMuxSetup(unsigned int instanceNum);
extern void CPSWPinMuxSetup(void);
extern void CPSWClkEnable(void);
extern unsigned int RTCRevisionInfoGet(void);
extern void EDMAModuleClkConfig(void);
extern void EVMMACAddrGet(unsigned int addrIdx, unsigned char *macAddr);
extern void WatchdogTimer1ModuleClkConfig(void);
extern void DMTimer2ModuleClkConfig(void);
extern void DMTimer3ModuleClkConfig(void);
extern void DMTimer4ModuleClkConfig(void);
extern void DMTimer7ModuleClkConfig(void);
extern void EVMPortMIIModeSelect(void);
extern void RTCModuleClkConfig(void);
extern void HSMMCSDModuleClkConfig(void);
extern void HSMMCSDPinMuxSetup(void);
extern void I2C0ModuleClkConfig(void);
extern void I2C1ModuleClkConfig(void);
extern void I2CPinMuxSetup(unsigned int instance);

#ifdef __cplusplus
}
#endif

#endif

/******************************** End of file *******************************/
