/*
 * hw_sata.h
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



#ifndef _HW_SATA_H_
#define _HW_SATA_H_

#ifdef __cplusplus
extern "C" {
#endif

#define SATA_CAP		(0x0)
#define SATA_GHC		(0x4)
#define SATA_IS			(0x8)
#define SATA_PI			(0xC)
#define SATA_VR			(0x10)
#define SATA_CCCCTL		(0x14)
#define SATA_CCCPORTS		(0x18)
#define SATA_BISTAFR		(0xA0)
#define SATA_BISTCR		(0xA4)
#define SATA_BISTFCTR		(0xA8)
#define SATA_BISTSR		(0xAC)
#define SATA_BISTDECR		(0xB0)
#define SATA_TIMER1MS		(0xE0)
#define SATA_GPARAM1R		(0xE8)
#define SATA_GPARAM2R		(0xEC)
#define SATA_PPARAMR		(0xF0)
#define SATA_TESTR		(0xF4)
#define SATA_VERSIONR		(0xF8)
#define SATA_IDR		(0xFC)
#define SATA_P0CLB		(0x100)
#define SATA_P0FB		(0x108)
#define SATA_P0IS		(0x110)
#define SATA_P0IE		(0x114)
#define SATA_P0CMD		(0x118)
#define SATA_P0TFD		(0x120)
#define SATA_P0SIG		(0x124)
#define SATA_P0SSTS		(0x128)
#define SATA_P0SCTL		(0x12C)
#define SATA_P0SERR		(0x130)
#define SATA_P0SACT		(0x134)
#define SATA_P0CI		(0x138)
#define SATA_P0SNTF		(0x13C)
#define SATA_P0DMACR		(0x170)
#define SATA_P0PHYCR		(0x178)
#define SATA_P0PHYSR		(0x17C)

/**************************************************************************\
* Field Definition Macros
\**************************************************************************/

/* CAP */

#define SATA_CAP_S64A           (0x80000000u)
#define SATA_CAP_S64A_SHIFT          (0x0000001Fu)

#define SATA_CAP_SNCQ           (0x40000000u)
#define SATA_CAP_SNCQ_SHIFT          (0x0000001Eu)

#define SATA_CAP_SSNTF          (0x20000000u)
#define SATA_CAP_SSNTF_SHIFT         (0x0000001Du)

#define SATA_CAP_SMPS           (0x10000000u)
#define SATA_CAP_SMPS_SHIFT          (0x0000001Cu)

#define SATA_CAP_SSS            (0x08000000u)
#define SATA_CAP_SSS_SHIFT           (0x0000001Bu)

#define SATA_CAP_SALP           (0x04000000u)
#define SATA_CAP_SALP_SHIFT          (0x0000001Au)

#define SATA_CAP_SAL            (0x02000000u)
#define SATA_CAP_SAL_SHIFT           (0x00000019u)

#define SATA_CAP_SCLO           (0x01000000u)
#define SATA_CAP_SCLO_SHIFT          (0x00000018u)

#define SATA_CAP_ISS            (0x00F00000u)
#define SATA_CAP_ISS_SHIFT           (0x00000014u)

#define SATA_CAP_SNZO           (0x00080000u)
#define SATA_CAP_SNZO_SHIFT          (0x00000013u)

#define SATA_CAP_SAM            (0x00040000u)
#define SATA_CAP_SAM_SHIFT           (0x00000012u)

#define SATA_CAP_SPM            (0x00020000u)
#define SATA_CAP_SPM_SHIFT           (0x00000011u)


#define SATA_CAP_PMD            (0x00008000u)
#define SATA_CAP_PMD_SHIFT           (0x0000000Fu)

#define SATA_CAP_SSC            (0x00004000u)
#define SATA_CAP_SSC_SHIFT           (0x0000000Eu)

#define SATA_CAP_PSC            (0x00002000u)
#define SATA_CAP_PSC_SHIFT           (0x0000000Du)

#define SATA_CAP_NCS            (0x00001F00u)
#define SATA_CAP_NCS_SHIFT           (0x00000008u)

#define SATA_CAP_CCCS           (0x00000080u)
#define SATA_CAP_CCCS_SHIFT          (0x00000007u)

#define SATA_CAP_EMS            (0x00000040u)
#define SATA_CAP_EMS_SHIFT           (0x00000006u)

