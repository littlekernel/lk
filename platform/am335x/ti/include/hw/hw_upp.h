/*
 * hw_upp.h
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


#ifndef _HW_UPP_H_
#define _HW_UPP_H_

#ifdef __cplusplus
extern "C" {
#endif

#define UPP_UPPID	(0x0)
#define UPP_UPPCR	(0x4)
#define UPP_UPDLB	(0x8)
#define UPP_UPCTL	(0x10)
#define UPP_UPICR	(0x14)
#define UPP_UPIVR	(0x18)
#define UPP_UPTCR	(0x1C)
#define UPP_UPISR	(0x20)
#define UPP_UPIER	(0x24)
#define UPP_UPIES	(0x2C)
#define UPP_UPIEC	(0x30)
#define UPP_UPEOI	(0x40)
#define UPP_UPID0	(0x44)
#define UPP_UPID1	(0x48)
#define UPP_UPID2	(0x50)
#define UPP_UPIS0	(0x54)
#define UPP_UPIS1	(0x58)
#define UPP_UPIS2	(0x60)
#define UPP_UPQD0	(0x64)
#define UPP_UPQD1	(0x68)
#define UPP_UPQD2	(0x6C)
#define UPP_UPQS0	(0x70)
#define UPP_UPQS1	(0x74)
#define UPP_UPQS2	(0x78)


/**************************************************************************\
* Field Definition Macros
\**************************************************************************/

/* UPPID */

#define UPP_UPPID_REVID         (0xFFFFFFFFu)
#define UPP_UPPID_REVID_SHIFT        (0x00000000u)


/* UPPCR */


#define UPP_UPPCR_DB            (0x00000080u)
#define UPP_UPPCR_DB_SHIFT           (0x00000007u)

#define UPP_UPPCR_SWRST         (0x00000010u)
#define UPP_UPPCR_SWRST_SHIFT        (0x00000004u)

#define UPP_UPPCR_EN            (0x00000008u)
#define UPP_UPPCR_EN_SHIFT           (0x00000003u)

#define UPP_UPPCR_RTEMU         (0x00000004u)
#define UPP_UPPCR_RTEMU_SHIFT        (0x00000002u)

#define UPP_UPPCR_SOFT          (0x00000002u)
#define UPP_UPPCR_SOFT_SHIFT         (0x00000001u)

#define UPP_UPPCR_FREE          (0x00000001u)
#define UPP_UPPCR_FREE_SHIFT         (0x00000000u)


/* UPDLB */


#define UPP_UPDLB_BA            (0x00002000u)
#define UPP_UPDLB_BA_SHIFT           (0x0000000Du)

#define UPP_UPDLB_AB            (0x00001000u)
#define UPP_UPDLB_AB_SHIFT           (0x0000000Cu)

/* UPCTL */


#define UPP_UPCTL_DPFB          (0x60000000u)
#define UPP_UPCTL_DPFB_SHIFT         (0x0000001Du)
/*----DPFB Tokens----*/
#define UPP_UPCTL_DPFB_RJZE          (0x00000000u)
#define UPP_UPCTL_DPFB_RJSE          (0x00000001u)
#define UPP_UPCTL_DPFB_LJZF          (0x00000002u)
#define UPP_UPCTL_DPFB_RESERVED      (0x00000003u)

#define UPP_UPCTL_DPWB          (0x1C000000u)
#define UPP_UPCTL_DPWB_SHIFT         (0x0000001Au)
/*----DPWB Tokens----*/
#define UPP_UPCTL_DPWB_FULL          (0x00000000u)
#define UPP_UPCTL_DPWB_9BIT          (0x00000001u)
#define UPP_UPCTL_DPWB_10BIT         (0x00000002u)
#define UPP_UPCTL_DPWB_11BIT         (0x00000003u)
#define UPP_UPCTL_DPWB_12BIT         (0x00000004u)
#define UPP_UPCTL_DPWB_13BIT         (0x00000005u)
#define UPP_UPCTL_DPWB_14BIT         (0x00000006u)
#define UPP_UPCTL_DPWB_15BIT         (0x00000007u)

#define UPP_UPCTL_IWB           (0x02000000u)
#define UPP_UPCTL_IWB_SHIFT          (0x00000019u)

#define UPP_UPCTL_DRB           (0x01000000u)
#define UPP_UPCTL_DRB_SHIFT          (0x00000018u)

