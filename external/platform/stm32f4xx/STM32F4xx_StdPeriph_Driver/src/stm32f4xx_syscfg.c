/**
  ******************************************************************************
  * @file    stm32f4xx_syscfg.c
  * @author  MCD Application Team
  * @version V1.8.1
  * @date    27-January-2022
  * @brief   This file provides firmware functions to manage the SYSCFG peripheral.
  *
 @verbatim
    
 ===============================================================================
                     ##### How to use this driver #####
 ===============================================================================
    [..] This driver provides functions for:
            
       (#) Remapping the memory accessible in the code area using SYSCFG_MemoryRemapConfig()
            
       (#) Swapping the internal flash Bank1 and Bank2 this features is only visible for 
           STM32F42xxx/43xxx devices Devices. 
                
       (#) Manage the EXTI lines connection to the GPIOs using SYSCFG_EXTILineConfig()
              
       (#) Select the ETHERNET media interface (RMII/RII) using SYSCFG_ETH_MediaInterfaceConfig()
  
       -@- SYSCFG APB clock must be enabled to get write access to SYSCFG registers,
           using RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
                   
 @endverbatim      
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

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_syscfg.h"
#include "stm32f4xx_rcc.h"

/** @addtogroup STM32F4xx_StdPeriph_Driver
  * @{
  */

/** @defgroup SYSCFG 
  * @brief SYSCFG driver modules
  * @{
  */ 

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* ------------ RCC registers bit address in the alias region ----------- */
#define SYSCFG_OFFSET             (SYSCFG_BASE - PERIPH_BASE)
/* ---  MEMRMP Register ---*/ 
/* Alias word address of UFB_MODE bit */ 
#define MEMRMP_OFFSET             SYSCFG_OFFSET 
#define UFB_MODE_BitNumber        ((uint8_t)0x8) 
#define UFB_MODE_BB               (PERIPH_BB_BASE + (MEMRMP_OFFSET * 32) + (UFB_MODE_BitNumber * 4)) 
    
/* ---  PMC Register ---*/ 
/* Alias word address of MII_RMII_SEL bit */ 
#define PMC_OFFSET                (SYSCFG_OFFSET + 0x04) 
#define MII_RMII_SEL_BitNumber    ((uint8_t)0x17) 
#define PMC_MII_RMII_SEL_BB       (PERIPH_BB_BASE + (PMC_OFFSET * 32) + (MII_RMII_SEL_BitNumber * 4)) 
    
/* ---  CMPCR Register ---*/ 
/* Alias word address of CMP_PD bit */ 
#define CMPCR_OFFSET              (SYSCFG_OFFSET + 0x20) 
#define CMP_PD_BitNumber          ((uint8_t)0x00) 
#define CMPCR_CMP_PD_BB           (PERIPH_BB_BASE + (CMPCR_OFFSET * 32) + (CMP_PD_BitNumber * 4)) 

/* ---  MCHDLYCR Register ---*/ 
/* Alias word address of BSCKSEL bit */ 
#define MCHDLYCR_OFFSET            (SYSCFG_OFFSET + 0x30) 
#define BSCKSEL_BIT_NUMBER         POSITION_VAL(SYSCFG_MCHDLYCR_BSCKSEL)
#define MCHDLYCR_BSCKSEL_BB        (uint32_t)(PERIPH_BB_BASE + (MCHDLYCR_OFFSET * 32) + (BSCKSEL_BIT_NUMBER * 4))

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/** @defgroup SYSCFG_Private_Functions
  * @{
  */ 

/**
  * @brief  Deinitializes the Alternate Functions (remap and EXTI configuration)
  *   registers to their default reset values.
  * @param  None
  * @retval None
  */
void SYSCFG_DeInit(void)
{
   RCC_APB2PeriphResetCmd(RCC_APB2Periph_SYSCFG, ENABLE);
   RCC_APB2PeriphResetCmd(RCC_APB2Periph_SYSCFG, DISABLE);
}