#define SATA_CAP_SXS            (0x00000020u)
#define SATA_CAP_SXS_SHIFT           (0x00000005u)

#define SATA_CAP_NP             (0x0000001Fu)
#define SATA_CAP_NP_SHIFT            (0x00000000u)


/* GHC */

#define SATA_GHC_AE             (0x80000000u)
#define SATA_GHC_AE_SHIFT            (0x0000001Fu)


#define SATA_GHC_IE             (0x00000002u)
#define SATA_GHC_IE_SHIFT            (0x00000001u)

#define SATA_GHC_HR             (0x00000001u)
#define SATA_GHC_HR_SHIFT            (0x00000000u)


/* IS */


#define SATA_IS_IPS             (0x00000001u)
#define SATA_IS_IPS_SHIFT            (0x00000000u)


/* PI */


#define SATA_PI_PI              (0x00000001u)
#define SATA_PI_PI_SHIFT             (0x00000000u)


/* VR */

#define SATA_VR_MJR             (0xFFFF0000u)
#define SATA_VR_MJR_SHIFT            (0x00000010u)

#define SATA_VR_MNR             (0x0000FFFFu)
#define SATA_VR_MNR_SHIFT            (0x00000000u)


/* CCCCTL */

#define SATA_CCCCTL_TV          (0xFFFF0000u)
#define SATA_CCCCTL_TV_SHIFT         (0x00000010u)

#define SATA_CCCCTL_CC          (0x0000FF00u)
#define SATA_CCCCTL_CC_SHIFT         (0x00000008u)

#define SATA_CCCCTL_INT         (0x000000F8u)
#define SATA_CCCCTL_INT_SHIFT        (0x00000003u)


#define SATA_CCCCTL_EN          (0x00000001u)
#define SATA_CCCCTL_EN_SHIFT         (0x00000000u)


/* CCCPORTS */


#define SATA_CCCPORTS_PRT       (0x00000001u)
#define SATA_CCCPORTS_PRT_SHIFT      (0x00000000u)


/* BISTAFR */


#define SATA_BISTAFR_NCP        (0x0000FF00u)
#define SATA_BISTAFR_NCP_SHIFT       (0x00000008u)

#define SATA_BISTAFR_PD         (0x000000FFu)
#define SATA_BISTAFR_PD_SHIFT        (0x00000000u)


/* BISTCR */


#define SATA_BISTCR_TXO         (0x00040000u)
#define SATA_BISTCR_TXO_SHIFT        (0x00000012u)

#define SATA_BISTCR_CNTCLR      (0x00020000u)
#define SATA_BISTCR_CNTCLR_SHIFT     (0x00000011u)

#define SATA_BISTCR_NEALB       (0x00010000u)
#define SATA_BISTCR_NEALB_SHIFT      (0x00000010u)


#define SATA_BISTCR_LLC         (0x00000700u)
#define SATA_BISTCR_LLC_SHIFT        (0x00000008u)


#define SATA_BISTCR_ERREN       (0x00000040u)
#define SATA_BISTCR_ERREN_SHIFT      (0x00000006u)

#define SATA_BISTCR_FLIP        (0x00000020u)
#define SATA_BISTCR_FLIP_SHIFT       (0x00000005u)

#define SATA_BISTCR_PV          (0x00000010u)
#define SATA_BISTCR_PV_SHIFT         (0x00000004u)

#define SATA_BISTCR_PATTERN     (0x0000000Fu)
#define SATA_BISTCR_PATTERN_SHIFT    (0x00000000u)


/* BISTFCTR */

#define SATA_BISTFCTR_BISTFCTR  (0xFFFFFFFFu)
#define SATA_BISTFCTR_BISTFCTR_SHIFT (0x00000000u)


/* BISTSR */


#define SATA_BISTSR_BRSTERR     (0x00FF0000u)
#define SATA_BISTSR_BRSTERR_SHIFT    (0x00000010u)

#define SATA_BISTSR_FRAMERR     (0x0000FFFFu)
#define SATA_BISTSR_FRAMERR_SHIFT    (0x00000000u)


/* BISTDECR */

#define SATA_BISTDECR_DWERR     (0xFFFFFFFFu)
#define SATA_BISTDECR_DWERR_SHIFT    (0x00000000u)


/* TIMER1MS */


