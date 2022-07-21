/**
  ******************************************************************************
  * @file    stm32f4xx_syscfg.h
  * @author  MCD Application Team
  * @version V1.8.1
  * @date    27-January-2022
  * @brief   This file contains all the functions prototypes for the SYSCFG firmware
  *          library. 
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2016 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM32F4xx_SYSCFG_H
#define __STM32F4xx_SYSCFG_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"

/** @addtogroup STM32F4xx_StdPeriph_Driver
  * @{
  */

/** @addtogroup SYSCFG
  * @{
  */ 

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/** @defgroup SYSCFG_Exported_Constants 
  * @{
  */ 
#if defined(STM32F413_423xx)
/** @defgroup BITSTREAM_CLOCK  Bit Stream clock source selection
  * @{
  */
#define BITSTREAM_CLOCK_DFSDM2     SYSCFG_MCHDLYCR_BSCKSEL
#define BITSTREAM_CLOCK_TIM2OC1    (uint32_t)0x00000000
/**
  * @}
  */
  
/** @defgroup MCHDLY_CLOCK  MCHDLY Clock enable
  * @{
  */
#define MCHDLY_CLOCK_DFSDM2       SYSCFG_MCHDLYCR_MCHDLY2EN
#define MCHDLY_CLOCK_DFSDM1       SYSCFG_MCHDLYCR_MCHDLY1EN
/**
  * @}
  */  
  
/** @defgroup DFSDM_CLOCKIN_SOURCE   DFSDM Clock In Source Selection
  * @{
  */
#define DFSDM2_CKIN_PAD           (uint32_t)0x00000000
#define DFSDM2_CKIN_DM            SYSCFG_MCHDLYCR_DFSDM2CFG
#define DFSDM1_CKIN_PAD           (uint32_t)0x00000000
#define DFSDM1_CKIN_DM            SYSCFG_MCHDLYCR_DFSDM1CFG
/**
  * @}
  */
  
/** @defgroup DFSDM_CLOCKOUT_SOURCE   DFSDM Clock Source Selection
  * @{
  */
#define DFSDM2_CKOUT_DFSDM2       (uint32_t)0x00000000
#define DFSDM2_CKOUT_M27          SYSCFG_MCHDLYCR_DFSDM2CKOSEL
#define DFSDM1_CKOUT_DFSDM1       (uint32_t)0x00000000U
#define DFSDM1_CKOUT_M27          SYSCFG_MCHDLYCR_DFSDM1CKOSEL
/**
  * @}
  */
  
/** @defgroup DFSDM_DATAIN0_SOURCE   DFSDM Source Selection For DATAIN0
  * @{
  */
#define DATAIN0_DFSDM2_PAD        (uint32_t)0x00000000
#define DATAIN0_DFSDM2_DATAIN1    SYSCFG_MCHDLYCR_DFSDM2D0SEL
#define DATAIN0_DFSDM1_PAD        (uint32_t)0x00000000
#define DATAIN0_DFSDM1_DATAIN1    SYSCFG_MCHDLYCR_DFSDM1D0SEL
/**
  * @}
  */  
  
/** @defgroup DFSDM_DATAIN2_SOURCE   DFSDM Source Selection For DATAIN2
  * @{
  */
#define DATAIN2_DFSDM2_PAD        (uint32_t)0x00000000
#define DATAIN2_DFSDM2_DATAIN3    SYSCFG_MCHDLYCR_DFSDM2D2SEL
#define DATAIN2_DFSDM1_PAD        (uint32_t)0x00000000
#define DATAIN2_DFSDM1_DATAIN3    SYSCFG_MCHDLYCR_DFSDM1D2SEL
/**
  * @}
  */ 
 
/** @defgroup DFSDM_DATAIN4_SOURCE   DFSDM Source Selection For DATAIN4
  * @{
  */
#define DATAIN4_DFSDM2_PAD        (uint32_t)0x00000000
#define DATAIN4_DFSDM2_DATAIN5    SYSCFG_MCHDLYCR_DFSDM2D4SEL
/**
  * @}
  */ 
  
/** @defgroup DFSDM_DATAIN6_SOURCE   DFSDM Source Selection For DATAIN6
  * @{
  */
#define DATAIN6_DFSDM2_PAD        (uint32_t)0x00000000
#define DATAIN6_DFSDM2_DATAIN7    SYSCFG_MCHDLYCR_DFSDM2D6SEL
/**
  * @}
  */ 

