/** ============================================================================
 *   \file  soc_C6A811x.h
 *
 *   \brief This file contains the peripheral information for C6A811x SoC
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


#ifndef _SOC_C6A811X_H_
#define _SOC_C6A811X_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Base address of AINTC memory mapped registers                      */
#define SOC_AINTC_REGS                       (0x48200000)

/** @brief Base addresses of UART memory mapped registers                     */
#define SOC_UART_0_REGS                      (0x48020000)
#define SOC_UART_1_REGS                      (0x48022000)
#define SOC_UART_2_REGS                      (0x48024000)
#define SOC_UART_3_REGS                      (0x481A6000)
#define SOC_UART_4_REGS                      (0x481A8000)
#define SOC_UART_5_REGS                      (0x481AA000)
#define SOC_UART_6_REGS                      (0x48440000)
#define SOC_UART_7_REGS                      (0x48442000)

/** @brief Base addresses of USB memory mapped registers                      */
#define SOC_USB_0_BASE                       (0x47401400)
#define SOC_USB_1_BASE                       (0x47401C00)

/** @brief Base addresses of SPI memory mapped registers                      */
#define SOC_SPI_0_REGS                       (0x48030000)
#define SOC_SPI_1_REGS                       (0x481A0000)
#define SOC_SPI_2_REGS                       (0x481A2000)
#define SOC_SPI_3_REGS                       (0x481A4000)

/** @brief Base addresses of GPIO memory mapped registers                     */
#define SOC_GPIO_0_REGS                      (0x48032000)
#define SOC_GPIO_1_REGS                      (0x4804C000)
#define SOC_GPIO_2_REGS                      (0x481AC000)
#define SOC_GPIO_3_REGS                      (0x481AE000)
#define SOC_GPIO_4_REGS                      (0x48420000)
#define SOC_GPIO_5_REGS                      (0x48422000)

/** @brief Base addresses of DMTIMER memory mapped registers                  */
#define SOC_DMTIMER_0_REGS                   (0x4802C000)
#define SOC_DMTIMER_1_REGS                   (0x4802E000)
#define SOC_DMTIMER_2_REGS                   (0x48040000)
#define SOC_DMTIMER_3_REGS                   (0x48042000)
#define SOC_DMTIMER_4_REGS                   (0x48044000)
#define SOC_DMTIMER_5_REGS                   (0x48046000)
#define SOC_DMTIMER_6_REGS                   (0x48048000)
#define SOC_DMTIMER_7_REGS                   (0x4804A000)
#define SOC_DMTIMER_8_REGS                   (0x481C1000)

/** @brief Base address of MMC memory mapped registers                        */
#define SOC_MMCHS_0_REGS                     (0x48060000)
#define SOC_MMCHS_1_REGS                     (0x481D8000)
#define SOC_MMCHS_2_REGS                     (0x47810000)

/** @brief Base address of GPMC memory mapped registers                       */
#define SOC_GPMC_0_REGS                      (0x50000000)

/** @brief Base address of GPMC memory mapped registers                       */
#define SOC_ELM_0_REGS                       (0x48080000)

/** @brief Base address of I2C memory mapped registers                        */
#define SOC_I2C_0_REGS                       (0x48028000)
#define SOC_I2C_1_REGS                       (0x4802A000)
#define SOC_I2C_2_REGS                       (0x4819C000)
#define SOC_I2C_3_REGS                       (0x4819E000)

/** @brief Base address of WDT memory mapped registers                        */
#define SOC_WDT_0_REGS                       (0x481C7000)

/** @brief Base address of EMAC SW memory mapped registers                    */
#define SOC_CPSW_SS_REGS                     (0x4A100000)
#define SOC_CPSW_PORT_0_REGS                 (0x4A100100)
#define SOC_CPSW_PORT_1_REGS                 (0x4A100200)
#define SOC_CPSW_PORT_2_REGS                 (0x4A100300)
#define SOC_CPSW_CPDMA_REGS                  (0x4A100800)
#define SOC_CPSW_STAT_REGS                   (0x4A100900)
#define SOC_CPSW_CPTS_REGS                   (0x4A100C00)
#define SOC_CPSW_ALE_REGS                    (0x4A100D00)
#define SOC_CPSW_SLIVER_1_REGS               (0x4A100D80)
#define SOC_CPSW_SLIVER_2_REGS               (0x4A100DC0)
#define SOC_CPSW_MDIO_REGS                   (0x4A101000)
#define SOC_CPSW_WR_REGS                     (0x4A101200)
#define SOC_CPSW_CPPI_RAM_REGS               (0x4A102000)