#define SATA_TIMER1MS_TIMV      (0x000FFFFFu)
#define SATA_TIMER1MS_TIMV_SHIFT     (0x00000000u)


/* GPARAM1R */

#define SATA_GPARAM1R_ALIGN_M   (0x80000000u)
#define SATA_GPARAM1R_ALIGN_M_SHIFT  (0x0000001Fu)

#define SATA_GPARAM1R_RX_BUFFER (0x40000000u)
#define SATA_GPARAM1R_RX_BUFFER_SHIFT (0x0000001Eu)

#define SATA_GPARAM1R_PHY_DATA  (0x30000000u)
#define SATA_GPARAM1R_PHY_DATA_SHIFT (0x0000001Cu)

#define SATA_GPARAM1R_PHY_RST   (0x08000000u)
#define SATA_GPARAM1R_PHY_RST_SHIFT  (0x0000001Bu)

#define SATA_GPARAM1R_PHY_CTRL  (0x07E00000u)
#define SATA_GPARAM1R_PHY_CTRL_SHIFT (0x00000015u)

#define SATA_GPARAM1R_PHY_STAT  (0x001F8000u)
#define SATA_GPARAM1R_PHY_STAT_SHIFT (0x0000000Fu)

#define SATA_GPARAM1R_LATCH_M   (0x00004000u)
#define SATA_GPARAM1R_LATCH_M_SHIFT  (0x0000000Eu)

#define SATA_GPARAM1R_BIST_M    (0x00002000u)
#define SATA_GPARAM1R_BIST_M_SHIFT   (0x0000000Du)

#define SATA_GPARAM1R_PHY_TYPE  (0x00001000u)
#define SATA_GPARAM1R_PHY_TYPE_SHIFT (0x0000000Cu)


#define SATA_GPARAM1R_RETURN_ERR (0x00000400u)
#define SATA_GPARAM1R_RETURN_ERR_SHIFT (0x0000000Au)

#define SATA_GPARAM1R_AHB_ENDIAN (0x00000300u)
#define SATA_GPARAM1R_AHB_ENDIAN_SHIFT (0x00000008u)

#define SATA_GPARAM1R_S_HADDR   (0x00000080u)
#define SATA_GPARAM1R_S_HADDR_SHIFT  (0x00000007u)

#define SATA_GPARAM1R_M_HADDR   (0x00000040u)
#define SATA_GPARAM1R_M_HADDR_SHIFT  (0x00000006u)

#define SATA_GPARAM1R_S_HDATA   (0x00000038u)
#define SATA_GPARAM1R_S_HDATA_SHIFT  (0x00000003u)

#define SATA_GPARAM1R_M_HDATA   (0x00000007u)
#define SATA_GPARAM1R_M_HDATA_SHIFT  (0x00000000u)


/* GPARAM2R */


#define SATA_GPARAM2R_DEV_CP    (0x00004000u)
#define SATA_GPARAM2R_DEV_CP_SHIFT   (0x0000000Eu)

#define SATA_GPARAM2R_DEV_MP    (0x00002000u)
#define SATA_GPARAM2R_DEV_MP_SHIFT   (0x0000000Du)

#define SATA_GPARAM2R_ENCODE_M  (0x00001000u)
#define SATA_GPARAM2R_ENCODE_M_SHIFT (0x0000000Cu)

#define SATA_GPARAM2R_RXOOB_CLK_M (0x00000800u)
#define SATA_GPARAM2R_RXOOB_CLK_M_SHIFT (0x0000000Bu)

#define SATA_GPARAM2R_RX_OOB_M  (0x00000400u)
#define SATA_GPARAM2R_RX_OOB_M_SHIFT (0x0000000Au)

#define SATA_GPARAM2R_TX_OOB_M  (0x00000200u)
#define SATA_GPARAM2R_TX_OOB_M_SHIFT (0x00000009u)

#define SATA_GPARAM2R_RXOOB_CLK (0x000001FFu)
#define SATA_GPARAM2R_RXOOB_CLK_SHIFT (0x00000000u)


/* PPARAMR */


#define SATA_PPARAMR_TX_MEM_M   (0x00000200u)
#define SATA_PPARAMR_TX_MEM_M_SHIFT  (0x00000009u)

#define SATA_PPARAMR_TX_MEM_S   (0x00000100u)
#define SATA_PPARAMR_TX_MEM_S_SHIFT  (0x00000008u)