/**
  * @brief  Changes the mapping of the specified pin.
  * @param  SYSCFG_Memory: selects the memory remapping.
  *         This parameter can be one of the following values:
  *            @arg SYSCFG_MemoryRemap_Flash:       Main Flash memory mapped at 0x00000000  
  *            @arg SYSCFG_MemoryRemap_SystemFlash: System Flash memory mapped at 0x00000000
  *            @arg SYSCFG_MemoryRemap_FSMC:        FSMC (Bank1 (NOR/PSRAM 1 and 2) mapped at 0x00000000 for STM32F405xx/407xx, STM32F415xx/417xx and STM32F413_423xx devices. 
  *            @arg SYSCFG_MemoryRemap_FMC:         FMC (Bank1 (NOR/PSRAM 1 and 2) mapped at 0x00000000 for STM32F42xxx/43xxx devices. 
  *            @arg SYSCFG_MemoryRemap_ExtMEM:      External Memory mapped at 0x00000000 for STM32F446xx/STM32F469_479xx devices. 
  *            @arg SYSCFG_MemoryRemap_SRAM:        Embedded SRAM (112kB) mapped at 0x00000000
  *            @arg SYSCFG_MemoryRemap_SDRAM:       FMC (External SDRAM)  mapped at 0x00000000 for STM32F42xxx/43xxx devices.            
  * @retval None
  */
void SYSCFG_MemoryRemapConfig(uint8_t SYSCFG_MemoryRemap)
{
  /* Check the parameters */
  assert_param(IS_SYSCFG_MEMORY_REMAP_CONFING(SYSCFG_MemoryRemap));

  SYSCFG->MEMRMP = SYSCFG_MemoryRemap;
}

/**
  * @brief  Enables or disables the Internal FLASH Bank Swapping.
  *   
  * @note   This function can be used only for STM32F42xxx/43xxx devices. 
  *
  * @param  NewState: new state of Internal FLASH Bank swapping.
  *          This parameter can be one of the following values:
  *            @arg ENABLE: Flash Bank2 mapped at 0x08000000 (and aliased @0x00000000) 
  *                         and Flash Bank1 mapped at 0x08100000 (and aliased at 0x00100000)   
  *            @arg DISABLE:(the default state) Flash Bank1 mapped at 0x08000000 (and aliased @0x0000 0000) 
                            and Flash Bank2 mapped at 0x08100000 (and aliased at 0x00100000)  
  * @retval None
  */
void SYSCFG_MemorySwappingBank(FunctionalState NewState)
{
  /* Check the parameters */
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  *(__IO uint32_t *) UFB_MODE_BB = (uint32_t)NewState;
}

/**
  * @brief  Selects the GPIO pin used as EXTI Line.
  * @param  EXTI_PortSourceGPIOx : selects the GPIO port to be used as source for
  *          EXTI lines where x can be (A..K) for STM32F42xxx/43xxx devices, (A..I) 
  *          for STM32F405xx/407xx and STM32F415xx/417xx devices or (A, B, C, D and H)
  *          for STM32401xx devices.  
  *            
  * @param  EXTI_PinSourcex: specifies the EXTI line to be configured.
  *           This parameter can be EXTI_PinSourcex where x can be (0..15, except
  *           for EXTI_PortSourceGPIOI x can be (0..11) for STM32F405xx/407xx
  *           and STM32F405xx/407xx devices and for EXTI_PortSourceGPIOK x can   
  *           be (0..7) for STM32F42xxx/43xxx devices. 
  *             
  * @retval None
  */
void SYSCFG_EXTILineConfig(uint8_t EXTI_PortSourceGPIOx, uint8_t EXTI_PinSourcex)
{
  uint32_t tmp = 0x00;

  /* Check the parameters */
  assert_param(IS_EXTI_PORT_SOURCE(EXTI_PortSourceGPIOx));
  assert_param(IS_EXTI_PIN_SOURCE(EXTI_PinSourcex));

  tmp = ((uint32_t)0x0F) << (0x04 * (EXTI_PinSourcex & (uint8_t)0x03));
  SYSCFG->EXTICR[EXTI_PinSourcex >> 0x02] &= ~tmp;
  SYSCFG->EXTICR[EXTI_PinSourcex >> 0x02] |= (((uint32_t)EXTI_PortSourceGPIOx) << (0x04 * (EXTI_PinSourcex & (uint8_t)0x03)));
}