/** @defgroup DFSDM_CLKIN_SOURCE   DFSDM1 Source Selection For CLKIN
  * @{
  */
#define DFSDM1_CLKIN0_TIM4OC2      (uint32_t)0x00000000
#define DFSDM1_CLKIN2_TIM4OC2      SYSCFG_MCHDLYCR_DFSDM1CK02SEL
#define DFSDM1_CLKIN1_TIM4OC1      (uint32_t)0x00000000
#define DFSDM1_CLKIN3_TIM4OC1      SYSCFG_MCHDLYCR_DFSDM1CK13SEL
/**
  * @}
  */ 
  
/** @defgroup DFSDM_CLKIN_SOURCE   DFSDM2 Source Selection For CLKIN
  * @{
  */
#define DFSDM2_CLKIN0_TIM3OC4      (uint32_t)0x00000000
#define DFSDM2_CLKIN4_TIM3OC4      SYSCFG_MCHDLYCR_DFSDM2CK04SEL
#define DFSDM2_CLKIN1_TIM3OC3      (uint32_t)0x00000000
#define DFSDM2_CLKIN5_TIM3OC3      SYSCFG_MCHDLYCR_DFSDM2CK15SEL
#define DFSDM2_CLKIN2_TIM3OC2      (uint32_t)0x00000000
#define DFSDM2_CLKIN6_TIM3OC2      SYSCFG_MCHDLYCR_DFSDM2CK26SEL
#define DFSDM2_CLKIN3_TIM3OC1      (uint32_t)0x00000000
#define DFSDM2_CLKIN7_TIM3OC1      SYSCFG_MCHDLYCR_DFSDM2CK37SEL
/**
  * @}
  */      
#endif /* STM32F413_423xx */

/** @defgroup SYSCFG_EXTI_Port_Sources 
  * @{
  */ 
#define EXTI_PortSourceGPIOA       ((uint8_t)0x00)
#define EXTI_PortSourceGPIOB       ((uint8_t)0x01)
#define EXTI_PortSourceGPIOC       ((uint8_t)0x02)
#define EXTI_PortSourceGPIOD       ((uint8_t)0x03)
#define EXTI_PortSourceGPIOE       ((uint8_t)0x04)
#define EXTI_PortSourceGPIOF       ((uint8_t)0x05)
#define EXTI_PortSourceGPIOG       ((uint8_t)0x06)
#define EXTI_PortSourceGPIOH       ((uint8_t)0x07)
#define EXTI_PortSourceGPIOI       ((uint8_t)0x08)
#define EXTI_PortSourceGPIOJ       ((uint8_t)0x09)
#define EXTI_PortSourceGPIOK       ((uint8_t)0x0A)

#define IS_EXTI_PORT_SOURCE(PORTSOURCE) (((PORTSOURCE) == EXTI_PortSourceGPIOA) || \
                                         ((PORTSOURCE) == EXTI_PortSourceGPIOB) || \
                                         ((PORTSOURCE) == EXTI_PortSourceGPIOC) || \
                                         ((PORTSOURCE) == EXTI_PortSourceGPIOD) || \
                                         ((PORTSOURCE) == EXTI_PortSourceGPIOE) || \
                                         ((PORTSOURCE) == EXTI_PortSourceGPIOF) || \
                                         ((PORTSOURCE) == EXTI_PortSourceGPIOG) || \
                                         ((PORTSOURCE) == EXTI_PortSourceGPIOH) || \
                                         ((PORTSOURCE) == EXTI_PortSourceGPIOI) || \
                                         ((PORTSOURCE) == EXTI_PortSourceGPIOJ) || \
                                         ((PORTSOURCE) == EXTI_PortSourceGPIOK))
                                         
/**
  * @}
  */ 


/** @defgroup SYSCFG_EXTI_Pin_Sources 
  * @{
  */ 