#define SATA_PPARAMR_RX_MEM_M   (0x00000080u)
#define SATA_PPARAMR_RX_MEM_M_SHIFT  (0x00000007u)

#define SATA_PPARAMR_RX_MEM_S   (0x00000040u)
#define SATA_PPARAMR_RX_MEM_S_SHIFT  (0x00000006u)

#define SATA_PPARAMR_TX_FIFO_DEPTH (0x00000038u)
#define SATA_PPARAMR_TX_FIFO_DEPTH_SHIFT (0x00000003u)

#define SATA_PPARAMR_RX_FIFO_DEPTH (0x00000007u)
#define SATA_PPARAMR_RX_FIFO_DEPTH_SHIFT (0x00000000u)


/* TESTR */


#define SATA_TESTR_PSEL         (0x00070000u)
#define SATA_TESTR_PSEL_SHIFT        (0x00000010u)


#define SATA_TESTR_TEST_IF      (0x00000001u)
#define SATA_TESTR_TEST_IF_SHIFT     (0x00000000u)


/* VERSIONR */

#define SATA_VERSIONR_VERSION   (0xFFFFFFFFu)
#define SATA_VERSIONR_VERSION_SHIFT  (0x00000000u)


/* IDR */

#define SATA_IDR_ID             (0xFFFFFFFFu)
#define SATA_IDR_ID_SHIFT            (0x00000000u)


/* P0CLB */

#define SATA_P0CLB_CLB          (0xFFFFFFFFu)
#define SATA_P0CLB_CLB_SHIFT         (0x00000000u)


/* P0FB */

#define SATA_P0FB_FB            (0xFFFFFFFFu)
#define SATA_P0FB_FB_SHIFT           (0x00000000u)


/* P0IS */

#define SATA_P0IS_CPDS          (0x80000000u)
#define SATA_P0IS_CPDS_SHIFT         (0x0000001Fu)

#define SATA_P0IS_TFES          (0x40000000u)
#define SATA_P0IS_TFES_SHIFT         (0x0000001Eu)

#define SATA_P0IS_HBFS          (0x20000000u)
#define SATA_P0IS_HBFS_SHIFT         (0x0000001Du)

#define SATA_P0IS_HBDS          (0x10000000u)
#define SATA_P0IS_HBDS_SHIFT         (0x0000001Cu)

#define SATA_P0IS_IFS           (0x08000000u)
#define SATA_P0IS_IFS_SHIFT          (0x0000001Bu)

#define SATA_P0IS_INFS          (0x04000000u)
#define SATA_P0IS_INFS_SHIFT         (0x0000001Au)


#define SATA_P0IS_OFS           (0x01000000u)
#define SATA_P0IS_OFS_SHIFT          (0x00000018u)

#define SATA_P0IS_IPMS          (0x00800000u)
#define SATA_P0IS_IPMS_SHIFT         (0x00000017u)

#define SATA_P0IS_PRCS          (0x00400000u)
#define SATA_P0IS_PRCS_SHIFT         (0x00000016u)


#define SATA_P0IS_DMPS          (0x00000080u)
#define SATA_P0IS_DMPS_SHIFT         (0x00000007u)

#define SATA_P0IS_PCS           (0x00000040u)
#define SATA_P0IS_PCS_SHIFT          (0x00000006u)

#define SATA_P0IS_DPS           (0x00000020u)
#define SATA_P0IS_DPS_SHIFT          (0x00000005u)

#define SATA_P0IS_UFS           (0x00000010u)
#define SATA_P0IS_UFS_SHIFT          (0x00000004u)

#define SATA_P0IS_SDBS          (0x00000008u)
#define SATA_P0IS_SDBS_SHIFT         (0x00000003u)

#define SATA_P0IS_DSS           (0x00000004u)
#define SATA_P0IS_DSS_SHIFT          (0x00000002u)

#define SATA_P0IS_PSS           (0x00000002u)
#define SATA_P0IS_PSS_SHIFT          (0x00000001u)

#define SATA_P0IS_DHRS          (0x00000001u)
#define SATA_P0IS_DHRS_SHIFT         (0x00000000u)


/* P0IE */

#define SATA_P0IE_CPDE          (0x80000000u)
#define SATA_P0IE_CPDE_SHIFT         (0x0000001Fu)