/**
  * @brief  Selects the ETHERNET media interface 
  * @param  SYSCFG_ETH_MediaInterface: specifies the Media Interface mode. 
  *          This parameter can be one of the following values: 
  *            @arg SYSCFG_ETH_MediaInterface_MII: MII mode selected
  *            @arg SYSCFG_ETH_MediaInterface_RMII: RMII mode selected 
  * @retval None 
  */
void SYSCFG_ETH_MediaInterfaceConfig(uint32_t SYSCFG_ETH_MediaInterface) 
{ 
  assert_param(IS_SYSCFG_ETH_MEDIA_INTERFACE(SYSCFG_ETH_MediaInterface)); 
  /* Configure MII_RMII selection bit */ 
  *(__IO uint32_t *) PMC_MII_RMII_SEL_BB = SYSCFG_ETH_MediaInterface; 
}

/**
  * @brief  Enables or disables the I/O Compensation Cell.
  * @note   The I/O compensation cell can be used only when the device supply
  *         voltage ranges from 2.4 to 3.6 V.  
  * @param  NewState: new state of the I/O Compensation Cell.
  *          This parameter can be one of the following values:
  *            @arg ENABLE: I/O compensation cell enabled  
  *            @arg DISABLE: I/O compensation cell power-down mode  
  * @retval None
  */
void SYSCFG_CompensationCellCmd(FunctionalState NewState)
{
  /* Check the parameters */
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  *(__IO uint32_t *) CMPCR_CMP_PD_BB = (uint32_t)NewState;
}

/**
  * @brief  Checks whether the I/O Compensation Cell ready flag is set or not.
  * @param  None
  * @retval The new state of the I/O Compensation Cell ready flag (SET or RESET)
  */
FlagStatus SYSCFG_GetCompensationCellStatus(void)
{
  FlagStatus bitstatus = RESET;
    
  if ((SYSCFG->CMPCR & SYSCFG_CMPCR_READY ) != (uint32_t)RESET)
  {
    bitstatus = SET;
  }
  else
  {
    bitstatus = RESET;
  }
  return bitstatus;
}

#if defined(STM32F410xx) || defined(STM32F412xG) || defined(STM32F413_423xx)
/**
  * @brief  Connects the selected parameter to the break input of TIM1.
  * @note   The selected configuration is locked and can be unlocked by system reset
  * @param  SYSCFG_Break: selects the configuration to be connected to break
  *         input of TIM1
  *   This parameter can be any combination of the following values:
  *     @arg SYSCFG_Break_PVD: PVD interrupt is connected to the break input of TIM1/8.
  *     @arg SYSCFG_Break_HardFault: Lockup output of CortexM4 is connected to the break input of TIM1/8.
  * @retval None
  */
void SYSCFG_BreakConfig(uint32_t SYSCFG_Break)
{
  /* Check the parameter */
  assert_param(IS_SYSCFG_LOCK_CONFIG(SYSCFG_Break));

  SYSCFG->CFGR2 |= (uint32_t) SYSCFG_Break;
}
#endif /* STM32F410xx || STM32F412xG || STM32F413_423xx */

#if defined(STM32F413_423xx)
/**
  * @brief  Select the DFSDM2 or TIM2_OC1 as clock source for the bitstream clock.
  * @param source: BITSTREAM_CLOCK_DFSDM2.
  *                BITSTREAM_CLOCK_TIM2OC1.           
  * @retval None
  */
void DFSDM_BitstreamClock_SourceSelection(uint32_t source)
{
  uint32_t tmp = 0; 
  
  tmp = SYSCFG->MCHDLYCR;
  tmp = (tmp &(~SYSCFG_MCHDLYCR_BSCKSEL));

  SYSCFG->MCHDLYCR  = (tmp|source);
}

/**
  * @brief  Disable Delay Clock for DFSDM1/2.
  * @param MCHDLY: MCHDLY_CLOCK_DFSDM2.
  *                MCHDLY_CLOCK_DFSDM1.               
  * @retval None
  */
void DFSDM_DisableDelayClock(uint32_t MCHDLY)
{
  uint32_t tmp = 0; 
  
  tmp = SYSCFG->MCHDLYCR;
  if(MCHDLY == MCHDLY_CLOCK_DFSDM2)
  {
    tmp =tmp &(~SYSCFG_MCHDLYCR_MCHDLY2EN);
  }
  else
  {
    tmp =tmp &(~SYSCFG_MCHDLYCR_MCHDLY1EN);
  }

  SYSCFG->MCHDLYCR  = tmp;
}