#define UPP_UPCTL_DPFA          (0x00600000u)
#define UPP_UPCTL_DPFA_SHIFT         (0x00000015u)
/*----DPFA Tokens----*/
#define UPP_UPCTL_DPFA_RJZE          (0x00000000u)
#define UPP_UPCTL_DPFA_RJSE          (0x00000001u)
#define UPP_UPCTL_DPFA_LJZF          (0x00000002u)
#define UPP_UPCTL_DPFA_RESERVED      (0x00000003u)

#define UPP_UPCTL_DPWA          (0x001C0000u)
#define UPP_UPCTL_DPWA_SHIFT         (0x00000012u)
/*----DPWA Tokens----*/
#define UPP_UPCTL_DPWA_FULL          (0x00000000u)
#define UPP_UPCTL_DPWA_9BIT          (0x00000001u)
#define UPP_UPCTL_DPWA_10BIT         (0x00000002u)
#define UPP_UPCTL_DPWA_11BIT         (0x00000003u)
#define UPP_UPCTL_DPWA_12BIT         (0x00000004u)
#define UPP_UPCTL_DPWA_13BIT         (0x00000005u)
#define UPP_UPCTL_DPWA_14BIT         (0x00000006u)
#define UPP_UPCTL_DPWA_15BIT         (0x00000007u)

#define UPP_UPCTL_IWA           (0x00020000u)
#define UPP_UPCTL_IWA_SHIFT          (0x00000011u)

#define UPP_UPCTL_DRA           (0x00010000u)
#define UPP_UPCTL_DRA_SHIFT          (0x00000010u)


#define UPP_UPCTL_DDRDEMUX      (0x00000010u)
#define UPP_UPCTL_DDRDEMUX_SHIFT     (0x00000004u)

#define UPP_UPCTL_SDRTXIL       (0x00000008u)
#define UPP_UPCTL_SDRTXIL_SHIFT      (0x00000003u)

#define UPP_UPCTL_CHN           (0x00000004u)
#define UPP_UPCTL_CHN_SHIFT          (0x00000002u)

#define UPP_UPCTL_MODE          (0x00000003u)
#define UPP_UPCTL_MODE_SHIFT         (0x00000000u)
/*----MODE Tokens----*/
#define UPP_UPCTL_MODE_RECEIVE       (0x00000000u)
#define UPP_UPCTL_MODE_TRANSMIT      (0x00000001u)
#define UPP_UPCTL_MODE_DUPLEX0       (0x00000002u)
#define UPP_UPCTL_MODE_DUPLEX1       (0x00000003u)


/* UPICR */


#define UPP_UPICR_TRISB         (0x20000000u)
#define UPP_UPICR_TRISB_SHIFT        (0x0000001Du)

#define UPP_UPICR_CLKINVB       (0x10000000u)
#define UPP_UPICR_CLKINVB_SHIFT      (0x0000001Cu)

#define UPP_UPICR_CLKDIVB       (0x0F000000u)
#define UPP_UPICR_CLKDIVB_SHIFT      (0x00000018u)


#define UPP_UPICR_WAITB         (0x00200000u)
#define UPP_UPICR_WAITB_SHIFT        (0x00000015u)

#define UPP_UPICR_ENAB          (0x00100000u)
#define UPP_UPICR_ENAB_SHIFT         (0x00000014u)

#define UPP_UPICR_STARTB        (0x00080000u)
#define UPP_UPICR_STARTB_SHIFT       (0x00000013u)

#define UPP_UPICR_WAITPOLB      (0x00040000u)
#define UPP_UPICR_WAITPOLB_SHIFT     (0x00000012u)

#define UPP_UPICR_ENAPOLB       (0x00020000u)
#define UPP_UPICR_ENAPOLB_SHIFT      (0x00000011u)

#define UPP_UPICR_STARTPOLB     (0x00010000u)
#define UPP_UPICR_STARTPOLB_SHIFT    (0x00000010u)


#define UPP_UPICR_TRISA         (0x00002000u)
#define UPP_UPICR_TRISA_SHIFT        (0x0000000Du)

#define UPP_UPICR_CLKINVA       (0x00001000u)
#define UPP_UPICR_CLKINVA_SHIFT      (0x0000000Cu)

#define UPP_UPICR_CLKDIVA       (0x00000F00u)
#define UPP_UPICR_CLKDIVA_SHIFT      (0x00000008u)


#define UPP_UPICR_WAITA         (0x00000020u)
#define UPP_UPICR_WAITA_SHIFT        (0x00000005u)