#define SATA_P0IE_TFEE          (0x40000000u)
#define SATA_P0IE_TFEE_SHIFT         (0x0000001Eu)

#define SATA_P0IE_HBFE          (0x20000000u)
#define SATA_P0IE_HBFE_SHIFT         (0x0000001Du)

#define SATA_P0IE_HBDE          (0x10000000u)
#define SATA_P0IE_HBDE_SHIFT         (0x0000001Cu)

#define SATA_P0IE_IFE           (0x08000000u)
#define SATA_P0IE_IFE_SHIFT          (0x0000001Bu)

#define SATA_P0IE_INFE          (0x04000000u)
#define SATA_P0IE_INFE_SHIFT         (0x0000001Au)


#define SATA_P0IE_OFE           (0x01000000u)
#define SATA_P0IE_OFE_SHIFT          (0x00000018u)

#define SATA_P0IE_IPME          (0x00800000u)
#define SATA_P0IE_IPME_SHIFT         (0x00000017u)

#define SATA_P0IE_PRCE          (0x00400000u)
#define SATA_P0IE_PRCE_SHIFT         (0x00000016u)


#define SATA_P0IE_DMPE          (0x00000080u)
#define SATA_P0IE_DMPE_SHIFT         (0x00000007u)

#define SATA_P0IE_PCE           (0x00000040u)
#define SATA_P0IE_PCE_SHIFT          (0x00000006u)

#define SATA_P0IE_DPE           (0x00000020u)
#define SATA_P0IE_DPE_SHIFT          (0x00000005u)

#define SATA_P0IE_UFE           (0x00000010u)
#define SATA_P0IE_UFE_SHIFT          (0x00000004u)

#define SATA_P0IE_SDBE          (0x00000008u)
#define SATA_P0IE_SDBE_SHIFT         (0x00000003u)

#define SATA_P0IE_DSE           (0x00000004u)
#define SATA_P0IE_DSE_SHIFT          (0x00000002u)

#define SATA_P0IE_PSE           (0x00000002u)
#define SATA_P0IE_PSE_SHIFT          (0x00000001u)

#define SATA_P0IE_DHRE          (0x00000001u)
#define SATA_P0IE_DHRE_SHIFT         (0x00000000u)


/* P0CMD */

#define SATA_P0CMD_ICC          (0xF0000000u)
#define SATA_P0CMD_ICC_SHIFT         (0x0000001Cu)

#define SATA_P0CMD_ASP          (0x08000000u)
#define SATA_P0CMD_ASP_SHIFT         (0x0000001Bu)

#define SATA_P0CMD_ALPE         (0x04000000u)
#define SATA_P0CMD_ALPE_SHIFT        (0x0000001Au)

#define SATA_P0CMD_DLAE         (0x02000000u)
#define SATA_P0CMD_DLAE_SHIFT        (0x00000019u)

#define SATA_P0CMD_ATAPI        (0x01000000u)
#define SATA_P0CMD_ATAPI_SHIFT       (0x00000018u)


#define SATA_P0CMD_ESP          (0x00200000u)
#define SATA_P0CMD_ESP_SHIFT         (0x00000015u)

#define SATA_P0CMD_CPD          (0x00100000u)
#define SATA_P0CMD_CPD_SHIFT         (0x00000014u)

#define SATA_P0CMD_MPSP         (0x00080000u)
#define SATA_P0CMD_MPSP_SHIFT        (0x00000013u)

#define SATA_P0CMD_HPCP         (0x00040000u)
#define SATA_P0CMD_HPCP_SHIFT        (0x00000012u)

#define SATA_P0CMD_PMA          (0x00020000u)
#define SATA_P0CMD_PMA_SHIFT         (0x00000011u)

#define SATA_P0CMD_CPS          (0x00010000u)
#define SATA_P0CMD_CPS_SHIFT         (0x00000010u)

#define SATA_P0CMD_CR           (0x00008000u)
#define SATA_P0CMD_CR_SHIFT          (0x0000000Fu)

#define SATA_P0CMD_FR           (0x00004000u)
#define SATA_P0CMD_FR_SHIFT          (0x0000000Eu)

#define SATA_P0CMD_MPSS         (0x00002000u)
#define SATA_P0CMD_MPSS_SHIFT        (0x0000000Du)

