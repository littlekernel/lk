/** ============================================================================
 *   \file  hw_uart.h
 *
 *   \brief This file contains the Register Descriptions for UART
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


#ifndef _HW_UART_H_
#define _HW_UART_H_

#ifdef __cplusplus
extern "C" {
#endif

#define UART_RBR		(0x0)
#define UART_THR		(0x0)
#define UART_IER		(0x4)
#define UART_IIR		(0x8)
#define UART_FCR		(0x8)
#define UART_LCR		(0xC)
#define UART_MCR		(0x10)
#define UART_LSR		(0x14)
#define UART_MSR		(0x18)
#define UART_SCR		(0x1C)
#define UART_DLL		(0x20)
#define UART_DLH		(0x24)
#define UART_REVID1		(0x28)
#define UART_REVID2		(0x2C)
#define UART_PWREMU_MGMT 	(0x30)
#define UART_MDR		(0x34)

/**************************************************************************\
* Field Definition Macros
\**************************************************************************/

/* RBR */
#define UART_RBR_DATA           (0x000000FFu)
#define UART_RBR_DATA_SHIFT          (0x00000000u)

/* THR */
#define UART_THR_DATA           (0x000000FFu)
#define UART_THR_DATA_SHIFT          (0x00000000u)

/* IER */
#define UART_IER_EDSSI          (0x00000008u)
#define UART_IER_EDSSI_SHIFT         (0x00000003u)
#define UART_IER_ELSI           (0x00000004u)
#define UART_IER_ELSI_SHIFT          (0x00000002u)
#define UART_IER_ETBEI          (0x00000002u)
#define UART_IER_ETBEI_SHIFT         (0x00000001u)
#define UART_IER_ERBI           (0x00000001u)
#define UART_IER_ERBI_SHIFT          (0x00000000u)

/* IIR */
#define UART_IIR_FIFOEN         (0x000000C0u)
#define UART_IIR_FIFOEN_SHIFT        (0x00000006u)
#define UART_IIR_INTID          (0x0000000Eu)
#define UART_IIR_INTID_SHIFT         (0x00000001u)
/*----INTID Tokens----*/
#define UART_IIR_INTID_MODSTAT       (0x00000000u)
#define UART_IIR_INTID_THRE          (0x00000001u)
#define UART_IIR_INTID_RDA           (0x00000002u)
#define UART_IIR_INTID_RLS           (0x00000003u)
#define UART_IIR_INTID_CTI           (0x00000006u)
#define UART_IIR_IPEND          (0x00000001u)
#define UART_IIR_IPEND_SHIFT         (0x00000000u)

/* FCR */
#define UART_FCR_RXFIFTL        (0x000000C0u)
#define UART_FCR_RXFIFTL_SHIFT       (0x00000006u)
/*----RXFIFTL Tokens----*/
#define UART_FCR_RXFIFTL_CHAR1       (0x00000000u)
#define UART_FCR_RXFIFTL_CHAR4       (0x00000001u)
#define UART_FCR_RXFIFTL_CHAR8       (0x00000002u)
#define UART_FCR_RXFIFTL_CHAR14      (0x00000003u)
#define UART_FCR_DMAMODE1       (0x00000008u)
#define UART_FCR_DMAMODE1_SHIFT      (0x00000003u)
#define UART_FCR_TXCLR          (0x00000004u)
#define UART_FCR_TXCLR_SHIFT         (0x00000002u)
#define UART_FCR_RXCLR          (0x00000002u)
#define UART_FCR_RXCLR_SHIFT         (0x00000001u)
#define UART_FCR_FIFOEN         (0x00000001u)
#define UART_FCR_FIFOEN_SHIFT        (0x00000000u)

/* LCR */
#define UART_LCR_DLAB           (0x00000080u)
#define UART_LCR_DLAB_SHIFT          (0x00000007u)
#define UART_LCR_BC             (0x00000040u)
#define UART_LCR_BC_SHIFT            (0x00000006u)
#define UART_LCR_SP             (0x00000020u)
#define UART_LCR_SP_SHIFT            (0x00000005u)
#define UART_LCR_EPS            (0x00000010u)
#define UART_LCR_EPS_SHIFT           (0x00000004u)
/*----EPS Tokens----*/
#define UART_LCR_EPS_ODD             (0x00000000u)
#define UART_LCR_EPS_EVEN            (0x00000001u)
#define UART_LCR_PEN            (0x00000008u)
#define UART_LCR_PEN_SHIFT           (0x00000003u)
#define UART_LCR_STB            (0x00000004u)
#define UART_LCR_STB_SHIFT           (0x00000002u)