#define UPP_UPICR_ENAA          (0x00000010u)
#define UPP_UPICR_ENAA_SHIFT         (0x00000004u)

#define UPP_UPICR_STARTA        (0x00000008u)
#define UPP_UPICR_STARTA_SHIFT       (0x00000003u)

#define UPP_UPICR_WAITPOLA      (0x00000004u)
#define UPP_UPICR_WAITPOLA_SHIFT     (0x00000002u)

#define UPP_UPICR_ENAPOLA       (0x00000002u)
#define UPP_UPICR_ENAPOLA_SHIFT      (0x00000001u)

#define UPP_UPICR_STARTPOLA     (0x00000001u)
#define UPP_UPICR_STARTPOLA_SHIFT    (0x00000000u)


/* UPIVR */

#define UPP_UPIVR_VALB          (0xFFFF0000u)
#define UPP_UPIVR_VALB_SHIFT         (0x00000010u)

#define UPP_UPIVR_VALA          (0x0000FFFFu)
#define UPP_UPIVR_VALA_SHIFT         (0x00000000u)


/* UPTCR */


#define UPP_UPTCR_TXSIZEB       (0x03000000u)
#define UPP_UPTCR_TXSIZEB_SHIFT      (0x00000018u)
/*----TXSIZEB Tokens----*/
#define UPP_UPTCR_TXSIZEB_64B        (0x00000000u)
#define UPP_UPTCR_TXSIZEB_128B       (0x00000001u)
#define UPP_UPTCR_TXSIZEB_256B       (0x00000003u)


#define UPP_UPTCR_TXSIZEA       (0x00030000u)
#define UPP_UPTCR_TXSIZEA_SHIFT      (0x00000010u)
/*----TXSIZEA Tokens----*/
#define UPP_UPTCR_TXSIZEA_64B        (0x00000000u)
#define UPP_UPTCR_TXSIZEA_128B       (0x00000001u)
#define UPP_UPTCR_TXSIZEA_256B       (0x00000003u)


#define UPP_UPTCR_RDSIZEQ       (0x00000300u)
#define UPP_UPTCR_RDSIZEQ_SHIFT      (0x00000008u)
/*----RDSIZEQ Tokens----*/
#define UPP_UPTCR_RDSIZEQ_64B        (0x00000000u)
#define UPP_UPTCR_RDSIZEQ_128B       (0x00000001u)
#define UPP_UPTCR_RDSIZEQ_256B       (0x00000003u)


#define UPP_UPTCR_RDSIZEI       (0x00000003u)
#define UPP_UPTCR_RDSIZEI_SHIFT      (0x00000000u)
/*----RDSIZEI Tokens----*/
#define UPP_UPTCR_RDSIZEI_64B        (0x00000000u)
#define UPP_UPTCR_RDSIZEI_128B       (0x00000001u)
#define UPP_UPTCR_RDSIZEI_256B       (0x00000003u)


/* UPISR */


#define UPP_UPISR_EOLQ          (0x00001000u)
#define UPP_UPISR_EOLQ_SHIFT         (0x0000000Cu)

#define UPP_UPISR_EOWQ          (0x00000800u)
#define UPP_UPISR_EOWQ_SHIFT         (0x0000000Bu)

#define UPP_UPISR_ERRQ          (0x00000400u)
#define UPP_UPISR_ERRQ_SHIFT         (0x0000000Au)

#define UPP_UPISR_UORQ          (0x00000200u)
#define UPP_UPISR_UORQ_SHIFT         (0x00000009u)

#define UPP_UPISR_DPEQ          (0x00000100u)
#define UPP_UPISR_DPEQ_SHIFT         (0x00000008u)

#define UPP_UPISR_EOLI          (0x00000010u)
#define UPP_UPISR_EOLI_SHIFT         (0x00000004u)

#define UPP_UPISR_EOWI          (0x00000008u)
#define UPP_UPISR_EOWI_SHIFT         (0x00000003u)

#define UPP_UPISR_ERRI          (0x00000004u)
#define UPP_UPISR_ERRI_SHIFT         (0x00000002u)

#define UPP_UPISR_UORI          (0x00000002u)
#define UPP_UPISR_UORI_SHIFT         (0x00000001u)

#define UPP_UPISR_DPEI          (0x00000001u)
#define UPP_UPISR_DPEI_SHIFT         (0x00000000u)


/* UPIER */


#define UPP_UPIER_EOLQ          (0x00001000u)
#define UPP_UPIER_EOLQ_SHIFT         (0x0000000Cu)