#define SATA_P0CMD_CCS          (0x00001F00u)
#define SATA_P0CMD_CCS_SHIFT         (0x00000008u)


#define SATA_P0CMD_FRE          (0x00000010u)
#define SATA_P0CMD_FRE_SHIFT         (0x00000004u)

#define SATA_P0CMD_CLO          (0x00000008u)
#define SATA_P0CMD_CLO_SHIFT         (0x00000003u)

#define SATA_P0CMD_POD          (0x00000004u)
#define SATA_P0CMD_POD_SHIFT         (0x00000002u)

#define SATA_P0CMD_SUD          (0x00000002u)
#define SATA_P0CMD_SUD_SHIFT         (0x00000001u)

#define SATA_P0CMD_ST           (0x00000001u)
#define SATA_P0CMD_ST_SHIFT          (0x00000000u)


/* P0TFD */


#define SATA_P0TFD_ERR          (0x0000FF00u)
#define SATA_P0TFD_ERR_SHIFT         (0x00000008u)

#define SATA_P0TFD_STS          (0x000000FFu)
#define SATA_P0TFD_STS_SHIFT         (0x00000000u)


/* P0SIG */

#define SATA_P0SIG_SIG          (0xFFFFFFFFu)
#define SATA_P0SIG_SIG_SHIFT         (0x00000000u)


/* P0SSTS */


#define SATA_P0SSTS_IPM         (0x00000F00u)
#define SATA_P0SSTS_IPM_SHIFT        (0x00000008u)

#define SATA_P0SSTS_SPD         (0x000000F0u)
#define SATA_P0SSTS_SPD_SHIFT        (0x00000004u)

#define SATA_P0SSTS_DET         (0x0000000Fu)
#define SATA_P0SSTS_DET_SHIFT        (0x00000000u)


/* P0SCTL */


#define SATA_P0SCTL_IPM         (0x00000F00u)
#define SATA_P0SCTL_IPM_SHIFT        (0x00000008u)

#define SATA_P0SCTL_SPD         (0x000000F0u)
#define SATA_P0SCTL_SPD_SHIFT        (0x00000004u)

#define SATA_P0SCTL_DET         (0x0000000Fu)
#define SATA_P0SCTL_DET_SHIFT        (0x00000000u)


/* P0SERR */


#define SATA_P0SERR_DIAG_X      (0x04000000u)
#define SATA_P0SERR_DIAG_X_SHIFT     (0x0000001Au)

#define SATA_P0SERR_DIAG_F      (0x02000000u)
#define SATA_P0SERR_DIAG_F_SHIFT     (0x00000019u)

#define SATA_P0SERR_DIAG_T      (0x01000000u)
#define SATA_P0SERR_DIAG_T_SHIFT     (0x00000018u)

#define SATA_P0SERR_DIAG_S      (0x00800000u)
#define SATA_P0SERR_DIAG_S_SHIFT     (0x00000017u)

#define SATA_P0SERR_DIAG_H      (0x00400000u)
#define SATA_P0SERR_DIAG_H_SHIFT     (0x00000016u)

#define SATA_P0SERR_DIAG_C      (0x00200000u)
#define SATA_P0SERR_DIAG_C_SHIFT     (0x00000015u)

#define SATA_P0SERR_DIAG_D      (0x00100000u)
#define SATA_P0SERR_DIAG_D_SHIFT     (0x00000014u)

#define SATA_P0SERR_DIAG_B      (0x00080000u)
#define SATA_P0SERR_DIAG_B_SHIFT     (0x00000013u)

#define SATA_P0SERR_DIAG_W      (0x00040000u)
#define SATA_P0SERR_DIAG_W_SHIFT     (0x00000012u)

#define SATA_P0SERR_DIAG_I      (0x00020000u)
#define SATA_P0SERR_DIAG_I_SHIFT     (0x00000011u)

#define SATA_P0SERR_DIAG_N      (0x00010000u)
#define SATA_P0SERR_DIAG_N_SHIFT     (0x00000010u)


#define SATA_P0SERR_ERR_E       (0x00000800u)
#define SATA_P0SERR_ERR_E_SHIFT      (0x0000000Bu)

#define SATA_P0SERR_ERR_P       (0x00000400u)
#define SATA_P0SERR_ERR_P_SHIFT      (0x0000000Au)

