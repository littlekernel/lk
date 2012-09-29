/*
 * hw_emifb.h
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



#ifndef _HW_EMIFB_H_
#define _HW_EMIFB_H_

#ifdef __cplusplus
extern "C" {
#endif

#define EMIFB_REVID		(0x0)	
#define EMIFB_SDCFG		(0x4)
#define EMIFB_SDRFC		(0x8)
#define EMIFB_SDTIM1		(0xC)
#define EMIFB_SDTIM2		(0x10)
#define EMIFB_SDCFG2		(0x1C)
#define EMIFB_BPRIO		(0x20)
#define EMIFB_PC1		(0x40)
#define EMIFB_PC2		(0x44)
#define EMIFB_PCC		(0x48)
#define EMIFB_PCMRS		(0x4C)
#define EMIFB_PCT		(0x50)
#define EMIFB_IRR		(0xC0)
#define EMIFB_IMR		(0xC4)
#define EMIFB_IMSR		(0xC8)
#define EMIFB_IMCR		(0xCC)
#define EMIFB_DRPYC1R		(0xE4)

/**************************************************************************\
* Field Definition Macros
\**************************************************************************/

/* REVID */

#define EMIFB_REVID_REV         (0xFFFFFFFFu)
#define EMIFB_REVID_REV_SHIFT        (0x00000000u)


/* SDCFG */


#define EMIFB_SDCFG_MSDRAM_ENABLE_SHIFT (0x00000019u)

#define EMIFB_SDCFG_BOOT_UNLOCK (0x00800000u)
#define EMIFB_SDCFG_BOOT_UNLOCK_SHIFT (0x00000017u)

#define EMIFB_SDCFG_SDREN       (0x00010000u)
#define EMIFB_SDCFG_SDREN_SHIFT      (0x00000010u)

#define EMIFB_SDCFG_TIMUNLOCK   (0x00008000u)
#define EMIFB_SDCFG_TIMUNLOCK_SHIFT  (0x0000000Fu)

#define EMIFB_SDCFG_NM          (0x00004000u)
#define EMIFB_SDCFG_NM_SHIFT         (0x0000000Eu)

#define EMIFB_SDCFG_CL          (0x00000E00u)
#define EMIFB_SDCFG_CL_SHIFT         (0x00000009u)
/*----CL Tokens----*/
#define EMIFB_SDCFG_CL_TWO           (0x00000002u)
#define EMIFB_SDCFG_CL_THREE         (0x00000003u)


#define EMIFB_SDCFG_IBANK       (0x00000070u)
#define EMIFB_SDCFG_IBANK_SHIFT      (0x00000004u)
/*----IBANK Tokens----*/
#define EMIFB_SDCFG_IBANK_ONE        (0x00000000u)
#define EMIFB_SDCFG_IBANK_TWO        (0x00000001u)
#define EMIFB_SDCFG_IBANK_FOUR       (0x00000002u)

#define EMIFB_SDCFG_EBANK       (0x00000008u)
#define EMIFB_SDCFG_EBANK_SHIFT      (0x00000003u)

#define EMIFB_SDCFG_PAGESIZE    (0x00000007u)
#define EMIFB_SDCFG_PAGESIZE_SHIFT   (0x00000000u)
/*----PAGESIZE Tokens----*/
#define EMIFB_SDCFG_PAGESIZE_256W_PAGE (0x00000000u)
#define EMIFB_SDCFG_PAGESIZE_512W_PAGE (0x00000001u)
#define EMIFB_SDCFG_PAGESIZE_1024W_PAGE (0x00000002u)
#define EMIFB_SDCFG_PAGESIZE_2048W_PAGE (0x00000003u)


/* SDRFC */

#define EMIFB_SDRFC_LP_MODE     (0x80000000u)
#define EMIFB_SDRFC_LP_MODE_SHIFT    (0x0000001Fu)

#define EMIFB_SDRFC_MCLKSTOP_EN (0x40000000u)
#define EMIFB_SDRFC_MCLKSTOP_EN_SHIFT (0x0000001Eu)

#define EMIFB_SDRFC_SR_PD       (0x00800000u)
#define EMIFB_SDRFC_SR_PD_SHIFT      (0x00000017u)

#define EMIFB_SDRFC_REFRESH_RATE (0x0000FFFFu)
#define EMIFB_SDRFC_REFRESH_RATE_SHIFT (0x00000000u)


/* SDTIM1 */

#define EMIFB_SDTIM1_T_RFC      (0xFE000000u)
#define EMIFB_SDTIM1_T_RFC_SHIFT     (0x00000019u)

#define EMIFB_SDTIM1_T_RP       (0x01C00000u)
#define EMIFB_SDTIM1_T_RP_SHIFT      (0x00000016u)

#define EMIFB_SDTIM1_T_RCD      (0x00380000u)
#define EMIFB_SDTIM1_T_RCD_SHIFT     (0x00000013u)

#define EMIFB_SDTIM1_T_WR       (0x00070000u)
#define EMIFB_SDTIM1_T_WR_SHIFT      (0x00000010u)

#define EMIFB_SDTIM1_T_RAS      (0x0000F800u)
#define EMIFB_SDTIM1_T_RAS_SHIFT     (0x0000000Bu)

#define EMIFB_SDTIM1_T_RC       (0x000007C0u)
#define EMIFB_SDTIM1_T_RC_SHIFT      (0x00000006u)