#define UPP_UPIER_EOWQ          (0x00000800u)
#define UPP_UPIER_EOWQ_SHIFT         (0x0000000Bu)
#define UPP_UPIER_EOWQ_TRUE          (0x00000001u)

#define UPP_UPIER_ERRQ          (0x00000400u)
#define UPP_UPIER_ERRQ_SHIFT         (0x0000000Au)

#define UPP_UPIER_UORQ          (0x00000200u)
#define UPP_UPIER_UORQ_SHIFT         (0x00000009u)
#define UPP_UPIER_UORQ_TRUE          (0x00000001u)

#define UPP_UPIER_DPEQ          (0x00000100u)
#define UPP_UPIER_DPEQ_SHIFT         (0x00000008u)


#define UPP_UPIER_EOLI          (0x00000010u)
#define UPP_UPIER_EOLI_SHIFT         (0x00000004u)

#define UPP_UPIER_EOWI          (0x00000008u)
#define UPP_UPIER_EOWI_SHIFT         (0x00000003u)

#define UPP_UPIER_ERRI          (0x00000004u)
#define UPP_UPIER_ERRI_SHIFT         (0x00000002u)

#define UPP_UPIER_UORI          (0x00000002u)
#define UPP_UPIER_UORI_SHIFT         (0x00000001u)

#define UPP_UPIER_DPEI          (0x00000001u)
#define UPP_UPIER_DPEI_SHIFT         (0x00000000u)


/* UPIES */


#define UPP_UPIES_EOLQ          (0x00001000u)
#define UPP_UPIES_EOLQ_SHIFT         (0x0000000Cu)

#define UPP_UPIES_EOWQ          (0x00000800u)
#define UPP_UPIES_EOWQ_SHIFT         (0x0000000Bu)

#define UPP_UPIES_ERRQ          (0x00000400u)
#define UPP_UPIES_ERRQ_SHIFT         (0x0000000Au)

#define UPP_UPIES_UORQ          (0x00000200u)
#define UPP_UPIES_UORQ_SHIFT         (0x00000009u)

#define UPP_UPIES_DPEQ          (0x00000100u)
#define UPP_UPIES_DPEQ_SHIFT         (0x00000008u)


#define UPP_UPIES_EOLI          (0x00000010u)
#define UPP_UPIES_EOLI_SHIFT         (0x00000004u)

#define UPP_UPIES_EOWI          (0x00000008u)
#define UPP_UPIES_EOWI_SHIFT         (0x00000003u)

#define UPP_UPIES_ERRI          (0x00000004u)
#define UPP_UPIES_ERRI_SHIFT         (0x00000002u)

#define UPP_UPIES_UORI          (0x00000002u)
#define UPP_UPIES_UORI_SHIFT         (0x00000001u)

#define UPP_UPIES_DPEI          (0x00000001u)
#define UPP_UPIES_DPEI_SHIFT         (0x00000000u)


/* UPIEC */


#define UPP_UPIEC_EOLQ          (0x00001000u)
#define UPP_UPIEC_EOLQ_SHIFT         (0x0000000Cu)

#define UPP_UPIEC_EOWQ          (0x00000800u)
#define UPP_UPIEC_EOWQ_SHIFT         (0x0000000Bu)

#define UPP_UPIEC_ERRQ          (0x00000400u)
#define UPP_UPIEC_ERRQ_SHIFT         (0x0000000Au)

#define UPP_UPIEC_UORQ          (0x00000200u)
#define UPP_UPIEC_UORQ_SHIFT         (0x00000009u)

#define UPP_UPIEC_DPEQ          (0x00000100u)
#define UPP_UPIEC_DPEQ_SHIFT         (0x00000008u)


#define UPP_UPIEC_EOLI          (0x00000010u)
#define UPP_UPIEC_EOLI_SHIFT         (0x00000004u)

#define UPP_UPIEC_EOWI          (0x00000008u)
#define UPP_UPIEC_EOWI_SHIFT         (0x00000003u)

#define UPP_UPIEC_ERRI          (0x00000004u)
#define UPP_UPIEC_ERRI_SHIFT         (0x00000002u)

#define UPP_UPIEC_UORI          (0x00000002u)
#define UPP_UPIEC_UORI_SHIFT         (0x00000001u)

#define UPP_UPIEC_DPEI          (0x00000001u)
#define UPP_UPIEC_DPEI_SHIFT         (0x00000000u)


/* UPEOI */


