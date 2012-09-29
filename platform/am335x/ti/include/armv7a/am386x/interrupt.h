/**
 *  \file   interrupt.h
 *
 *  \brief  Interrupt related API declarations.
 *
 *   This file contains the API prototypes for configuring INTC for 
 *   ARM Cortex-A8
*/

/* Copyright (C) 2012 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED */

#ifndef INTERRUPT_H
#define INTERRUPT_H

#include "hw_intc.h"
#include "hw_types.h"

#ifdef __cplusplus
extern "C" {
#endif
/****************************************************************************
**                       MACRO DEFINITIONS
****************************************************************************/
/*
** Macros which can be passed to IntPrioritySet API as hostIntRoute.
*/
/* To route an interrupt to IRQ */
#define AINTC_HOSTINT_ROUTE_IRQ                (0)

/* To route an interrupt to FIQ */
#define AINTC_HOSTINT_ROUTE_FIQ                (INTC_ILR_FIQNIRQ)

/*
** Interrupt number list
*/
#define SYS_INT_EMUINT                         (0)
#define SYS_INT_COMMTX                         (1)
#define SYS_INT_COMMRX                         (2)
#define SYS_INT_BENCH                          (3)
#define SYS_INT_ELM_IRQ                        (4)
#define SYS_INT_SSM_WFI_IRQ                    (5)
#define SYS_INT_SSM_IRQ                        (6)
#define SYS_INT_NMI                            (7)
#define SYS_INT_SEC_EVNT                       (8)
#define SYS_INT_L3DEBUG                        (9)
#define SYS_INT_L3APPINT                       (10)
#define SYS_INT_TINT8                          (11)
#define SYS_INT_EDMACOMPINT                    (12)
#define SYS_INT_EDMAMPERR                      (13)
#define SYS_INT_EDMAERRINT                     (14)
#define SYS_INT_WDTINT                         (15)
#define SYS_INT_SATA0INT                       (16)
#define SYS_INT_USBSSINT                       (17)
#define SYS_INT_USBINT0                        (18)
#define SYS_INT_USBINT1                        (19)
#define SYS_INT_TPPSSERR                       (20)
#define SYS_INT_TPPMBOXINT                     (21)
#define SYS_INT_TPPSTCINT0                     (22)
#define SYS_INT_TPPSTCINT1                     (23)
#define SYS_INT_TPPDMAPKT                      (24)
#define SYS_INT_TPPDMABS                       (25)
#define SYS_INT_TPPERRINT0                     (26)
#define SYS_INT_TPPERRINT1                     (27)
#define SYS_INT_SDINT1                         (28)
#define SYS_INT_SDINT2                         (29)
#define SYS_INT_I2CINT2                        (30)
#define SYS_INT_I2CINT3                        (31)
#define SYS_INT_GPIOINT2A                      (32)
#define SYS_INT_GPIOINT2B                      (33)
#define SYS_INT_USBWAKEUP                      (34)
#define SYS_INT_PCIeWAKEUP                     (35)
#define SYS_INT_DSSINT                         (36)
#define SYS_INT_GFXINT                         (37)
#define SYS_INT_HDMIINT                        (38)
#define SYS_INT_ISS_IRQ_5                      (39)
#define SYS_INT_3PGSWRXTHR0                    (40)
#define SYS_INT_3PGSWRXINT0                    (41)
#define SYS_INT_3PGSWTXINT0                    (42)
#define SYS_INT_3PGSWMISC0                     (43)
#define SYS_INT_UARTINT3                       (44)
#define SYS_INT_UARTINT4                       (45)
#define SYS_INT_UARTINT5                       (46)
#define SYS_INT_FDIF_IRQ3                      (47)
#define SYS_INT_PCIINT0                        (48)
#define SYS_INT_PCIINT1                        (49)
#define SYS_INT_PCIINT2                        (50)
#define SYS_INT_PCIINT3                        (51)
#define SYS_INT_DCAN0_INT0                     (52)
#define SYS_INT_DCAN0_INT1                     (53)
#define SYS_INT_DCAN0_PARITY                   (54)
#define SYS_INT_DCAN1_INT0                     (55)
#define SYS_INT_DCAN1_INT1                     (56)
#define SYS_INT_DCAN1_PARITY                   (57)
#define SYS_INT_MLB_SYS_INT0                   (58)
#define SYS_INT_MLB_SYS_INT1                   (59)
#define SYS_INT_MLB_INT                        (60)
#define SYS_INT_VCPINT                         (61)
#define SYS_INT_GPIOINT3A                      (62)
#define SYS_INT_GPIOINT3B                      (63)
#define SYS_INT_SDINT0                         (64)
#define SYS_INT_SPIINT0                        (65)
#define SYS_INT_TINT0                          (66)
#define SYS_INT_TINT1                          (67)
#define SYS_INT_TINT2                          (68)
#define SYS_INT_TINT3                          (69)
#define SYS_INT_I2CINT0                        (70)
#define SYS_INT_I2CINT1                        (71)
#define SYS_INT_UARTINT0                       (72)
#define SYS_INT_UARTINT1                       (73)
#define SYS_INT_UARTINT2                       (74)
#define SYS_INT_RTCINT                         (75)
#define SYS_INT_RTCALARMINT                    (76)
#define SYS_INT_MB1INT                         (77)
#define SYS_INT_IDEINTR                        (78)
#define SYS_INT_PLL RECAL                      (79)
#define SYS_INT_MCATXINT0                      (80)
#define SYS_INT_MCARXINT0                      (81)
#define SYS_INT_MCATXINT1                      (82)
#define SYS_INT_MCARXINT1                      (83)
#define SYS_INT_MCATXINT2                      (84)
#define SYS_INT_MCARXINT2                      (85)
#define SYS_INT_MCBSPINT                       (86)
#define SYS_INT_SMCDINT0                       (87)
#define SYS_INT_SMCDINT1                       (88)
#define SYS_INT_SMRFLX_Active                  (89)
#define SYS_INT_SMRFLX_IVA                     (90)
#define SYS_INT_WDTINT                         (91)
#define SYS_INT_TINT4                          (92)
#define SYS_INT_TINT5                          (93)
#define SYS_INT_TINT6                          (94)
#define SYS_INT_TINT7                          (95)
#define SYS_INT_GPIOINT0A                      (96)
#define SYS_INT_GPIOINT0B                      (97)
#define SYS_INT_GPIOINT1A                      (98)
#define SYS_INT_GPIOINT1B                      (99)
#define SYS_INT_GPMCINT                        (100)
#define SYS_INT_DDRERR0                        (101)
#define SYS_INT_DDRERR1                        (102)
#define SYS_INT_IVA0CONT1SYNC                  (103)
#define SYS_INT_IVA0CONT2SYNC                  (104)
#define SYS_INT_MCATXINT3                      (105)
#define SYS_INT_MCARXINT3                      (106)
#define SYS_INT_IVA0MBOXINT                    (107)
#define SYS_INT_MCATXINT4                      (108)
#define SYS_INT_MCARXINT4                      (109)
#define SYS_INT_MCATXINT5                      (110)
#define SYS_INT_MCARXINT5                      (111)
#define SYS_INT_TCERRINT0                      (112)
#define SYS_INT_TCERRINT1                      (113)
#define SYS_INT_TCERRINT2                      (114)
#define SYS_INT_TCERRINT3                      (115)
#define SYS_INT_SECPUBINT                      (116)
#define SYS_INT_SECSECINT                      (117)
#define SYS_INT_SECPUBSWINT                    (118)
#define SYS_INT_SECSECSWINT                    (119)
#define SYS_INT_SMRFLX_Modena                  (120)
#define SYS_INT_SMRFLX_Core                    (121)
#define SYS_INT_MMUINT                         (122)
#define SYS_INT_MCMMUINT                       (123)
#define SYS_INT_DMMINT                         (124)
#define SYS_INT_SPIINT1                        (125)
#define SYS_INT_SPIINT2                        (126)
#define SYS_INT_SPIINT3                        (127)

/*****************************************************************************
**                     API FUNCTION PROTOTYPES
*****************************************************************************/
extern void IntAINTCInit (void);
extern void IntIfClkFreeRunSet(void);
extern void IntIfClkAutoGateSet(void);
extern void IntProtectionEnable(void);
extern void IntProtectionDisable(void);
extern void IntSyncClkFreeRunSet(void);
extern void IntSyncClkAutoGateSet(void);
extern void IntFuncClkFreeRunSet(void);
extern void IntFuncClkAutoGateSet(void);
extern void IntPriorityThresholdSet(unsigned int threshold);
extern void IntSoftwareIntSet(unsigned int intrNum);
extern void IntSoftwareIntClear(unsigned int intrNum);
extern void IntMasterIRQEnable(void);
extern void IntMasterIRQDisable(void);
extern void IntMasterFIQEnable(void);
extern void IntMasterFIQDisable(void);
extern void IntSystemEnable(unsigned int intrNum);
extern void IntSystemDisable(unsigned int intrNum);
extern void IntUnRegister(unsigned int intrNum);
extern void IntEnable(unsigned char  status);
extern void IntRegister(unsigned int intrNum, void (*pfnHandler)(void));
extern void IntPrioritySet(unsigned int intrNum, unsigned int priority,
                           unsigned int hostIntRoute);
extern unsigned char IntDisable(void);
extern unsigned int IntMasterStatusGet(void);
extern unsigned int IntActiveIrqNumGet(void);
extern unsigned int IntActiveFiqNumGet(void);
extern unsigned int IntSpurIrqFlagGet(void);
extern unsigned int IntSpurFiqFlagGet(void);
extern unsigned int IntCurrIrqPriorityGet(void);
extern unsigned int IntCurrFiqPriorityGet(void);
extern unsigned int IntPriorityThresholdGet(void);
extern unsigned int IntRawStatusGet(unsigned int intrNum);
extern unsigned int IntPendingIrqMaskedStatusGet(unsigned int intrNum);
extern unsigned int IntPendingFiqMaskedStatusGet(unsigned int intrNum);

#ifdef __cplusplus
}
#endif
#endif

