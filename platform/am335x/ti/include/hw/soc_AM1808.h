/** ============================================================================
 *   \file  soc_AM1808.h
 *
 *   \brief This file contains the peripheral information for AM1808 SOC
 *
 *  ============================================================================
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


#ifndef _SOC_AM1808_H_
#define _SOC_AM1808_H_

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************\
* Peripheral Instance count
\******************************************************************************/

/** @brief Number of UPP  instances                                           */
#define SOC_UPP_PER_CNT                     1

/** @brief Number of UHPI instances                                           */
#define SOC_HPI_PER_CNT                     1

/** @brief Number of McASP instances                                          */
#define SOC_MCASP_PER_CNT                   1

/** @brief Number of TIMER instances                                          */
#define SOC_TMR_PER_CNT                     4

/** @brief Number of PSC instances                                            */
#define SOC_PSC_PER_CNT                     2

/** @brief Number of UART instances                                           */
#define SOC_UART_PER_CNT                    3

/** @brief Number of SPI instances                                            */
#define SOC_SPI_PER_CNT                     2

/** @brief Number of I2C instances                                            */
#define SOC_I2C_PER_CNT                     2

/** @brief Number of PLL instances                                            */
#define SOC_PLLC_PER_CNT                    2

/** @brief Number of MMCSD instances                                          */
#define SOC_MMCSD_PER_CNT                   2

/** @brief Number of LCDC instances                                           */
#define SOC_LCDC_PER_CNT                    1

/** @brief Number of Mcbsp instances                                          */
#define SOC_MCBSP_PER_CNT                   2

/** @brief Number of EDMA3 CC instances                                       */
#define SOC_EDMA3CC_CNT                     2

/** @brief Number of EDMA3 TC instances                                       */
#define SOC_EDMA3TC_CNT                     3

/** @brief Number of EMIFA instances                                          */
#define SOC_EMIFA_PER_CNT                   1

/** @brief Number of EMIFB instances                                          */
#define SOC_EMIFB_PER_CNT                   1

/** @brief Number of EMAC instances                                           */
#define SOC_EMAC_PER_CNT                    1

/** @brief Number of MDIO instances                                           */
#define SOC_MDIO_PER_CNT                    1

/** @brief Number of EHRPWM instances                                         */
#define SOC_EHRPWM_PER_CNT                  2

/** @brief Number of ECAP instances                                           */
#define SOC_ECAP_PER_CNT                    3

/** @brief Number of CPGMAC instances                                         */
#define SOC_CPGMACSSR_PER_CNT               1

/** @brief Number of CPPI instances                                           */
#define SOC_CPPI_PER_CNT                    1

/** @brief Number of USB instances                                            */
#define SOC_USB_PER_CNT                     2

/** @brief Number of VPIF instances                                           */
#define SOC_VPIF_PER_CNT                    1

/** @brief Number of INTC instances                                           */
#define SOC_INTC_PER_CNT                    1

/** @brief Number of AINTC instances                                          */
#define SOC_AINTC_PER_CNT                   1

/** @brief Number of SATA instances                                           */
#define SOC_SATA_PER_CNT                    1

/** @brief Number of RTC instances                                            */
#define SOC_RTC_PER_CNT                     1

/** @brief Number of GPIO instances                                           */
#define SOC_GPIO_PER_CNT                    1

/** @brief Number of SYSCFG instances                                         */
#define SOC_SYSCFG_PER_CNT                  2
/******************************************************************************\
* Peripheral Instance definitions.
\******************************************************************************/

/** @brief Peripheral Instances of UHPI instances                             */
#define SOC_HPI                             (0)

/** @brief Peripheral Instances of McASP instances                            */
#define SOC_MCASP_0                         (0)

/** @brief Peripheral Instance of EDMA CC instances                           */
#define SOC_EDMA3CC_0                       (0)
#define SOC_EDMA3CC_1                       (1)