#define UART_LCR_WLS            (0x00000003u)
#define UART_LCR_WLS_SHIFT           (0x00000000u)
/*----WLS Tokens----*/
#define UART_LCR_WLS_5BITS           (0x00000000u)
#define UART_LCR_WLS_6BITS           (0x00000001u)
#define UART_LCR_WLS_7BITS           (0x00000002u)
#define UART_LCR_WLS_8BITS           (0x00000003u)

/* MCR */
#define UART_MCR_AFE            (0x00000020u)
#define UART_MCR_AFE_SHIFT           (0x00000005u)
#define UART_MCR_LOOP           (0x00000010u)
#define UART_MCR_LOOP_SHIFT          (0x00000004u)
#define UART_MCR_OUT2           (0x00000008u)
#define UART_MCR_OUT2_SHIFT          (0x00000003u)
#define UART_MCR_OUT1           (0x00000004u)
#define UART_MCR_OUT1_SHIFT          (0x00000002u)
#define UART_MCR_RTS            (0x00000002u)
#define UART_MCR_RTS_SHIFT           (0x00000001u)
#define UART_MCR_DTR            (0x00000001u)
#define UART_MCR_DTR_SHIFT           (0x00000000u)

/* LSR */
#define UART_LSR_RXFIFOE        (0x00000080u)
#define UART_LSR_RXFIFOE_SHIFT       (0x00000007u)
#define UART_LSR_TEMT           (0x00000040u)
#define UART_LSR_TEMT_SHIFT          (0x00000006u)
#define UART_LSR_THRE           (0x00000020u)
#define UART_LSR_THRE_SHIFT          (0x00000005u)
#define UART_LSR_BI             (0x00000010u)
#define UART_LSR_BI_SHIFT            (0x00000004u)
#define UART_LSR_FE             (0x00000008u)
#define UART_LSR_FE_SHIFT            (0x00000003u)
#define UART_LSR_PE             (0x00000004u)
#define UART_LSR_PE_SHIFT            (0x00000002u)
#define UART_LSR_OE             (0x00000002u)
#define UART_LSR_OE_SHIFT            (0x00000001u)
#define UART_LSR_DR             (0x00000001u)
#define UART_LSR_DR_SHIFT            (0x00000000u)

/* MSR */
#define UART_MSR_CD             (0x00000080u)
#define UART_MSR_CD_SHIFT            (0x00000007u)
#define UART_MSR_RI             (0x00000040u)
#define UART_MSR_RI_SHIFT            (0x00000006u)
#define UART_MSR_DSR            (0x00000020u)
#define UART_MSR_DSR_SHIFT           (0x00000005u)
#define UART_MSR_CTS            (0x00000010u)
#define UART_MSR_CTS_SHIFT           (0x00000004u)
#define UART_MSR_DCD            (0x00000008u)
#define UART_MSR_DCD_SHIFT           (0x00000003u)
#define UART_MSR_TERI           (0x00000004u)
#define UART_MSR_TERI_SHIFT          (0x00000002u)
#define UART_MSR_DDSR           (0x00000002u)
#define UART_MSR_DDSR_SHIFT          (0x00000001u)
#define UART_MSR_DCTS           (0x00000001u)
#define UART_MSR_DCTS_SHIFT          (0x00000000u)

/* SCR */
#define UART_SCR_SCR            (0x000000FFu)
#define UART_SCR_SCR_SHIFT           (0x00000000u)

/* DLL */
#define UART_DLL_DLL            (0x000000FFu)
#define UART_DLL_DLL_SHIFT           (0x00000000u)

/* DLH */
#define UART_DLH_DLH            (0x000000FFu)
#define UART_DLH_DLH_SHIFT           (0x00000000u)

/* PID1 */
#define UART_REVID1_REV           (0xFFFFFFFFu)
#define UART_REVID1_REV_SHIFT          (0x00000000u)

/* PID2 */


#define UART_REVID2_REV           (0xFFFFFFFFu)
#define UART_REVID2_REV_SHIFT          (0x00000000u)

/* PWREMU_MGMT */

#define UART_PWREMU_MGMT_UTRST  (0x00004000u)
#define UART_PWREMU_MGMT_UTRST_SHIFT (0x0000000Eu)

#define UART_PWREMU_MGMT_URRST  (0x00002000u)
#define UART_PWREMU_MGMT_URRST_SHIFT (0x0000000Du)
#define UART_PWREMU_MGMT_FREE   (0x00000001u)
#define UART_PWREMU_MGMT_FREE_SHIFT  (0x00000000u)

/* MDR */
#define UART_MDR_OSM_SEL        (0x00000001u)
#define UART_MDR_OSM_SEL_SHIFT       (0x00000000u)

#ifdef __cplusplus
}
#endif

#endif