#define EMIFB_SDTIM1_T_RRD      (0x00000038u)
#define EMIFB_SDTIM1_T_RRD_SHIFT     (0x00000003u)



/* SDTIM2 */


#define EMIFB_SDTIM2_T_RAS_MAX  (0x78000000u)
#define EMIFB_SDTIM2_T_RAS_MAX_SHIFT (0x0000001Bu)


#define EMIFB_SDTIM2_T_XSR      (0x007F0000u)
#define EMIFB_SDTIM2_T_XSR_SHIFT     (0x00000010u)


#define EMIFB_SDTIM2_T_CKE      (0x0000001Fu)
#define EMIFB_SDTIM2_T_CKE_SHIFT     (0x00000000u)


/* SDCFG2 */


#define EMIFB_SDCFG2_PASR       (0x00070000u)
#define EMIFB_SDCFG2_PASR_SHIFT      (0x00000010u)
/*----PASR Tokens----*/
#define EMIFB_SDCFG2_PASR_FOUR_BANK  (0x00000000u)
#define EMIFB_SDCFG2_PASR_TWO_BANK   (0x00000001u)
#define EMIFB_SDCFG2_PASR_ONE_BANK   (0x00000002u)
#define EMIFB_SDCFG2_PASR_HALF_BANK  (0x00000005u)
#define EMIFB_SDCFG2_PASR_QRTR_BANK  (0x00000006u)
#define EMIFB_SDCFG2_PASR_RSVD1      (0x00000007u)


#define EMIFB_SDCFG2_ROWSIZE    (0x00000007u)
#define EMIFB_SDCFG2_ROWSIZE_SHIFT   (0x00000000u)
/*----ROWSIZE Tokens----*/
#define EMIFB_SDCFG2_ROWSIZE_9BITS   (0x00000000u)
#define EMIFB_SDCFG2_ROWSIZE_10BITS  (0x00000001u)
#define EMIFB_SDCFG2_ROWSIZE_11BITS  (0x00000002u)
#define EMIFB_SDCFG2_ROWSIZE_12BITS  (0x00000003u)
#define EMIFB_SDCFG2_ROWSIZE_13BITS  (0x00000004u)
#define EMIFB_SDCFG2_ROWSIZE_14BITS  (0x00000005u)


/* BPRIO */


#define EMIFB_BPRIO_PRIO_RAISE  (0x000000FFu)
#define EMIFB_BPRIO_PRIO_RAISE_SHIFT (0x00000000u)


/* PC1 */

#define EMIFB_PC1_COUNTER1      (0xFFFFFFFFu)
#define EMIFB_PC1_COUNTER1_SHIFT     (0x00000000u)


/* PC2 */

#define EMIFB_PC2_COUNTER2      (0xFFFFFFFFu)
#define EMIFB_PC2_COUNTER2_SHIFT     (0x00000000u)


/* PCC */

#define EMIFB_PCC_CNTR2_MSTID_EN (0x80000000u)
#define EMIFB_PCC_CNTR2_MSTID_EN_SHIFT (0x0000001Fu)

#define EMIFB_PCC_CNTR2_REGION_EN (0x40000000u)
#define EMIFB_PCC_CNTR2_REGION_EN_SHIFT (0x0000001Eu)


#define EMIFB_PCC_CNTR2_CFG     (0x000F0000u)
#define EMIFB_PCC_CNTR2_CFG_SHIFT    (0x00000010u)

#define EMIFB_PCC_CNTR1_MSTID_EN (0x00008000u)
#define EMIFB_PCC_CNTR1_MSTID_EN_SHIFT (0x0000000Fu)

#define EMIFB_PCC_CNTR1_REGION_EN (0x00004000u)
#define EMIFB_PCC_CNTR1_REGION_EN_SHIFT (0x0000000Eu)


#define EMIFB_PCC_CNTR1_CFG     (0x0000000Fu)
#define EMIFB_PCC_CNTR1_CFG_SHIFT    (0x00000000u)


/* PCMRS */

#define EMIFB_PCMRS_MST_ID2     (0xFF000000u)
#define EMIFB_PCMRS_MST_ID2_SHIFT    (0x00000018u)


#define EMIFB_PCMRS_MST_ID1     (0x0000FF00u)
#define EMIFB_PCMRS_MST_ID1_SHIFT    (0x00000008u)



/* PCT */

#define EMIFB_PCT_TOTAL_TIME    (0xFFFFFFFFu)
#define EMIFB_PCT_TOTAL_TIME_SHIFT   (0x00000000u)


/* IRR */


#define EMIFB_IRR_LT            (0x00000004u)
#define EMIFB_IRR_LT_SHIFT           (0x00000002u)

/* IMR */


#define EMIFB_IMR_LTM           (0x00000004u)
#define EMIFB_IMR_LTM_SHIFT          (0x00000002u)

/* IMSR */


#define EMIFB_IMSR_LTMSET       (0x00000004u)
#define EMIFB_IMSR_LTMSET_SHIFT      (0x00000002u)

/* IMCR */


#define EMIFB_IMCR_LTMCLR       (0x00000004u)
#define EMIFB_IMCR_LTMCLR_SHIFT      (0x00000002u)

#ifdef __cplusplus
}
#endif

#endif