/** @brief Peripheral Instance of EDMA TC instances                           */
#define SOC_EDMA3TC_0                       (0)
#define SOC_EDMA3TC_1                       (1)

/** @brief Peripheral Instance of Timer 64 instances                          */
#define SOC_TMR_0                           (0)
#define SOC_TMR_1                           (1)
#define SOC_TMR_2                           (2)
#define SOC_TMR_3                           (3)

/** @brief Peripheral Instances of PSC instances                              */
#define SOC_PSC_0                           (0)
#define SOC_PSC_1                           (1)

/** @brief Peripheral Instances of UART instances                             */
#define SOC_UART_0                          (0)
#define SOC_UART_1                          (1)
#define SOC_UART_2                          (2)

/** @brief Peripheral Instances of SPI instances                              */
#define SOC_SPI_0                           (0)
#define SOC_SPI_1                           (1)

/** @brief Peripheral Instances of I2C instances                              */
#define SOC_I2C_0                           (0)
#define SOC_I2C_1                           (1)

/** @brief Peripheral Instances of MMCSD instances                            */
#define SOC_MMCSD_0                         (0)
#define SOC_MMCSD_1                         (1)

/** @brief Peripheral Instances of LCDC instances                              */
#define SOC_LCDC                            (0)

/** @brief Instance number of PLL controller                                  */
#define SOC_PLLC_0                          (0)
#define SOC_PLLC_1                          (1)

/** @brief Peripheral Instance of EMIFA instances                             */
#define SOC_EMIFA                           (0)

/** @brief Peripheral Instance of EMAC instances                              */
#define SOC_EMAC                            (0)

/** @brief Peripheral Instance of MDIO instances                              */
#define SOC_MDIO                            (0)

/** @brief Peripheral Instance of EHRPWM instances                            */
#define SOC_EHRPWM_0                        (0)
#define SOC_EHRPWM_1                        (1)

/** @brief Peripheral Instance of ECAP instances                              */
#define SOC_ECAP_0                          (0)
#define SOC_ECAP_1                          (1)
#define SOC_ECAP_2                          (2)

/** @brief Peripheral Instance of USB instances                               */
#define SOC_USB_0                           (0)
#define SOC_USB_1                           (1)

/** @brief Peripheral Instance of PRU CORE instances                          */
#define SOC_PRUCORE_0                       (0)
#define SOC_PRUCORE_1                       (1)

/** @brief Peripheral Instance of PRUINTC instances                           */
#define SOC_PRUINTC                         (0)

/** @brief Peripheral Instances of VPIF instances                            */
#define SOC_VPIF                            (0)

/** @brief Peripheral Instance of INTC instances                              */
#define SOC_INTC                            (0)

/** @brief Peripheral Instance of AINTC instances                             */
#define SOC_AINTC                           (0)

/** @brief Peripheral Instance of RTC instances                               */
#define SOC_RTC                             (0)

/** @brief Peripheral Instance of GPIO instances                              */
#define SOC_GPIO                            (0)
/** @brief GPIO pin and bank information                                      */
#define SOC_GPIO_NUM_PINS                   (144)
#define SOC_GPIO_NUM_BANKS                  ((SOC_GPIO_NUM_PINS + 15)/16)

/** @brief Peripheral Instance of ECTL instances                              */
#define SOC_ECTL                            (0)

/** @brief Peripheral Instance of SYSCFG instances                            */
#define SOC_SYSCFG                          (2)

/*******************************************************************************
* Peripheral Base Address
*******************************************************************************/

/** @brief Base address of INTC memory mapped registers                       */
#define SOC_INTC_0_REGS                     (0x01800000)

/** @brief Base address of PDC memory mapped registers                        */
#define SOC_PWRDWN_PDC_REGS                 (0x01810000)

/** @brief Base address of SYS - Security ID register                         */
#define SOC_SYS_0_SECURITY_ID_REGS          (0x01811000)