/**
  * @brief  Enable Delay Clock for DFSDM1/2.
  * @param MCHDLY: MCHDLY_CLOCK_DFSDM2.
  *                MCHDLY_CLOCK_DFSDM1.         
  * @retval None
  */
void DFSDM_EnableDelayClock(uint32_t MCHDLY)
{
  uint32_t tmp = 0; 
  
  tmp = SYSCFG->MCHDLYCR;
  tmp = tmp & ~MCHDLY;

  SYSCFG->MCHDLYCR  = (tmp|MCHDLY);
}

/**
  * @brief  Select the source for CKin signals for DFSDM1/2.
  * @param source: DFSDM2_CKIN_PAD.
  *                DFSDM2_CKIN_DM. 
  *                DFSDM1_CKIN_PAD.
  *                DFSDM1_CKIN_DM.            
  * @retval None
  */
void DFSDM_ClockIn_SourceSelection(uint32_t source)
{
  uint32_t tmp = 0; 
  
  tmp = SYSCFG->MCHDLYCR;
  if((source == DFSDM2_CKIN_PAD) || (source == DFSDM2_CKIN_DM))
  {
    tmp =  (tmp & ~SYSCFG_MCHDLYCR_DFSDM2CFG);
  }
  else
  {
    tmp =  (tmp & ~SYSCFG_MCHDLYCR_DFSDM1CFG);
  }

  SYSCFG->MCHDLYCR |= (source|tmp);
}

/**
  * @brief  Select the source for CKOut signals for DFSDM1/2.
  * @param source: DFSDM2_CKOUT_DFSDM2.
  *                DFSDM2_CKOUT_M27. 
  *                DFSDM1_CKOUT_DFSDM1.
  *                DFSDM1_CKOUT_M27.            
  * @retval None
  */
void DFSDM_ClockOut_SourceSelection(uint32_t source)
{
  uint32_t tmp = 0; 
  
  tmp = SYSCFG->MCHDLYCR;

  if((source == DFSDM2_CKOUT_DFSDM2) || (source == DFSDM2_CKOUT_M27))
  {
    tmp =  (tmp & ~SYSCFG_MCHDLYCR_DFSDM2CKOSEL);
  }
  else
  {
    tmp =  (tmp & ~SYSCFG_MCHDLYCR_DFSDM1CKOSEL);
  }

  SYSCFG->MCHDLYCR |= (source|tmp);
}

/**
  * @brief  Select the source for DataIn0 signals for DFSDM1/2.
  * @param source: DATAIN0_DFSDM2_PAD.
  *                DATAIN0_DFSDM2_DATAIN1. 
  *                DATAIN0_DFSDM1_PAD.
  *                DATAIN0_DFSDM1_DATAIN1.                  
  * @retval None
  */
void DFSDM_DataIn0_SourceSelection(uint32_t source)
{
  uint32_t tmp = 0; 
  
  tmp = SYSCFG->MCHDLYCR;
  
  if((source == DATAIN0_DFSDM2_PAD)|| (source == DATAIN0_DFSDM2_DATAIN1))
  {
    tmp =  (tmp & ~SYSCFG_MCHDLYCR_DFSDM2D0SEL);
  }
  else
  {
    tmp =  (tmp & ~SYSCFG_MCHDLYCR_DFSDM1D0SEL);
  }
  SYSCFG->MCHDLYCR |= (source|tmp);
}

/**
  * @brief  Select the source for DataIn2 signals for DFSDM1/2.
  * @param source: DATAIN2_DFSDM2_PAD.
  *                DATAIN2_DFSDM2_DATAIN3. 
  *                DATAIN2_DFSDM1_PAD.
  *                DATAIN2_DFSDM1_DATAIN3.                  
  * @retval None
  */
void DFSDM_DataIn2_SourceSelection(uint32_t source)
{
  uint32_t tmp = 0; 
  
  tmp = SYSCFG->MCHDLYCR;
  
  if((source == DATAIN2_DFSDM2_PAD)|| (source == DATAIN2_DFSDM2_DATAIN3))
  {
    tmp =  (tmp & ~SYSCFG_MCHDLYCR_DFSDM2D2SEL);
  }
  else
  {
    tmp =  (tmp & ~SYSCFG_MCHDLYCR_DFSDM1D2SEL);
  }
  SYSCFG->MCHDLYCR |= (source|tmp);
}