/** @brief Base address of McASP memory mapped registers                      */
#define SOC_MCASP_0_CTRL_REGS                (0x48038000)
#define SOC_MCASP_0_FIFO_REGS                (SOC_MCASP_0_CTRL_REGS + 0x1000)
#define SOC_MCASP_0_DATA_REGS                (0x46000000)
#define SOC_MCASP_1_CTRL_REGS                (0x4803C000)
#define SOC_MCASP_1_FIFO_REGS                (SOC_MCASP_1_CTRL_REGS + 0x1000)
#define SOC_MCASP_1_DATA_REGS                (0x46400000)
#define SOC_MCASP_2_CTRL_REGS                (0x48050000)
#define SOC_MCASP_2_FIFO_REGS                (SOC_MCASP_2_CTRL_REGS + 0x1000)
#define SOC_MCASP_2_DATA_REGS                (0x46800000)
#define SOC_MCASP_3_CTRL_REGS                (0x4A1A2000)
#define SOC_MCASP_3_FIFO_REGS                (SOC_MCASP_3_CTRL_REGS + 0x1000)
#define SOC_MCASP_3_DATA_REGS                (0x4A1A5000)
#define SOC_MCASP_4_CTRL_REGS                (0x4A1A8000)
#define SOC_MCASP_4_FIFO_REGS                (SOC_MCASP_4_CTRL_REGS + 0x1000)
#define SOC_MCASP_4_DATA_REGS                (0x4A1AB000)
#define SOC_MCASP_5_CTRL_REGS                (0x4A1AE000)
#define SOC_MCASP_5_FIFO_REGS                (SOC_MCASP_5_CTRL_REGS + 0x1000)
#define SOC_MCASP_5_DATA_REGS                (0x4A1B1000)

/** @brief Base address of McBSP memory mapped registers                      */
#define SOC_MCBSP_0_CTRL_REGS                (0x47000000)

/** @brief Base address of EMIF memory mapped registers                       */
#define SOC_EMIF_0_REGS                      (0x4C000000)
#define SOC_EMIF_1_REGS                      (0x4D000000)

/** @brief Base addresses of RTC memory mapped registers                      */
#define SOC_RTC_0_REGS                       (0x44E3E000)

/** @brief Base addresses of PRCM memory mapped registers                     */
#define SOC_PRCM_REGS                        (0x48180000)
#define SOC_PRM_DEVICE_REGS                  (SOC_PRCM_REGS + 0x0)
#define SOC_CM_DEVICE_REGS                   (SOC_PRCM_REGS + 0x100)
#define SOC_OCP_SOCKET_PRM_REGS              (SOC_PRCM_REGS + 0x200)
#define SOC_CM_DPLL_REGS                     (SOC_PRCM_REGS + 0x300)
#define SOC_CM_ACTIVE_REGS                   (SOC_PRCM_REGS + 0x400)
#define SOC_CM_DEFAULT_REGS                  (SOC_PRCM_REGS + 0x500)
/* note: removed HDICP, ISP */
#define SOC_CM_DSS_REGS                      (SOC_PRCM_REGS + 0x800)
#define SOC_CM_SGX_REGS                      (SOC_PRCM_REGS + 0x900)
#define SOC_PRM_ACTIVE_REGS                  (SOC_PRCM_REGS + 0xA00)
#define SOC_PRM_DEFAULT_REGS                 (SOC_PRCM_REGS + 0xB00)
/* note: removed HDICP, ISP */
#define SOC_PRM_DSS_REGS                     (SOC_PRCM_REGS + 0xE00)
#define SOC_PRM_SGX_REGS                     (SOC_PRCM_REGS + 0xF00)
#define SOC_CM_ALWON_REGS                    (SOC_PRCM_REGS + 0x1400)
#define SOC_PRM_ALWON_REGS                   (SOC_PRCM_REGS + 0x1800)