/** @brief Base address of SYS - Revision ID register                         */
#define SOC_SYS_0_REV_ID_REGS               (0x01812000)

/** #brief IDMA Module memory mapped address  */
#define SOC_IDMA_0_REGS                     (0x01820000)

/** #brief EMC Module memory mapped address                                   */
#define SOC_EMC_0_REGS                      (0x01820000)

/** #brief Cache Module memory mapped address                                 */
#define SOC_CACHE_0_REGS                    (0x01840000)

/** @brief Base address of Channel controller  memory mapped registers        */
#define SOC_EDMA30CC_0_REGS                 (0x01C00000)

/** @brief Base address of Transfer controller  memory mapped registers       */
#define SOC_EDMA30TC_0_REGS                 (0x01C08000)
#define SOC_EDMA30TC_1_REGS                 (0x01C08400)

/** @brief Base address of PSC memory mapped registers                        */
#define SOC_PSC_0_REGS                      (0x01C10000)

/** @brief PLL controller instance o module address                           */
#define SOC_PLLC_0_REGS                     (0x01C11000)

/** @brief Base address of DEV memory mapped registers                        */
#define SOC_SYSCFG_0_REGS                   (0x01C14000)

/** @brief Base address of TIMER memory mapped registers                      */
#define SOC_TMR_0_REGS                      (0x01C20000)
#define SOC_TMR_1_REGS                      (0x01C21000)

/** @brief Base address of I2C memory mapped registers                        */
#define SOC_I2C_0_REGS                      (0x01C22000)

/** @brief Base address of RTC memory                                         */
#define SOC_RTC_0_REGS                      (0x01C23000)

/** @brief Base address of MMCSD memory mapped registers                      */
#define SOC_MMCSD_0_REGS                    (0x01C40000)

/** @brief Base address of SPI memory mapped registers                        */
#define SOC_SPI_0_REGS                      (0x01C41000)

/** @brief Base address of UART memory mapped registers                       */
#define SOC_UART_0_REGS                     (0x01C42000)

/** @brief Base address of McASP memory mapped registers                      */
#define SOC_MCASP_0_CTRL_REGS               (0x01D00000)
#define SOC_MCASP_0_FIFO_REGS               (0x01D01000)
#define SOC_MCASP_0_DATA_REGS               (0x01D02000)

/** @brief Base address of UART memory mapped registers                       */
#define SOC_UART_1_REGS                     (0x01D0C000)
#define SOC_UART_2_REGS                     (0x01D0D000)

/** @brief Base address of McBSP memory mapped registers                      */
#define SOC_MCBSP_0_CTRL_REGS               (0x01D10000)
#define SOC_MCBSP_0_FIFO_REGS               (0x01D10800)
#define SOC_MCBSP_0_DATA_REGS               (0x01F10000)

/** @brief Base address of McASP memory mapped registers                      */
#define SOC_MCBSP_1_CTRL_REGS               (0x01D11000)
#define SOC_MCBSP_1_FIFO_REGS               (0x01D11800)
#define SOC_MCBSP_1_DATA_REGS               (0x01F11000)

#define SOC_MPU_0_REGS                      (0x01E14000)
#define SOC_MPU_1_REGS                      (0x01E15000)

/** @brief Base address of USB memory                                         */
#define SOC_USB_0_REGS                      (0x01E00000)
#define SOC_USB_1_REGS                      (0x01E25000)

/** @brief Base address of HPI memory mapped registers                        */
#define SOC_HPI_0_REGS                      (0x01E10000)

/** @brief Base address of LCDC memory mapped registers                       */
#define SOC_LCDC_0_REGS                     (0x01E13000)

/** @brief Base address of UPP memory mapped registers                        */
#define SOC_UPP_0_REGS                      (0x01E16000)

/** @brief Base address of VPIF memory mapped registers                       */
#define SOC_VPIF_0_REGS                     (0x01E17000)

/** @brief Base address of SATA memory mapped registers                       */
#define SOC_SATA_0_REGS                     (0x01E18000)

