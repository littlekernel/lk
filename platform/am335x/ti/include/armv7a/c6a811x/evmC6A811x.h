/**
 * \file   evmC6A811x.h
 *
 * \brief  This file contains prototype declarations of functions which 
 *         performs EVM configurations.
 *         Placeholder (TODO: update for new part)
 */

/* Copyright (C) 2012 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED */

#ifndef _EVM_C6A811X_H_
#define _EVM_C6A811X_H_

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
**                    FUNCTION PROTOTYPES
*****************************************************************************/

/* Pinmux functions */

extern void CPSWPinMuxSetup(void);
extern void DCANPinMuxSetUp(unsigned int instanceNum);
extern void EPWMPinMuxSetup(unsigned int instanceNum);
extern void GPIOPinMuxSetup(unsigned int bank, unsigned int pin,
    char rxa_en, char pts_en, char pul_en, char slw_en);
extern void HSMMCSDPinMuxSetup(unsigned int instanceNum);
extern void I2CPinMuxSetup(unsigned int instanceNum);
extern void McASPPinMuxSetup(unsigned int instanceNum);
extern void McSPIPinMuxSetup(unsigned int instanceNum);
extern void NANDPinMuxSetup(void);
extern void UARTPinMuxSetup(unsigned int instanceNum);

/* Clock configuration functions */

extern void CPSWClkEnable(void);
extern void DCANModuleClkConfig(void);
extern void DMTimerModuleClkConfig(unsigned int instanceNum);
extern void EDMAModuleClkConfig(void);
extern void EPWMSSModuleClkConfig(unsigned int instanceNum);
extern void GPIOModuleClkConfig(unsigned int instanceNum);
extern void GPMCClkConfig(void);
extern void HSMMCSDModuleClkConfig(unsigned int instanceNum);
extern void I2CModuleClkConfig(unsigned int instanceNum);
extern void McASPModuleClkConfig(unsigned int instanceNum);
extern void PWMSSClockEnable(unsigned int instance, unsigned int module);
extern unsigned int PWMSSClockEnableStatusGet(unsigned int instance, unsigned int module);
extern void PWMSSClockStop(unsigned int instance, unsigned int module);
extern unsigned int PWMSSClockStopStatusGet(unsigned int instance, unsigned int module);
extern void PWMSSModuleClkConfig(unsigned int instanceNum);
extern void PWMSSTBClkEnable(unsigned int);
extern void McSPIModuleClkConfig(void);
extern void RTCModuleClkConfig(void);
extern void TSCADCModuleClkConfig(void);
extern void UARTModuleClkConfig(unsigned int instanceNum);
extern void UPDNPinControl(void);
extern void USB0ModuleClkConfig(void);
extern void USBModuleClkEnable(unsigned int ulIndex, unsigned int ulBase);
extern void USBModuleClkDisable(unsigned int ulIndex, unsigned int ulBase);
extern void WDTModuleClkConfig(void);

/* Other functions */

extern int ControlIntMuxARM(unsigned int eventNum, unsigned int muxVal);
extern int ControlIntMuxDSP(unsigned int eventNum, unsigned int muxVal);
extern void DCANMsgRAMInit(unsigned int instanceNum);
extern void EEPROMI2CRead(unsigned char *data, unsigned int length,
                          unsigned short offset);
extern void EEPROMI2CSetUp(unsigned int slaveAddr);
extern void EVMMACAddrGet(unsigned int addrIdx, unsigned char *macAddr);
extern void EVMPortRGMIIModeSelect(void);
extern unsigned int EVMProfileGet(void);
extern unsigned int RTCRevisionInfoGet(void);
extern void USBIntMux(void);

#ifdef __cplusplus
}
#endif

#endif

/******************************** End of file *******************************/