/** @brief Base addresses of PLLSS memory mapped registers                    */
#define SOC_PLLSS_REGS                       (0x481C5000)

/** @brief Base address of control module memory mapped registers             */
#define SOC_CONTROL_REGS                     (0x48140000)

/** @brief Base address of Channel controller  memory mapped registers        */
#define SOC_EDMA30CC_0_REGS                  (0x49000000)
#define SOC_EDMA30TC_0_REGS                  (0x49800000)
#define SOC_EDMA30TC_1_REGS                  (0x49900000)
#define SOC_EDMA30TC_2_REGS                  (0x49A00000)
#define SOC_EDMA30TC_3_REGS                  (0x49B00000)


/** @brief Base address of DCAN module memory mapped registers                */
#define SOC_DCAN_0_REGS                      (0x481CC000)
#define SOC_DCAN_1_REGS                      (0x481D0000)

/** @brief Base address of PCIe module memory mapped port and registers       */
#define SOC_PCIE_0_PORT                      (0x20000000)
#define SOC_PCIE_0_REGS                      (0x51000000)

/******************************************************************************\
*  Parameterizable Configuration:- These are fed directly from the RTL
*  parameters for the given SOC
\******************************************************************************/
#define TPCC_MUX(n)                         0xF90 + ((n) * 4)

#define SOC_ADC_TSC_0_REGS                  (0x48450000)

/** @brief Base addresses of PWMSS memory mapped registers.                   */

#define SOC_PWMSS0_REGS                     (0x48302000)
#define SOC_PWMSS1_REGS                     (0x48304000)
#define SOC_PWMSS2_REGS                     (0x48306000)

#define SOC_ECAP_REGS                       (0x00000100)
#define SOC_EQEP_REGS                       (0x00000180)
#define SOC_EPWM_REGS                       (0x00000200)

#define SOC_ECAP_0_REGS                     (SOC_PWMSS0_REGS + SOC_ECAP_REGS)
#define SOC_ECAP_1_REGS                     (SOC_PWMSS1_REGS + SOC_ECAP_REGS)
#define SOC_ECAP_2_REGS                     (SOC_PWMSS2_REGS + SOC_ECAP_REGS)

#define SOC_EQEP_0_REGS                     (SOC_PWMSS0_REGS + SOC_EQEP_REGS)
#define SOC_EQEP_1_REGS                     (SOC_PWMSS1_REGS + SOC_EQEP_REGS)
#define SOC_EQEP_2_REGS                     (SOC_PWMSS2_REGS + SOC_EQEP_REGS) 

#define SOC_EPWM_0_REGS                     (SOC_PWMSS0_REGS + SOC_EPWM_REGS)
#define SOC_EPWM_1_REGS                     (SOC_PWMSS1_REGS + SOC_EPWM_REGS)
#define SOC_EPWM_2_REGS                     (SOC_PWMSS2_REGS + SOC_EPWM_REGS)

#define SOC_PWM0_SUB_SYS_CLOCK_CONFIG_REGS   0x48302008
#define SOC_PWM0_SUB_SYS_CLOCK_STATUS_REGS   0x4830200C
#define SOC_PWM0_SUB_SYS_SYS_CONFIG_REGS     0x48302004

#define SOC_PWM1_SUB_SYS_CLOCK_CONFIG_REGS   0x48304008
#define SOC_PWM1_SUB_SYS_CLOCK_STATUS_REGS   0x4830400C
#define SOC_PWM1_SUB_SYS_SYS_CONFIG_REGS     0x48304004

#define SOC_PWM2_SUB_SYS_CLOCK_CONFIG_REGS   0x48306008
#define SOC_PWM2_SUB_SYS_CLOCK_STATUS_REGS   0x4830600C
#define SOC_PWM2_SUB_SYS_SYS_CONFIG_REGS     0x48306004

#define SOC_EPWM_MODULE_FREQ                 100 
 
#ifdef __cplusplus
}
#endif

#endif  /* _SOC_C6A811X_H_ */