/** @brief PLL controller instance 1 module address                           */
#define SOC_PLLC_1_REGS                     (0X01E1A000)

/** @brief Base address of MMCSD memory mapped registers                      */
#define SOC_MMCSD_1_REGS                    (0x01E1B000)

/** @brief Base address of EMAC memory                                        */
#define SOC_EMAC_DSC_CTRL_MOD_RAM           (0x01E20000)
#define SOC_EMAC_DSC_CTRL_MOD_REG           (0x01E22000)
#define SOC_EMAC_DSC_CONTROL_REG            (0x01E23000)
#define SOC_MDIO_0_REGS                     (0x01E24000)

/** @brief Base address of PRU Core Regsiters                                 */
#define SOC_PRUCORE_0_REGS                  (0x01C37000)
#define SOC_PRUCORE_1_REGS                  (0x01C37800)

/** @brief Base address of PRU Interrupt Controller Registers                 */
#define SOC_PRUINTC_0_REGS                  (0x01C34000)

/** @brief Base address of MUSB memmory mapped Registers                 */
#define SOC_USB_0_BASE              		(0x01E00400)

/** @brief Base address of  OTG  memmory mapped Registers                 */
#define SOC_USB_0_OTG_BASE             		(0x01E00000)

/** *brief USB 0 Phy regsister( CFGCHIP2 register) address				*/
#define SOC_USB_0_PHY_REGS					(0x01C14184)

/** @brief Base address of GPIO memory mapped registers                       */
#define SOC_GPIO_0_REGS                     (0x01E26000)

/** @brief Base address of PSC memory mapped registers                        */
#define SOC_PSC_1_REGS                      (0x01E27000)

/** @brief Base address of I2C memory mapped registers                        */
#define SOC_I2C_1_REGS                      (0x01E28000)

/** @brief Base address of syscfg registers                                   */
#define SOC_SYSCFG_1_REGS                   (0x01E2C000)

/** @brief Base address of Channel controller  memory mapped registers        */
#define SOC_EDMA31CC_0_REGS                 (0x01E30000)

/** @brief Base address of Transfer controller  memory mapped registers       */
#define SOC_EDMA31TC_0_REGS                 (0x01E38000)

/** @brief Base address of EPWM memory mapped registers                       */
#define SOC_EHRPWM_0_REGS                   (0x01F00000)
#define SOC_EHRPWM_1_REGS                   (0x01F02000)

/** @brief Base address of EPWM memory mapped registers                       */
#define SOC_HRPWM_0_REGS                    (0x01F01000)
#define SOC_HRPWM_1_REGS                    (0x01F03000)

/** @brief Base address of ECAP memory mapped registers                       */
#define SOC_ECAP_0_REGS                     (0x01F06000)
#define SOC_ECAP_1_REGS                     (0x01F07000)
#define SOC_ECAP_2_REGS                     (0x01F08000)

/** @brief Base address of TIMER memory mapped registers                      */
#define SOC_TMR_2_REGS                      (0x01F0C000)
#define SOC_TMR_3_REGS                      (0x01F0D000)

/** @brief Base address of SPI memory mapped registers                        */
#define SOC_SPI_1_REGS                      (0x01F0E000)

/** @brief Base address of EMIFA memory mapped registers                      */
#define SOC_EMIFA_0_REGS                    (0x68000000)

/** @brief Base address of EMIFA_CS0 memory                                   */
#define SOC_EMIFA_CS0_ADDR                  (0x40000000)

/** @brief Base address of EMIFA_CS2 memory                                   */
#define SOC_EMIFA_CS2_ADDR                  (0x60000000)

/** @brief Base address of EMIFA_CS3 memory                                   */
#define SOC_EMIFA_CS3_ADDR                  (0x62000000)

/** @brief Base address of EMIFA_CS4 memory                                   */
#define SOC_EMIFA_CS4_ADDR                  (0x64000000)