#define SATA_P0SERR_ERR_C       (0x00000200u)
#define SATA_P0SERR_ERR_C_SHIFT      (0x00000009u)

#define SATA_P0SERR_ERR_T       (0x00000100u)
#define SATA_P0SERR_ERR_T_SHIFT      (0x00000008u)


#define SATA_P0SERR_ERR_M       (0x00000002u)
#define SATA_P0SERR_ERR_M_SHIFT      (0x00000001u)

#define SATA_P0SERR_ERR_I       (0x00000001u)
#define SATA_P0SERR_ERR_I_SHIFT      (0x00000000u)


/* P0SACT */

#define SATA_P0SACT_DS          (0xFFFFFFFFu)
#define SATA_P0SACT_DS_SHIFT         (0x00000000u)


/* P0CI */

#define SATA_P0CI_CI            (0xFFFFFFFFu)
#define SATA_P0CI_CI_SHIFT           (0x00000000u)


/* P0SNTF */


#define SATA_P0SNTF_PMN         (0x0000FFFFu)
#define SATA_P0SNTF_PMN_SHIFT        (0x00000000u)


/* P0DMACR */


#define SATA_P0DMACR_RXABL      (0x0000F000u)
#define SATA_P0DMACR_RXABL_SHIFT     (0x0000000Cu)

#define SATA_P0DMACR_TXABL      (0x00000F00u)
#define SATA_P0DMACR_TXABL_SHIFT     (0x00000008u)

#define SATA_P0DMACR_RXTS       (0x000000F0u)
#define SATA_P0DMACR_RXTS_SHIFT      (0x00000004u)

#define SATA_P0DMACR_TXTS       (0x0000000Fu)
#define SATA_P0DMACR_TXTS_SHIFT      (0x00000000u)


/* P0PHYCR */

#define SATA_P0PHYCR_ENPLL      (0x80000000u)
#define SATA_P0PHYCR_ENPLL_SHIFT     (0x0000001Fu)

#define SATA_P0PHYCR_OVERRI     (0x40000000u)
#define SATA_P0PHYCR_OVERRI_SHIFT    (0x0000001Eu)


#define SATA_P0PHYCR_TXDE       (0x03C00000u)
#define SATA_P0PHYCR_TXDE_SHIFT      (0x00000016u)

#define SATA_P0PHYCR_TXSWING    (0x00380000u)
#define SATA_P0PHYCR_TXSWING_SHIFT   (0x00000013u)

#define SATA_P0PHYCR_TXCM       (0x00040000u)
#define SATA_P0PHYCR_TXCM_SHIFT      (0x00000012u)

#define SATA_P0PHYCR_TXINVPAIR  (0x00020000u)
#define SATA_P0PHYCR_TXINVPAIR_SHIFT (0x00000011u)

#define SATA_P0PHYCR_RXEQ       (0x0001E000u)
#define SATA_P0PHYCR_RXEQ_SHIFT      (0x0000000Du)

#define SATA_P0PHYCR_RXCDR      (0x00001C00u)
#define SATA_P0PHYCR_RXCDR_SHIFT     (0x0000000Au)

#define SATA_P0PHYCR_RXTERM     (0x00000300u)
#define SATA_P0PHYCR_RXTERM_SHIFT    (0x00000008u)

#define SATA_P0PHYCR_RXINVPAIR  (0x00000080u)
#define SATA_P0PHYCR_RXINVPAIR_SHIFT (0x00000007u)

#define SATA_P0PHYCR_LOS        (0x00000040u)
#define SATA_P0PHYCR_LOS_SHIFT       (0x00000006u)

#define SATA_P0PHYCR_LB         (0x00000030u)
#define SATA_P0PHYCR_LB_SHIFT        (0x00000004u)

#define SATA_P0PHYCR_MPY        (0x0000000Fu)
#define SATA_P0PHYCR_MPY_SHIFT       (0x00000000u)


/* P0PHYSR */


#define SATA_P0PHYSR_SIGDET     (0x00000002u)
#define SATA_P0PHYSR_SIGDET_SHIFT    (0x00000001u)

#define SATA_P0PHYSR_LOCK       (0x00000001u)
#define SATA_P0PHYSR_LOCK_SHIFT      (0x00000000u)


#ifdef __cplusplus
}
#endif

#endif
