/**
 * \file   evmAM386x.h
 *
 * \brief  This file contains prototype declarations of functions which 
 *         performs EVM configurations.
 *         Placeholder (TODO: update for new part)
 */

/* Copyright (C) 2012 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED */

#ifndef _EVM_AM386X_H_
#define _EVM_AM386X_H_

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
**                    FUNCTION PROTOTYPES
*****************************************************************************/

/* Pinmux functions */

extern unsigned int CPSWPinMuxSetup(void);
extern unsigned int DCANPinMuxSetUp(unsigned int instanceNum);
extern unsigned int ECAPPinMuxSetup(unsigned int instanceNum);
extern unsigned int EPWM2PinMuxSetup(void);
extern unsigned int GPIO0Pin19PinMuxSetup(void);
extern unsigned int GPIO0Pin6PinMuxSetup(void);
extern unsigned int GPIO0Pin7PinMuxSetup(void);
extern unsigned int GPIO0PinMuxSetup(unsigned int pinNum);
extern unsigned int GPIO1PinMuxSetup(unsigned int pinNo);
extern unsigned int GPIO1Pin16PinMuxSetup(void);
extern unsigned int GPIO1Pin20PinMuxSetup(void);
extern unsigned int GPIO1Pin23PinMuxSetup(void);
extern unsigned int GPIO1Pin28PinMuxSetup(void);
extern unsigned int GPIO1Pin30PinMuxSetup(void);
extern unsigned int GPIO2Pin24PinMuxSetup(void);
extern unsigned int HSMMCSDPinMuxSetup(void);
extern unsigned int I2CPinMuxSetup(unsigned int instanceNum);
extern unsigned int McASP1PinMuxSetup(void);
extern void McSPI0ModuleClkConfig(void);
extern unsigned int NANDPinMuxSetup(void);
extern unsigned int TSCADCPinMuxSetUp(void);
extern unsigned int UARTPinMuxSetup(unsigned int instanceNum);

/* Clock configuration functions */

extern void CPSWClkEnable(void);
extern void DCANModuleClkConfig(void);
extern void DMTimer2ModuleClkConfig(void);
extern void DMTimer3ModuleClkConfig(void);
extern void DMTimer4ModuleClkConfig(void);
extern void DMTimer7ModuleClkConfig(void);
extern void EDMAModuleClkConfig(void);
extern void EPWMSSModuleClkConfig(unsigned int instanceNum);
extern void GPIO0ModuleClkConfig(void);
extern void GPIO1ModuleClkConfig(void);
extern void GPMCClkConfig(void);
extern void HSMMCSDModuleClkConfig(void);
extern void I2C0ModuleClkConfig(void);
extern void I2C1ModuleClkConfig(void);
extern void McASP1ModuleClkConfig(void);
extern unsigned int McSPI0CSPinMuxSetup(unsigned int csPinNum);
extern unsigned int McSPIPinMuxSetup(unsigned int instanceNum);
extern void PWMSSClockEnable(unsigned int instance, unsigned int module);
extern unsigned int PWMSSClockEnableStatusGet(unsigned int instance, unsigned int module);
extern void PWMSSClockStop(unsigned int instance, unsigned int module);
extern unsigned int PWMSSClockStopStatusGet(unsigned int instance, unsigned int module);
extern void PWMSSModuleClkConfig(unsigned int instanceNum);
extern void PWMSSTBClkEnable(unsigned int);
extern void RTCModuleClkConfig(void);
extern void TSCADCModuleClkConfig(void);
extern void UART0ModuleClkConfig(void);
extern void UPDNPinControl(void);
extern void USB0ModuleClkConfig(void);
extern void USBModuleClkEnable(unsigned int ulIndex, unsigned int ulBase);
extern void USBModuleClkDisable(unsigned int ulIndex, unsigned int ulBase);
extern void WDTModuleClkConfig(void);

/* Other functions */

extern void DCANMsgRAMInit(unsigned int instanceNum);
extern void EEPROMI2CRead(unsigned char *data, unsigned int length,
                          unsigned short offset);
extern void EEPROMI2CSetUp(unsigned int slaveAddr);
extern void EVMMACAddrGet(unsigned int addrIdx, unsigned char *macAddr);
extern void EVMPortRGMIIModeSelect(void);
extern unsigned int EVMProfileGet(void);
extern unsigned int RTCRevisionInfoGet(void);

#ifdef __cplusplus
}
#endif

#endif

/******************************** End of file *******************************/