/** @brief Base address of EMIFA_CS5 memory                                   */
#define SOC_EMIFA_CS5_ADDR                  (0x66000000)

/** @brief Base address of DDR memory mapped registers                        */
#define SOC_DDR2_0_CTRL_REGS                (0xB0000000)
#define SOC_DDR2_0_DATA_REGS                (0xC0000000)

/** @brief Base address of AINTC memory mapped registers                      */
#define SOC_AINTC_0_REGS                    (0xFFFEE000)

/** @brief Base address of UMC Memory protection registers                    */
#define SOC_MEMPROT_L2_REGS                 (0x00800000)

/** @brief Base address of PMC memory Protection registers                    */
#define SOC_MEMPROT_L1P_REGS                (0x00E00000)

/** @brief Base address of DMC memory protection registers                    */
#define SOC_MEMPROT_L1D_REGS                (0x00F00000)

/*************************** EDMA RELATED DEFINES  ****************************/

/******************************************************************************\
* Channel Instance count
\******************************************************************************/
#define SOC_EDMA3_CHA_CNT                   (SOC_EDMA3_NUM_DMACH + \
                                             SOC_EDMA3_NUM_QDMACH)


/* QDMA channels                                                              */
#define    SOC_EDMA3_QCHA_BASE              SOC_EDMA3_NUM_DMACH         /* QDMA Channel Base */
#define    SOC_EDMA3_QCHA_0                 (SOC_EDMA3_QCHA_BASE + 0)   /* QDMA Channel 0 */
#define    SOC_EDMA3_QCHA_1                 (SOC_EDMA3_QCHA_BASE + 1)   /* QDMA Channel 1 */
#define    SOC_EDMA3_QCHA_2                 (SOC_EDMA3_QCHA_BASE + 2)   /* QDMA Channel 2 */
#define    SOC_EDMA3_QCHA_3                 (SOC_EDMA3_QCHA_BASE + 3)   /* QDMA Channel 3 */
#define    SOC_EDMA3_QCHA_4                 (SOC_EDMA3_QCHA_BASE + 4)   /* QDMA Channel 4 */
#define    SOC_EDMA3_QCHA_5                 (SOC_EDMA3_QCHA_BASE + 5)   /* QDMA Channel 5 */
#define    SOC_EDMA3_QCHA_6                 (SOC_EDMA3_QCHA_BASE + 6)   /* QDMA Channel 6 */
#define    SOC_EDMA3_QCHA_7                 (SOC_EDMA3_QCHA_BASE + 7)   /* QDMA Channel 7 */


/* Enumerations for EDMA Controlleres */
#define  SOC_EDMACC_ANY       -1 /* Any instance of EDMACC module*/
#define  SOC_EDMACC_0          0 /* EDMACC Instance 0            */


/* Enumerations for EDMA Event Queues */
#define  SOC_EDMA3_QUE_0     0 /* Queue 0                     */
#define  SOC_EDMA3_QUE_1     1 /* Queue 1                     */

/* Enumerations for EDMA Transfer Controllers
 *
 * There are 2 Transfer Controllers. Typically a one to one mapping exists
 * between Event Queues and Transfer Controllers.
 *
 */
#define  SOC_EDMATC_ANY  -1 /* Any instance of EDMATC                */
#define  SOC_EDMATC_0    0  /* EDMATC Instance 0                     */
#define  SOC_EDMATC_1    1  /* EDMATC Instance 1                     */


#define SOC_EDMA3_REGION_GLOBAL            (-1)
#define SOC_EDMA3_REGION_0                  0
#define SOC_EDMA3_REGION_1                  1
#define SOC_EDMA3_REGION_2                  2
#define SOC_EDMA3_REGION_3                  3



/******************************* DAT RELATED DEFINES **************************/