/**
  * @brief  Select the source for DataIn4 signals for DFSDM2.
  * @param source: DATAIN4_DFSDM2_PAD.
  *                DATAIN4_DFSDM2_DATAIN5                
  * @retval None
  */
void DFSDM_DataIn4_SourceSelection(uint32_t source)
{
  uint32_t tmp = 0; 
  
  tmp = SYSCFG->MCHDLYCR;
  tmp =  (tmp & ~SYSCFG_MCHDLYCR_DFSDM2D4SEL);

  SYSCFG->MCHDLYCR |= (source|tmp);
}

/**
  * @brief  Select the source for DataIn6 signals for DFSDM2.
  * @param source: DATAIN6_DFSDM2_PAD.
  *                DATAIN6_DFSDM2_DATAIN7.                  
  * @retval None
  */
void DFSDM_DataIn6_SourceSelection(uint32_t source)
{
  uint32_t tmp = 0; 
  
  tmp = SYSCFG->MCHDLYCR;
  
  tmp =  (tmp & ~SYSCFG_MCHDLYCR_DFSDM2D6SEL);

  SYSCFG->MCHDLYCR |= (source|tmp);
}

/**
  * @brief  Configure the distribution of the bitstream clock gated from TIM4.
  * @param source: DFSDM1_CLKIN0_TIM4OC2
  *                DFSDM1_CLKIN2_TIM4OC2
  *                DFSDM1_CLKIN1_TIM4OC1
  *                DFSDM1_CLKIN3_TIM4OC1                         
  * @retval None
  */
void DFSDM1_BitStreamClk_Config(uint32_t source)
{
  uint32_t tmp = 0; 
  
  tmp = SYSCFG->MCHDLYCR;

  if ((source == DFSDM1_CLKIN0_TIM4OC2) || (source == DFSDM1_CLKIN2_TIM4OC2))
  {
    tmp =  (tmp & ~SYSCFG_MCHDLYCR_DFSDM1CK02SEL);
  }
  else
  {
    tmp =  (tmp & ~SYSCFG_MCHDLYCR_DFSDM1CK13SEL);
  }

  SYSCFG->MCHDLYCR |= (source|tmp);
}

/**
  * @brief  Configure the distribution of the bitstream clock gated from TIM3.
  * @param source: DFSDM2_CLKIN0_TIM3OC4
  *                DFSDM2_CLKIN4_TIM3OC4
  *                DFSDM2_CLKIN1_TIM3OC3
  *                DFSDM2_CLKIN5_TIM3OC3 
  *                DFSDM2_CLKIN2_TIM3OC2
  *                DFSDM2_CLKIN6_TIM3OC2
  *                DFSDM2_CLKIN3_TIM3OC1
  *                DFSDM2_CLKIN7_TIM3OC1                        
  * @retval None
  */
void DFSDM2_BitStreamClk_Config(uint32_t source)
{
  uint32_t tmp = 0; 
  
  tmp = SYSCFG->MCHDLYCR;

  if ((source == DFSDM2_CLKIN0_TIM3OC4) || (source == DFSDM2_CLKIN4_TIM3OC4))
  {
    tmp =  (tmp & ~SYSCFG_MCHDLYCR_DFSDM2CK04SEL);
  }
  else if ((source == DFSDM2_CLKIN1_TIM3OC3) || (source == DFSDM2_CLKIN5_TIM3OC3))
  {
    tmp =  (tmp & ~SYSCFG_MCHDLYCR_DFSDM2CK15SEL);
    
  }else  if ((source == DFSDM2_CLKIN2_TIM3OC2) || (source == DFSDM2_CLKIN6_TIM3OC2))
  {
    tmp =  (tmp & ~SYSCFG_MCHDLYCR_DFSDM2CK26SEL);
  }
  else
  {
    tmp =  (tmp & ~SYSCFG_MCHDLYCR_DFSDM2CK37SEL);
  }

  SYSCFG->MCHDLYCR |= (source|tmp);
}

#endif /* STM32F413_423xx */
/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