#define EXTI_PinSource0            ((uint8_t)0x00)
#define EXTI_PinSource1            ((uint8_t)0x01)
#define EXTI_PinSource2            ((uint8_t)0x02)
#define EXTI_PinSource3            ((uint8_t)0x03)
#define EXTI_PinSource4            ((uint8_t)0x04)
#define EXTI_PinSource5            ((uint8_t)0x05)
#define EXTI_PinSource6            ((uint8_t)0x06)
#define EXTI_PinSource7            ((uint8_t)0x07)
#define EXTI_PinSource8            ((uint8_t)0x08)
#define EXTI_PinSource9            ((uint8_t)0x09)
#define EXTI_PinSource10           ((uint8_t)0x0A)
#define EXTI_PinSource11           ((uint8_t)0x0B)
#define EXTI_PinSource12           ((uint8_t)0x0C)
#define EXTI_PinSource13           ((uint8_t)0x0D)
#define EXTI_PinSource14           ((uint8_t)0x0E)
#define EXTI_PinSource15           ((uint8_t)0x0F)
#define IS_EXTI_PIN_SOURCE(PINSOURCE) (((PINSOURCE) == EXTI_PinSource0)  || \
                                       ((PINSOURCE) == EXTI_PinSource1)  || \
                                       ((PINSOURCE) == EXTI_PinSource2)  || \
                                       ((PINSOURCE) == EXTI_PinSource3)  || \
                                       ((PINSOURCE) == EXTI_PinSource4)  || \
                                       ((PINSOURCE) == EXTI_PinSource5)  || \
                                       ((PINSOURCE) == EXTI_PinSource6)  || \
                                       ((PINSOURCE) == EXTI_PinSource7)  || \
                                       ((PINSOURCE) == EXTI_PinSource8)  || \
                                       ((PINSOURCE) == EXTI_PinSource9)  || \
                                       ((PINSOURCE) == EXTI_PinSource10) || \
                                       ((PINSOURCE) == EXTI_PinSource11) || \
                                       ((PINSOURCE) == EXTI_PinSource12) || \
                                       ((PINSOURCE) == EXTI_PinSource13) || \
                                       ((PINSOURCE) == EXTI_PinSource14) || \
                                       ((PINSOURCE) == EXTI_PinSource15))
/**
  * @}
  */ 


/** @defgroup SYSCFG_Memory_Remap_Config 
  * @{
  */ 
#define SYSCFG_MemoryRemap_Flash       ((uint8_t)0x00)
#define SYSCFG_MemoryRemap_SystemFlash ((uint8_t)0x01)
#define SYSCFG_MemoryRemap_SRAM        ((uint8_t)0x03)
#define SYSCFG_MemoryRemap_SDRAM       ((uint8_t)0x04)

#if defined (STM32F40_41xxx) || defined(STM32F412xG) || defined(STM32F413_423xx)
#define SYSCFG_MemoryRemap_FSMC        ((uint8_t)0x02) 
#endif /* STM32F40_41xxx || STM32F412xG || STM32F413_423xx */

#if defined (STM32F427_437xx) || defined (STM32F429_439xx)
#define SYSCFG_MemoryRemap_FMC         ((uint8_t)0x02) 
#endif /* STM32F427_437xx ||  STM32F429_439xx */  

#if defined (STM32F446xx) || defined (STM32F469_479xx)
#define SYSCFG_MemoryRemap_ExtMEM      ((uint8_t)0x02) 
#endif /*  STM32F446xx || STM32F469_479xx */ 

#if defined (STM32F40_41xxx) || defined(STM32F412xG) || defined(STM32F413_423xx)
#define IS_SYSCFG_MEMORY_REMAP_CONFING(REMAP) (((REMAP) == SYSCFG_MemoryRemap_Flash)       || \
                                               ((REMAP) == SYSCFG_MemoryRemap_SystemFlash) || \
                                               ((REMAP) == SYSCFG_MemoryRemap_SRAM)        || \
                                               ((REMAP) == SYSCFG_MemoryRemap_FSMC))
#endif /* STM32F40_41xxx || STM32F412xG || STM32F413_423xx */

#if defined (STM32F401xx) || defined (STM32F410xx) || defined (STM32F411xE)
#define IS_SYSCFG_MEMORY_REMAP_CONFING(REMAP) (((REMAP) == SYSCFG_MemoryRemap_Flash)       || \
                                               ((REMAP) == SYSCFG_MemoryRemap_SystemFlash) || \
                                               ((REMAP) == SYSCFG_MemoryRemap_SRAM))
#endif /* STM32F401xx || STM32F410xx || STM32F411xE */