/******************************************************************************\
*  Parameterizable Configuration:- These are fed directly from the RTL
*  parameters for the given SOC
\******************************************************************************/
/******************************************************************************\
* Channel Instance count
\******************************************************************************/
/** @brief Number of Generic Channel instances                                */


/** @brief Enumerations for EDMA channels
*
*  There are 8 QDMA channels -
*
*/

#define    SOC_DAT_QCHA_0                   0 /**<   QDMA Channel 0           */
#define    SOC_DAT_QCHA_1                   1 /**<   QDMA Channel 1           */
#define    SOC_DAT_QCHA_2                   2 /**<   QDMA Channel 2           */
#define    SOC_DAT_QCHA_3                   3 /**<   QDMA Channel 3           */
#define    SOC_DAT_QCHA_4                   4 /**<   QDMA Channel 4           */
#define    SOC_DAT_QCHA_5                   5 /**<   QDMA Channel 5           */
#define    SOC_DAT_QCHA_6                   6 /**<   QDMA Channel 6           */
#define    SOC_DAT_QCHA_7                   7 /**<   QDMA Channel 7           */

/** @brief Enumerations for EDMA Event Queues
*
*  There are two Event Queues. Q0 is the highest priority and Q1 is the least
*  priority
*
*/
#define    SOC_DAT_PRI_DEFAULT    0 /* Queue 0 is default          */
#define    SOC_DAT_PRI_0          0 /* Queue 0                     */
#define    SOC_DAT_PRI_1          1 /* Queue 1                     */

/** @brief Enumeration for EDMA Regions
*
*
*/

#define SOC_DAT_REGION_GLOBAL              (-1)       /* Global Region        */
#define SOC_DAT_REGION_0                    0         /* EDMA Region 0        */
#define SOC_DAT_REGION_1                    1         /* EDMA Region 1        */
#define SOC_DAT_REGION_2                    2         /* EDMA Region 2        */
#define SOC_DAT_REGION_3                    3         /* EDMA Region 3        */

/** @brief Enumeration for peripheral frequencies
*
*
*/

#define SOC_SYSCLK_1_FREQ                   (300000000)
#define SOC_SYSCLK_2_FREQ                   (SOC_SYSCLK_1_FREQ/2)
#define SOC_SYSCLK_3_FREQ                   (SOC_SYSCLK_1_FREQ/3)
#define SOC_SYSCLK_4_FREQ                   (SOC_SYSCLK_1_FREQ/4)

#define SOC_ASYNC_2_FREQ                    (24000000)

/** I2C      */
#define SOC_I2C_0_MODULE_FREQ               (SOC_ASYNC_2_FREQ)
#define SOC_I2C_1_MODULE_FREQ               (SOC_SYSCLK_4_FREQ)

/** MCBSP    */
#define SOC_MCBSP_0_MODULE_FREQ             (SOC_SYSCLK_2_FREQ)
#define SOC_MCBSP_1_MODULE_FREQ             (SOC_SYSCLK_2_FREQ)

/** LCDC     */
#define SOC_LCDC_0_MODULE_FREQ              (SOC_SYSCLK_2_FREQ)

/** SPI      */
#define SOC_SPI_0_MODULE_FREQ               (SOC_SYSCLK_2_FREQ)
#define SOC_SPI_1_MODULE_FREQ               (SOC_SYSCLK_2_FREQ)

/** UART     */
#define SOC_UART_0_MODULE_FREQ              (SOC_SYSCLK_2_FREQ)
#define SOC_UART_1_MODULE_FREQ              (SOC_SYSCLK_2_FREQ)
#define SOC_UART_2_MODULE_FREQ              (SOC_SYSCLK_2_FREQ)

/** EHRPWM	*/
#define SOC_EHRPWM_0_MODULE_FREQ	    (SOC_SYSCLK_2_FREQ)
#define SOC_EHRPWM_1_MODULE_FREQ	    (SOC_SYSCLK_2_FREQ)

#ifdef __cplusplus
}
#endif

#endif  /* _SOC_AM1808_H_ */