#define UPP_UPEOI_EOI           (0x000000FFu)
#define UPP_UPEOI_EOI_SHIFT          (0x00000000u)


/* UPID0 */

#define UPP_UPID0_ADDRH         (0xFFFFFFF8u)
#define UPP_UPID0_ADDRH_SHIFT        (0x00000003u)

#define UPP_UPID0_ADDR          (0x00000007u)
#define UPP_UPID0_ADDR_SHIFT         (0x00000000u)


/* UPID1 */

#define UPP_UPID1_LNCNT         (0xFFFF0000u)
#define UPP_UPID1_LNCNT_SHIFT        (0x00000010u)

#define UPP_UPID1_BCNTH         (0x0000FFFEu)
#define UPP_UPID1_BCNTH_SHIFT        (0x00000001u)

#define UPP_UPID1_BCNT          (0x00000001u)
#define UPP_UPID1_BCNT_SHIFT         (0x00000000u)


/* UPID2 */


#define UPP_UPID2_LNOFFSETH     (0x0000FFF8u)
#define UPP_UPID2_LNOFFSETH_SHIFT    (0x00000003u)

#define UPP_UPID2_LNOFFSET      (0x00000007u)
#define UPP_UPID2_LNOFFSET_SHIFT     (0x00000000u)


/* UPIS0 */

#define UPP_UPIS0_ADDR          (0xFFFFFFFFu)
#define UPP_UPIS0_ADDR_SHIFT         (0x00000000u)


/* UPIS1 */

#define UPP_UPIS1_LNCNT         (0xFFFF0000u)
#define UPP_UPIS1_LNCNT_SHIFT        (0x00000010u)

#define UPP_UPIS1_BCNT          (0x0000FFFFu)
#define UPP_UPIS1_BCNT_SHIFT         (0x00000000u)


/* UPIS2 */


#define UPP_UPIS2_WM            (0x000000F0u)
#define UPP_UPIS2_WM_SHIFT           (0x00000004u)


#define UPP_UPIS2_PEND          (0x00000002u)
#define UPP_UPIS2_PEND_SHIFT         (0x00000001u)

#define UPP_UPIS2_ACT           (0x00000001u)
#define UPP_UPIS2_ACT_SHIFT          (0x00000000u)


/* UPQD0 */

#define UPP_UPQD0_ADDRH         (0xFFFFFFF8u)
#define UPP_UPQD0_ADDRH_SHIFT        (0x00000003u)

#define UPP_UPQD0_ADDR          (0x00000007u)
#define UPP_UPQD0_ADDR_SHIFT         (0x00000000u)


/* UPQD1 */

#define UPP_UPQD1_LNCNT         (0xFFFF0000u)
#define UPP_UPQD1_LNCNT_SHIFT        (0x00000010u)

#define UPP_UPQD1_BCNTH         (0x0000FFFEu)
#define UPP_UPQD1_BCNTH_SHIFT        (0x00000001u)

#define UPP_UPQD1_BCNT          (0x00000001u)
#define UPP_UPQD1_BCNT_SHIFT         (0x00000000u)


/* UPQD2 */


#define UPP_UPQD2_LNOFFSETH     (0x0000FFF8u)
#define UPP_UPQD2_LNOFFSETH_SHIFT    (0x00000003u)

#define UPP_UPQD2_LNOFFSET      (0x00000007u)
#define UPP_UPQD2_LNOFFSET_SHIFT     (0x00000000u)


/* UPQS0 */

#define UPP_UPQS0_ADDR          (0xFFFFFFFFu)
#define UPP_UPQS0_ADDR_SHIFT         (0x00000000u)


/* UPQS1 */

#define UPP_UPQS1_LNCNT         (0xFFFF0000u)
#define UPP_UPQS1_LNCNT_SHIFT        (0x00000010u)

#define UPP_UPQS1_BCNT          (0x0000FFFFu)
#define UPP_UPQS1_BCNT_SHIFT         (0x00000000u)


/* UPQS2 */


#define UPP_UPQS2_WM            (0x000000F0u)
#define UPP_UPQS2_WM_SHIFT           (0x00000004u)


#define UPP_UPQS2_PEND          (0x00000002u)
#define UPP_UPQS2_PEND_SHIFT         (0x00000001u)

#define UPP_UPQS2_ACT           (0x00000001u)
#define UPP_UPQS2_ACT_SHIFT          (0x00000000u)


#ifdef __cplusplus
}
#endif

#endif