#if defined (STM32F427_437xx) || defined (STM32F429_439xx)
#define IS_SYSCFG_MEMORY_REMAP_CONFING(REMAP) (((REMAP) == SYSCFG_MemoryRemap_Flash)       || \
                                               ((REMAP) == SYSCFG_MemoryRemap_SystemFlash) || \
                                               ((REMAP) == SYSCFG_MemoryRemap_SRAM)        || \
                                               ((REMAP) == SYSCFG_MemoryRemap_SDRAM)       || \
                                               ((REMAP) == SYSCFG_MemoryRemap_FMC))
#endif /* STM32F427_437xx ||  STM32F429_439xx */

#if defined (STM32F446xx) || defined (STM32F469_479xx)
#define IS_SYSCFG_MEMORY_REMAP_CONFING(REMAP) (((REMAP) == SYSCFG_MemoryRemap_Flash)       || \
                                               ((REMAP) == SYSCFG_MemoryRemap_ExtMEM)      || \
                                               ((REMAP) == SYSCFG_MemoryRemap_SystemFlash) || \
                                               ((REMAP) == SYSCFG_MemoryRemap_SRAM)        || \
                                               ((REMAP) == SYSCFG_MemoryRemap_SDRAM))
#endif /* STM32F446xx || STM32F469_479xx */

#if defined(STM32F410xx) || defined(STM32F412xG) || defined(STM32F413_423xx)
#define SYSCFG_Break_PVD             SYSCFG_CFGR2_PVDL
#define SYSCFG_Break_HardFault       SYSCFG_CFGR2_CLL

#define IS_SYSCFG_LOCK_CONFIG(BREAK) (((BREAK) == SYSCFG_Break_PVD)       || \
                                      ((BREAK) == SYSCFG_Break_HardFault))
#endif /* STM32F410xx || STM32F412xG || STM32F413_423xx */
/**
  * @}
  */ 


/** @defgroup SYSCFG_ETHERNET_Media_Interface 
  * @{
  */ 
#define SYSCFG_ETH_MediaInterface_MII    ((uint32_t)0x00000000)
#define SYSCFG_ETH_MediaInterface_RMII   ((uint32_t)0x00000001)

#define IS_SYSCFG_ETH_MEDIA_INTERFACE(INTERFACE) (((INTERFACE) == SYSCFG_ETH_MediaInterface_MII) || \
                                                 ((INTERFACE) == SYSCFG_ETH_MediaInterface_RMII))
/**
  * @}
  */ 

/**
  * @}
  */ 

/* Exported macro ------------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/ 
 
void       SYSCFG_DeInit(void);
void       SYSCFG_MemoryRemapConfig(uint8_t SYSCFG_MemoryRemap);
void       SYSCFG_MemorySwappingBank(FunctionalState NewState);
void       SYSCFG_EXTILineConfig(uint8_t EXTI_PortSourceGPIOx, uint8_t EXTI_PinSourcex);
void       SYSCFG_ETH_MediaInterfaceConfig(uint32_t SYSCFG_ETH_MediaInterface); 
void       SYSCFG_CompensationCellCmd(FunctionalState NewState); 
FlagStatus SYSCFG_GetCompensationCellStatus(void);
#if defined(STM32F410xx) || defined(STM32F412xG) || defined(STM32F413_423xx)
void       SYSCFG_BreakConfig(uint32_t SYSCFG_Break);
#endif /* STM32F410xx || STM32F412xG || STM32F413_423xx */
#if defined(STM32F413_423xx)
void DFSDM_BitstreamClock_SourceSelection(uint32_t source);
void DFSDM_DisableDelayClock(uint32_t MCHDLY);
void DFSDM_EnableDelayClock(uint32_t MCHDLY);
void DFSDM_ClockIn_SourceSelection(uint32_t source);
void DFSDM_ClockOut_SourceSelection(uint32_t source);
void DFSDM_DataIn0_SourceSelection(uint32_t source);
void DFSDM_DataIn2_SourceSelection(uint32_t source);
void DFSDM_DataIn4_SourceSelection(uint32_t source);
void DFSDM_DataIn6_SourceSelection(uint32_t source);
void DFSDM1_BitStreamClk_Config(uint32_t source);
void DFSDM2_BitStreamClk_Config(uint32_t source);
#endif /* STM32F413_423xx */
#ifdef __cplusplus
}
#endif

#endif /*__STM32F4xx_SYSCFG_H */

/**
  * @}
  */ 

/**
  * @}
  */ 

