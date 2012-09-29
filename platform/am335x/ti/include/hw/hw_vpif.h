/**
 * \file  hw_vpif.h
 *
 * \brief Hardware registers and fields for VPIF module
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


#ifndef _HW_VPIF_H_
#define _HW_VPIF_H_

#ifdef __cplusplus
extern "C" {
#endif



#define REVID                   (0x0)
#define C0CTRL                  (0x4)
#define C1CTRL                  (0x8)
#define C2CTRL                  (0xC)
#define C3CTRL                  (0x10)
#define ERRSTAT                 (0x14)
#define INTEN                   (0x20)
#define INTSET                  (0x24)
#define INTCLR                  (0x28)
#define INTSTAT                 (0x2C)
#define INTSTATCLR              (0x30)
#define EMUCTRL                 (0x34)
#define REQSIZE                 (0x38)
/* Channel 0                     */
#define C0TLUMA                 (0x40)
#define C0BLUMA                 (0x44)
#define C0TCHROMA               (0x48)
#define C0BCHROMA               (0x4C)
#define C0THANC                 (0x50)
#define C0BHANC                 (0x54)
#define C0TVANC                 (0x58)
#define C0BVANC                 (0x5C)
#define C0IMGOFFSET             (0x64)
#define C0HANCOFFSET            (0x68)
#define C0HCFG                  (0x6C)
#define C0VCFG0                 (0x70)
#define C0VCFG1                 (0x74)
#define C0VCFG2                 (0x78)
#define C0VSIZE                 (0x7C)
/* Channel 1                     */
#define C1TLUMA                 (0x80)
#define C1BLUMA                 (0x84)
#define C1TCHROMA               (0x88)
#define C1BCHROMA               (0x8C)
#define C1THANC                 (0x90)
#define C1BHANC                 (0x94)
#define C1TVANC                 (0x98)
#define C1BVANC                 (0x9C)
#define C1IMGOFFSET             (0xA4)
#define C1HANCOFFSET            (0xA8)
#define C1HCFG                  (0xAC)
#define C1VCFG0                 (0xB0)
#define C1VCFG1                 (0xB4)
#define C1VCFG2                 (0xB8)
#define C1VSIZE                 (0xBC)
/* Channel 2                   */
#define C2TLUMA                 (0xC0)
#define C2BLUMA                 (0xC4)
#define C2TCHROMA               (0xC8)
#define C2BCHROMA               (0xCC)
#define C2THANC                 (0xD0)
#define C2BHANC                 (0xD4)
#define C2TVANC                 (0xD8)
#define C2BVANC                 (0xDC)
#define C2IMGOFFSET             (0xE4)
#define C2HANCOFFSET            (0xE8)
#define C2HCFG                  (0xEC)
#define C2VCFG0                 (0xF0)
#define C2VCFG1                 (0xF4)
#define C2VCFG2                 (0xF8)
#define C2VSIZE                 (0xFC)
#define C2THANCPOS              (0x100)
#define C2THANCSIZE             (0x104)
#define C2BHANCPOS              (0x108)
#define C2BHANCSIZE             (0x10C)
#define C2TVANCPOS              (0x110)
#define C2TVANCSIZE             (0x114)
#define C2BVANCPOS              (0x118)
#define C2BVANCSIZE             (0x11C)
/* Channel 3                                         */
#define C3TLUMA                 (0x140)
#define C3BLUMA                 (0x144)
#define C3TCHROMA               (0x148)
#define C3BCHROMA               (0x14C)
#define C3THANC                 (0x150)
#define C3BHANC                 (0x154)
#define C3TVANC                 (0x158)
#define C3BVANC                 (0x15C)
#define C3IMGOFFSET             (0x164)
#define C3HANCOFFSET            (0x168)
#define C3HCFG                  (0x16C)
#define C3VCFG0                 (0x170)
#define C3VCFG1                 (0x174)
#define C3VCFG2                 (0x178)
#define C3VSIZE                 (0x17C)
#define C3THANCPOS              (0x180)
#define C3THANCSIZE             (0x184)
#define C3BHANCPOS              (0x188)
#define C3BHANCSIZE             (0x18C)
#define C3TVANCPOS              (0x190)
#define C3TVANCSIZE             (0x194)
#define C3BVANCPOS              (0x198)
#define C3BVANCSIZE             (0x19C)

/**************************************************************************\
* Field Definition Macros
\**************************************************************************/

/* REVID */

#define VPIF_REVID_REV                          (0xFFFFFFFFu)
#define VPIF_REVID_REV_SHIFT                    (0x00000000u)

/* C0CTRL */

#define VPIF_C0CTRL_CLKEDGE                     (0x80000000u)
#define VPIF_C0CTRL_CLKEDGE_SHIFT               (0x0000001Fu)

#define VPIF_C0CTRL_DATAWIDTH                   (0x30000000u)
#define VPIF_C0CTRL_DATAWIDTH_SHIFT             (0x0000001Cu)
/*-----DATAWIDTH Tokens-----*/
#define VPIF_C0CTRL_DATAWIDTH_EIGHT_BPS         (0x00000000u)
#define VPIF_C0CTRL_DATAWIDTH_TEN_BPS           (0x00000001u)
#define VPIF_C0CTRL_DATAWIDTH_TWELVE_BPS        (0x00000002u)

#define VPIF_C0CTRL_INTLINE                     (0x0FFF0000u)
#define VPIF_C0CTRL_INTLINE_SHIFT               (0x00000010u)

#define VPIF_C0CTRL_FIDINV                      (0x00008000u)
#define VPIF_C0CTRL_FIDINV_SHIFT                (0x0000000Fu)

#define VPIF_C0CTRL_VVINV                       (0x00004000u)
#define VPIF_C0CTRL_VVINV_SHIFT                 (0x0000000Eu)

#define VPIF_C0CTRL_HVINV                       (0x00002000u)
#define VPIF_C0CTRL_HVINV_SHIFT                 (0x0000000Du)

#define VPIF_C0CTRL_FIELDFRAME                  (0x00001000u)
#define VPIF_C0CTRL_FIELDFRAME_SHIFT            (0x0000000Cu)

#define VPIF_C0CTRL_INTRPROG                    (0x00000400u)
#define VPIF_C0CTRL_INTRPROG_SHIFT              (0x0000000Au)

#define VPIF_C0CTRL_VANC                        (0x00000200u)
#define VPIF_C0CTRL_VANC_SHIFT                  (0x00000009u)

#define VPIF_C0CTRL_HANC                        (0x00000100u)
#define VPIF_C0CTRL_HANC_SHIFT                  (0x00000008u)

#define VPIF_C0CTRL_INTFRAME                    (0x000000C0u)
#define VPIF_C0CTRL_INTFRAME_SHIFT              (0x00000006u)
/*-----INTFRAME Tokens-----*/
#define VPIF_C0CTRL_INTFRAME_TOP                (0x00000000u)
#define VPIF_C0CTRL_INTFRAME_BOTTOM             (0x00000001u)
#define VPIF_C0CTRL_INTFRAME_TOP_BOTTOM         (0x00000002u)

#define VPIF_C0CTRL_FID                         (0x00000020u)
#define VPIF_C0CTRL_FID_SHIFT                   (0x00000005u)

#define VPIF_C0CTRL_YCMUX                       (0x00000008u)
#define VPIF_C0CTRL_YCMUX_SHIFT                 (0x00000003u)

#define VPIF_C0CTRL_CAPMODE                     (0x00000004u)
#define VPIF_C0CTRL_CAPMODE_SHIFT               (0x00000002u)

#define VPIF_C0CTRL_CHANEN                      (0x00000001u)
#define VPIF_C0CTRL_CHANEN_SHIFT                (0x00000000u)

/* C1CTRL */

#define VPIF_C1CTRL_CLKEDGE                     (0x80000000u)
#define VPIF_C1CTRL_CLKEDGE_SHIFT               (0x0000001Fu)

#define VPIF_C1CTRL_INTRPROG                    (0x00000400u)
#define VPIF_C1CTRL_INTRPROG_SHIFT              (0x0000000Au)

#define VPIF_C1CTRL_VANC                        (0x00000200u)
#define VPIF_C1CTRL_VANC_SHIFT                  (0x00000009u)

#define VPIF_C1CTRL_HANC                        (0x00000100u)
#define VPIF_C1CTRL_HANC_SHIFT                  (0x00000008u)

#define VPIF_C1CTRL_INTFRAME                    (0x000000C0u)
#define VPIF_C1CTRL_INTFRAME_SHIFT              (0x00000006u)
/*-----INTFRAME Tokens-----*/
#define VPIF_C1CTRL_INTFRAME_TOP                (0x00000000u)
#define VPIF_C1CTRL_INTFRAME_BOTTOM             (0x00000001u)
#define VPIF_C1CTRL_INTFRAME_TOP_BOTTOM         (0x00000002u)

#define VPIF_C1CTRL_FID                         (0x00000020u)
#define VPIF_C1CTRL_FID_SHIFT                   (0x00000005u)

#define VPIF_C1CTRL_YCMUX                       (0x00000008u)
#define VPIF_C1CTRL_YCMUX_SHIFT                 (0x00000003u)

#define VPIF_C1CTRL_CAPMODE                     (0x00000004u)
#define VPIF_C1CTRL_CAPMODE_SHIFT               (0x00000002u)

#define VPIF_C1CTRL_CHANEN                      (0x00000001u)
#define VPIF_C1CTRL_CHANEN_SHIFT                (0x00000000u)

/* C2CTRL */

#define VPIF_C2CTRL_CLKEDGE                     (0x80000000u)
#define VPIF_C2CTRL_CLKEDGE_SHIFT               (0x0000001Fu)

#define VPIF_C2CTRL_CLIPANC                     (0x00004000u)
#define VPIF_C2CTRL_CLIPANC_SHIFT               (0x0000000Eu)

#define VPIF_C2CTRL_CLIPVID                     (0x00002000u)
#define VPIF_C2CTRL_CLIPVID_SHIFT               (0x0000000Du)

#define VPIF_C2CTRL_FIELDFRAME                  (0x00001000u)
#define VPIF_C2CTRL_FIELDFRAME_SHIFT            (0x0000000Cu)

#define VPIF_C2CTRL_INTRPROG                    (0x00000800u)
#define VPIF_C2CTRL_INTRPROG_SHIFT              (0x0000000Bu)

#define VPIF_C2CTRL_PIXEL                       (0x00000400u)
#define VPIF_C2CTRL_PIXEL_SHIFT                 (0x0000000Au)

#define VPIF_C2CTRL_VANC                        (0x00000200u)
#define VPIF_C2CTRL_VANC_SHIFT                  (0x00000009u)

#define VPIF_C2CTRL_HANC                        (0x00000100u)
#define VPIF_C2CTRL_HANC_SHIFT                  (0x00000008u)

#define VPIF_C2CTRL_INTFRAME                    (0x000000C0u)
#define VPIF_C2CTRL_INTFRAME_SHIFT              (0x00000006u)
/*-----INTFRAME Tokens-----*/
#define VPIF_C2CTRL_INTFRAME_TOP                (0x00000000u)
#define VPIF_C2CTRL_INTFRAME_BOTTOM             (0x00000001u)
#define VPIF_C2CTRL_INTFRAME_TOP_BOTTOM         (0x00000002u)

#define VPIF_C2CTRL_FID                         (0x00000020u)
#define VPIF_C2CTRL_FID_SHIFT                   (0x00000005u)

#define VPIF_C2CTRL_YCMUX                       (0x00000008u)
#define VPIF_C2CTRL_YCMUX_SHIFT                 (0x00000003u)

#define VPIF_C2CTRL_CLKEN                       (0x00000002u)
#define VPIF_C2CTRL_CLKEN_SHIFT                 (0x00000001u)

#define VPIF_C2CTRL_CHANEN                      (0x00000001u)
#define VPIF_C2CTRL_CHANEN_SHIFT                (0x00000000u)

/* C3CTRL */

#define VPIF_C3CTRL_CLKEDGE                     (0x80000000u)
#define VPIF_C3CTRL_CLKEDGE_SHIFT               (0x0000001Fu)

#define VPIF_C3CTRL_CLIPANC                     (0x00004000u)
#define VPIF_C3CTRL_CLIPANC_SHIFT               (0x0000000Eu)

#define VPIF_C3CTRL_CLIPVID                     (0x00002000u)
#define VPIF_C3CTRL_CLIPVID_SHIFT               (0x0000000Du)

#define VPIF_C3CTRL_INTRPROG                    (0x00000800u)
#define VPIF_C3CTRL_INTRPROG_SHIFT              (0x0000000Bu)

#define VPIF_C3CTRL_PIXEL                       (0x00000400u)
#define VPIF_C3CTRL_PIXEL_SHIFT                 (0x0000000Au)

#define VPIF_C3CTRL_VANC                        (0x00000200u)
#define VPIF_C3CTRL_VANC_SHIFT                  (0x00000009u)

#define VPIF_C3CTRL_HANC                        (0x00000100u)
#define VPIF_C3CTRL_HANC_SHIFT                  (0x00000008u)

#define VPIF_C3CTRL_INTFRAME                    (0x000000C0u)
#define VPIF_C3CTRL_INTFRAME_SHIFT              (0x00000006u)
/*-----INTFRAME Tokens-----*/
#define VPIF_C3CTRL_INTFRAME_TOP                (0x00000000u)
#define VPIF_C3CTRL_INTFRAME_BOTTOM             (0x00000001u)
#define VPIF_C3CTRL_INTFRAME_TOP_BOTTOM         (0x00000002u)

#define VPIF_C3CTRL_FID                         (0x00000020u)
#define VPIF_C3CTRL_FID_SHIFT                   (0x00000005u)

#define VPIF_C3CTRL_YCMUX                       (0x00000008u)
#define VPIF_C3CTRL_YCMUX_SHIFT                 (0x00000003u)

#define VPIF_C3CTRL_CLKEN                       (0x00000002u)
#define VPIF_C3CTRL_CLKEN_SHIFT                 (0x00000001u)

#define VPIF_C3CTRL_CHANEN                      (0x00000001u)
#define VPIF_C3CTRL_CHANEN_SHIFT                (0x00000000u)

/* ERRSTAT */

#define VPIF_ERRSTAT_UNDERFLOW3                 (0x00000080u)
#define VPIF_ERRSTAT_UNDERFLOW3_SHIFT           (0x00000007u)

#define VPIF_ERRSTAT_UNDERFLOW2                 (0x00000040u)
#define VPIF_ERRSTAT_UNDERFLOW2_SHIFT           (0x00000006u)

#define VPIF_ERRSTAT_OVERFLOW1                  (0x00000020u)
#define VPIF_ERRSTAT_OVERFLOW1_SHIFT            (0x00000005u)

#define VPIF_ERRSTAT_OVERFLOW0                  (0x00000010u)
#define VPIF_ERRSTAT_OVERFLOW0_SHIFT            (0x00000004u)

#define VPIF_ERRSTAT_SYNCDISTERR1               (0x00000008u)
#define VPIF_ERRSTAT_SYNCDISTERR1_SHIFT         (0x00000003u)

#define VPIF_ERRSTAT_SYNCDISTERR0               (0x00000004u)
#define VPIF_ERRSTAT_SYNCDISTERR0_SHIFT         (0x00000002u)

#define VPIF_ERRSTAT_BITERR1                    (0x00000002u)
#define VPIF_ERRSTAT_BITERR1_SHIFT              (0x00000001u)

#define VPIF_ERRSTAT_BITERR0                    (0x00000001u)
#define VPIF_ERRSTAT_BITERR0_SHIFT              (0x00000000u)

/* INTEN */

#define VPIF_INTEN_ERROR                        (0x00000010u)
#define VPIF_INTEN_ERROR_SHIFT                  (0x00000004u)

#define VPIF_INTEN_FRAME3                       (0x00000008u)
#define VPIF_INTEN_FRAME3_SHIFT                 (0x00000003u)

#define VPIF_INTEN_FRAME2                       (0x00000004u)
#define VPIF_INTEN_FRAME2_SHIFT                 (0x00000002u)

#define VPIF_INTEN_FRAME1                       (0x00000002u)
#define VPIF_INTEN_FRAME1_SHIFT                 (0x00000001u)

#define VPIF_INTEN_FRAME0                       (0x00000001u)
#define VPIF_INTEN_FRAME0_SHIFT                 (0x00000000u)

/* INTSET */

#define VPIF_INTSET_ERROR                       (0x00000010u)
#define VPIF_INTSET_ERROR_SHIFT                 (0x00000004u)

#define VPIF_INTSET_FRAME3                      (0x00000008u)
#define VPIF_INTSET_FRAME3_SHIFT                (0x00000003u)

#define VPIF_INTSET_FRAME2                      (0x00000004u)
#define VPIF_INTSET_FRAME2_SHIFT                (0x00000002u)

#define VPIF_INTSET_FRAME1                      (0x00000002u)
#define VPIF_INTSET_FRAME1_SHIFT                (0x00000001u)

#define VPIF_INTSET_FRAME0                      (0x00000001u)
#define VPIF_INTSET_FRAME0_SHIFT                (0x00000000u)

/* INTCLR */

#define VPIF_INTCLR_ERROR                       (0x00000010u)
#define VPIF_INTCLR_ERROR_SHIFT                 (0x00000004u)

#define VPIF_INTCLR_FRAME3                      (0x00000008u)
#define VPIF_INTCLR_FRAME3_SHIFT                (0x00000003u)

#define VPIF_INTCLR_FRAME2                      (0x00000004u)
#define VPIF_INTCLR_FRAME2_SHIFT                (0x00000002u)

#define VPIF_INTCLR_FRAME1                      (0x00000002u)
#define VPIF_INTCLR_FRAME1_SHIFT                (0x00000001u)

#define VPIF_INTCLR_FRAME0                      (0x00000001u)
#define VPIF_INTCLR_FRAME0_SHIFT                (0x00000000u)

/* INTSTAT */

#define VPIF_INTSTAT_ERROR                      (0x00000010u)
#define VPIF_INTSTAT_ERROR_SHIFT                (0x00000004u)

#define VPIF_INTSTAT_FRAME3                     (0x00000008u)
#define VPIF_INTSTAT_FRAME3_SHIFT               (0x00000003u)

#define VPIF_INTSTAT_FRAME2                     (0x00000004u)
#define VPIF_INTSTAT_FRAME2_SHIFT               (0x00000002u)

#define VPIF_INTSTAT_FRAME1                     (0x00000002u)
#define VPIF_INTSTAT_FRAME1_SHIFT               (0x00000001u)

#define VPIF_INTSTAT_FRAME0                     (0x00000001u)
#define VPIF_INTSTAT_FRAME0_SHIFT               (0x00000000u)

/* INTSTATCLR */

#define VPIF_INTSTATCLR_ERROR                   (0x00000010u)
#define VPIF_INTSTATCLR_ERROR_SHIFT             (0x00000004u)

#define VPIF_INTSTATCLR_FRAME3                  (0x00000008u)
#define VPIF_INTSTATCLR_FRAME3_SHIFT            (0x00000003u)

#define VPIF_INTSTATCLR_FRAME2                  (0x00000004u)
#define VPIF_INTSTATCLR_FRAME2_SHIFT            (0x00000002u)

#define VPIF_INTSTATCLR_FRAME1                  (0x00000002u)
#define VPIF_INTSTATCLR_FRAME1_SHIFT            (0x00000001u)

#define VPIF_INTSTATCLR_FRAME0                  (0x00000001u)
#define VPIF_INTSTATCLR_FRAME0_SHIFT            (0x00000000u)

/* EMUCTRL */

#define VPIF_EMUCTRL_FREE                       (0x00000001u)
#define VPIF_EMUCTRL_FREE_SHIFT                 (0x00000000u)

/* REQSIZE */

#define VPIF_REQSIZE_BYTES                      (0x000001FFu)
#define VPIF_REQSIZE_BYTES_SHIFT                (0x00000000u)
/*-----BYTES Tokens-----*/
#define VPIF_REQSIZE_BYTES_THIRTY_TWO           (0x00000020u)
#define VPIF_REQSIZE_BYTES_SIXTY_FOUR           (0x00000040u)
#define VPIF_REQSIZE_BYTES_ONE_TWENTY_EIGHT     (0x00000080u)
#define VPIF_REQSIZE_BYTES_TWO_FIFTY_SIX        (0x00000100u)

/* Channel 0 */

/* C0TLUMA */

#define VPIF_C0TLUMA_C0TLUMA                    (0xFFFFFFFFu)
#define VPIF_C0TLUMA_C0TLUMA_SHIFT              (0x00000000u)

/* C0BLUMA */

#define VPIF_C0BLUMA_C0BLUMA                    (0xFFFFFFFFu)
#define VPIF_C0BLUMA_C0BLUMA_SHIFT              (0x00000000u)

/* C0TCHROMA */

#define VPIF_C0TCHROMA_C0TCHROMA                (0xFFFFFFFFu)
#define VPIF_C0TCHROMA_C0TCHROMA_SHIFT          (0x00000000u)

/* C0BCHROMA */

#define VPIF_C0BCHROMA_C0BCHROMA                (0xFFFFFFFFu)
#define VPIF_C0BCHROMA_C0BCHROMA_SHIFT          (0x00000000u)

/* C0THANC */

#define VPIF_C0THANC_C0THANC                    (0xFFFFFFFFu)
#define VPIF_C0THANC_C0THANC_SHIFT              (0x00000000u)

/* C0BHANC */

#define VPIF_C0BHANC_C0BHANC                    (0xFFFFFFFFu)
#define VPIF_C0BHANC_C0BHANC_SHIFT              (0x00000000u)

/* C0TVANC */

#define VPIF_C0TVANC_C0TVANC                    (0xFFFFFFFFu)
#define VPIF_C0TVANC_C0TVANC_SHIFT              (0x00000000u)

/* C0BVANC */

#define VPIF_C0BVANC_C0BVANC                    (0xFFFFFFFFu)
#define VPIF_C0BVANC_C0BVANC_SHIFT              (0x00000000u)

/* C0IMGOFFSET */

#define VPIF_C0IMGOFFSET_C0IMGOFFSET            (0xFFFFFFFFu)
#define VPIF_C0IMGOFFSET_C0IMGOFFSET_SHIFT      (0x00000000u)

/* C0HANCOFFSET */

#define VPIF_C0HANCOFFSET_C0HANCOFFSET          (0xFFFFFFFFu)
#define VPIF_C0HANCOFFSET_C0HANCOFFSET_SHIFT    (0x00000000u)

/* C0HCFG */

#define VPIF_C0HCFG_EAV2SAV                     (0x1FFF0000u)
#define VPIF_C0HCFG_EAV2SAV_SHIFT               (0x00000010u)

#define VPIF_C0HCFG_SAV2EAV                     (0x00001FFFu)
#define VPIF_C0HCFG_SAV2EAV_SHIFT               (0x00000000u)

/* C0VCFG0 */

#define VPIF_C0VCFG0_L1                         (0x0FFF0000u)
#define VPIF_C0VCFG0_L1_SHIFT                   (0x00000010u)

#define VPIF_C0VCFG0_L3                         (0x00000FFFu)
#define VPIF_C0VCFG0_L3_SHIFT                   (0x00000000u)

/* C0VCFG1 */

#define VPIF_C0VCFG1_L5                         (0x0FFF0000u)
#define VPIF_C0VCFG1_L5_SHIFT                   (0x00000010u)

#define VPIF_C0VCFG1_L7                         (0x00000FFFu)
#define VPIF_C0VCFG1_L7_SHIFT                   (0x00000000u)

/* C0VCFG2 */

#define VPIF_C0VCFG2_L9                         (0x0FFF0000u)
#define VPIF_C0VCFG2_L9_SHIFT                   (0x00000010u)

#define VPIF_C0VCFG2_L11                        (0x00000FFFu)
#define VPIF_C0VCFG2_L11_SHIFT                  (0x00000000u)

/* C0VSIZE */

#define VPIF_C0VSIZE_VSIZE                      (0x00000FFFu)
#define VPIF_C0VSIZE_VSIZE_SHIFT                (0x00000000u)

/* Channel 1 */

/* C1TLUMA */

#define VPIF_C1TLUMA_C1TLUMA                    (0xFFFFFFFFu)
#define VPIF_C1TLUMA_C1TLUMA_SHIFT              (0x00000000u)

/* C1BLUMA */

#define VPIF_C1BLUMA_C1BLUMA                    (0xFFFFFFFFu)
#define VPIF_C1BLUMA_C1BLUMA_SHIFT              (0x00000000u)

/* C1TCHROMA */

#define VPIF_C1TCHROMA_C1TCHROMA                (0xFFFFFFFFu)
#define VPIF_C1TCHROMA_C1TCHROMA_SHIFT          (0x00000000u)

/* C1BCHROMA */

#define VPIF_C1BCHROMA_C1BCHROMA                (0xFFFFFFFFu)
#define VPIF_C1BCHROMA_C1BCHROMA_SHIFT          (0x00000000u)

/* C1THANC */

#define VPIF_C1THANC_C1THANC                    (0xFFFFFFFFu)
#define VPIF_C1THANC_C1THANC_SHIFT              (0x00000000u)

/* C1BHANC */

#define VPIF_C1BHANC_C1BHANC                    (0xFFFFFFFFu)
#define VPIF_C1BHANC_C1BHANC_SHIFT              (0x00000000u)

/* C1TVANC */

#define VPIF_C1TVANC_C1TVANC                    (0xFFFFFFFFu)
#define VPIF_C1TVANC_C1TVANC_SHIFT              (0x00000000u)

/* C1BVANC */

#define VPIF_C1BVANC_C1BVANC                    (0xFFFFFFFFu)
#define VPIF_C1BVANC_C1BVANC_SHIFT              (0x00000000u)

/* C1IMGOFFSET */

#define VPIF_C1IMGOFFSET_C1IMGOFFSET            (0xFFFFFFFFu)
#define VPIF_C1IMGOFFSET_C1IMGOFFSET_SHIFT      (0x00000000u)

/* C1HANCOFFSET */

#define VPIF_C1HANCOFFSET_C1HANCOFFSET          (0xFFFFFFFFu)
#define VPIF_C1HANCOFFSET_C1HANCOFFSET_SHIFT    (0x00000000u)

/* C1HCFG */

#define VPIF_C1HCFG_EAV2SAV                     (0x1FFF0000u)
#define VPIF_C1HCFG_EAV2SAV_SHIFT               (0x00000010u)

#define VPIF_C1HCFG_SAV2EAV                     (0x00001FFFu)
#define VPIF_C1HCFG_SAV2EAV_SHIFT               (0x00000000u)

/* C1VCFG0 */

#define VPIF_C1VCFG0_L1                         (0x0FFF0000u)
#define VPIF_C1VCFG0_L1_SHIFT                   (0x00000010u)

#define VPIF_C1VCFG0_L3                         (0x00000FFFu)
#define VPIF_C1VCFG0_L3_SHIFT                   (0x00000000u)

/* C1VCFG1 */

#define VPIF_C1VCFG1_L5                         (0x0FFF0000u)
#define VPIF_C1VCFG1_L5_SHIFT                   (0x00000010u)

#define VPIF_C1VCFG1_L7                         (0x00000FFFu)
#define VPIF_C1VCFG1_L7_SHIFT                   (0x00000000u)

/* C1VCFG2 */

#define VPIF_C1VCFG2_L9                         (0x0FFF0000u)
#define VPIF_C1VCFG2_L9_SHIFT                   (0x00000010u)

#define VPIF_C1VCFG2_L11                        (0x00000FFFu)
#define VPIF_C1VCFG2_L11_SHIFT                  (0x00000000u)

/* C1VSIZE */

#define VPIF_C1VSIZE_VSIZE                      (0x00000FFFu)
#define VPIF_C1VSIZE_VSIZE_SHIFT                (0x00000000u)

/* Channel 2 */

/* C2TLUMA */

#define VPIF_C2TLUMA_C2TLUMA                    (0xFFFFFFFFu)
#define VPIF_C2TLUMA_C2TLUMA_SHIFT              (0x00000000u)

/* C2BLUMA */

#define VPIF_C2BLUMA_C2BLUMA                    (0xFFFFFFFFu)
#define VPIF_C2BLUMA_C2BLUMA_SHIFT              (0x00000000u)

/* C2TCHROMA */

#define VPIF_C2TCHROMA_C2TCHROMA                (0xFFFFFFFFu)
#define VPIF_C2TCHROMA_C2TCHROMA_SHIFT          (0x00000000u)

/* C2BCHROMA */

#define VPIF_C2BCHROMA_C2BCHROMA                (0xFFFFFFFFu)
#define VPIF_C2BCHROMA_C2BCHROMA_SHIFT          (0x00000000u)

/* C2THANC */

#define VPIF_C2THANC_C2THANC                    (0xFFFFFFFFu)
#define VPIF_C2THANC_C2THANC_SHIFT              (0x00000000u)

/* C2BHANC */

#define VPIF_C2BHANC_C2BHANC                    (0xFFFFFFFFu)
#define VPIF_C2BHANC_C2BHANC_SHIFT              (0x00000000u)

/* C2TVANC */

#define VPIF_C2TVANC_C2TVANC                    (0xFFFFFFFFu)
#define VPIF_C2TVANC_C2TVANC_SHIFT              (0x00000000u)

/* C2BVANC */

#define VPIF_C2BVANC_C2BVANC                    (0xFFFFFFFFu)
#define VPIF_C2BVANC_C2BVANC_SHIFT              (0x00000000u)

/* C2IMGOFFSET */

#define VPIF_C2IMGOFFSET_C2IMGOFFSET            (0xFFFFFFFFu)
#define VPIF_C2IMGOFFSET_C2IMGOFFSET_SHIFT      (0x00000000u)

/* C2HANCOFFSET */

#define VPIF_C2HANCOFFSET_C2HANCOFFSET          (0xFFFFFFFFu)
#define VPIF_C2HANCOFFSET_C2HANCOFFSET_SHIFT    (0x00000000u)

/* C2HCFG */

#define VPIF_C2HCFG_EAV2SAV                     (0x07FF0000u)
#define VPIF_C2HCFG_EAV2SAV_SHIFT               (0x00000010u)

#define VPIF_C2HCFG_SAV2EAV                     (0x000007FFu)
#define VPIF_C2HCFG_SAV2EAV_SHIFT               (0x00000000u)

/* C2VCFG0 */

#define VPIF_C2VCFG0_L1                         (0x07FF0000u)
#define VPIF_C2VCFG0_L1_SHIFT                   (0x00000010u)

#define VPIF_C2VCFG0_L3                         (0x000007FFu)
#define VPIF_C2VCFG0_L3_SHIFT                   (0x00000000u)

/* C2VCFG1 */

#define VPIF_C2VCFG1_L5                         (0x07FF0000u)
#define VPIF_C2VCFG1_L5_SHIFT                   (0x00000010u)

#define VPIF_C2VCFG1_L7                         (0x000007FFu)
#define VPIF_C2VCFG1_L7_SHIFT                   (0x00000000u)

/* C2VCFG2 */

#define VPIF_C2VCFG2_L9                         (0x07FF0000u)
#define VPIF_C2VCFG2_L9_SHIFT                   (0x00000010u)

#define VPIF_C2VCFG2_L11                        (0x000007FFu)
#define VPIF_C2VCFG2_L11_SHIFT                  (0x00000000u)

/* C2VSIZE */

#define VPIF_C2VSIZE_VSIZE                      (0x000007FFu)
#define VPIF_C2VSIZE_VSIZE_SHIFT                (0x00000000u)

/* C2THANCPOS */

#define VPIF_C2THANCPOS_VPOS                    (0x07FF0000u)
#define VPIF_C2THANCPOS_VPOS_SHIFT              (0x00000010u)

#define VPIF_C2THANCPOS_HPOS                    (0x000007FFu)
#define VPIF_C2THANCPOS_HPOS_SHIFT              (0x00000000u)

/* C2THANCSIZE */

#define VPIF_C2THANCSIZE_VSIZE                  (0x07FF0000u)
#define VPIF_C2THANCSIZE_VSIZE_SHIFT            (0x00000010u)

#define VPIF_C2THANCSIZE_HSIZE                  (0x000007FFu)
#define VPIF_C2THANCSIZE_HSIZE_SHIFT            (0x00000000u)

/* C2BHANCPOS */

#define VPIF_C2BHANCPOS_VPOS                    (0x07FF0000u)
#define VPIF_C2BHANCPOS_VPOS_SHIFT              (0x00000010u)

#define VPIF_C2BHANCPOS_HPOS                    (0x000007FFu)
#define VPIF_C2BHANCPOS_HPOS_SHIFT              (0x00000000u)

/* C2BHANCSIZE */

#define VPIF_C2BHANCSIZE_VSIZE                  (0x07FF0000u)
#define VPIF_C2BHANCSIZE_VSIZE_SHIFT            (0x00000010u)

#define VPIF_C2BHANCSIZE_HSIZE                  (0x000007FFu)
#define VPIF_C2BHANCSIZE_HSIZE_SHIFT            (0x00000000u)

/* C2TVANCPOS */

#define VPIF_C2TVANCPOS_VPOS                    (0x07FF0000u)
#define VPIF_C2TVANCPOS_VPOS_SHIFT              (0x00000010u)

#define VPIF_C2TVANCPOS_HPOS                    (0x000007FFu)
#define VPIF_C2TVANCPOS_HPOS_SHIFT              (0x00000000u)

/* C2TVANCSIZE */

#define VPIF_C2TVANCSIZE_VSIZE                  (0x07FF0000u)
#define VPIF_C2TVANCSIZE_VSIZE_SHIFT            (0x00000010u)

#define VPIF_C2TVANCSIZE_HSIZE                  (0x000007FFu)
#define VPIF_C2TVANCSIZE_HSIZE_SHIFT            (0x00000000u)

/* C2BVANCPOS */

#define VPIF_C2BVANCPOS_VPOS                    (0x07FF0000u)
#define VPIF_C2BVANCPOS_VPOS_SHIFT              (0x00000010u)

#define VPIF_C2BVANCPOS_HPOS                    (0x000007FFu)
#define VPIF_C2BVANCPOS_HPOS_SHIFT              (0x00000000u)

/* C2BVANCSIZE */

#define VPIF_C2BVANCSIZE_VSIZE                  (0x07FF0000u)
#define VPIF_C2BVANCSIZE_VSIZE_SHIFT            (0x00000010u)

#define VPIF_C2BVANCSIZE_HSIZE                  (0x000007FFu)
#define VPIF_C2BVANCSIZE_HSIZE_SHIFT            (0x00000000u)

/* Channel 3                   */

/* C3TLUMA */

#define VPIF_C3TLUMA_C3TLUMA                    (0xFFFFFFFFu)
#define VPIF_C3TLUMA_C3TLUMA_SHIFT              (0x00000000u)

/* C3BLUMA */

#define VPIF_C3BLUMA_C3BLUMA                    (0xFFFFFFFFu)
#define VPIF_C3BLUMA_C3BLUMA_SHIFT              (0x00000000u)

/* C3TCHROMA */

#define VPIF_C3TCHROMA_C3TCHROMA                (0xFFFFFFFFu)
#define VPIF_C3TCHROMA_C3TCHROMA_SHIFT          (0x00000000u)

/* C3BCHROMA */

#define VPIF_C3BCHROMA_C3BCHROMA                (0xFFFFFFFFu)
#define VPIF_C3BCHROMA_C3BCHROMA_SHIFT          (0x00000000u)

/* C3THANC */

#define VPIF_C3THANC_C3THANC                    (0xFFFFFFFFu)
#define VPIF_C3THANC_C3THANC_SHIFT              (0x00000000u)

/* C3BHANC */

#define VPIF_C3BHANC_C3BHANC                    (0xFFFFFFFFu)
#define VPIF_C3BHANC_C3BHANC_SHIFT              (0x00000000u)

/* C3TVANC */

#define VPIF_C3TVANC_C3TVANC                    (0xFFFFFFFFu)
#define VPIF_C3TVANC_C3TVANC_SHIFT              (0x00000000u)

/* C3BVANC */

#define VPIF_C3BVANC_C3BVANC                    (0xFFFFFFFFu)
#define VPIF_C3BVANC_C3BVANC_SHIFT              (0x00000000u)

/* C3IMGOFFSET */

#define VPIF_C3IMGOFFSET_C3IMGOFFSET            (0xFFFFFFFFu)
#define VPIF_C3IMGOFFSET_C3IMGOFFSET_SHIFT      (0x00000000u)

/* C3HANCOFFSET */

#define VPIF_C3HANCOFFSET_C3HANCOFFSET          (0xFFFFFFFFu)
#define VPIF_C3HANCOFFSET_C3HANCOFFSET_SHIFT    (0x00000000u)

/* C3HCFG */

#define VPIF_C3HCFG_EAV2SAV                     (0x07FF0000u)
#define VPIF_C3HCFG_EAV2SAV_SHIFT               (0x00000010u)

#define VPIF_C3HCFG_SAV2EAV                     (0x000007FFu)
#define VPIF_C3HCFG_SAV2EAV_SHIFT               (0x00000000u)

/* C3VCFG0 */

#define VPIF_C3VCFG0_L1                         (0x07FF0000u)
#define VPIF_C3VCFG0_L1_SHIFT                   (0x00000010u)

#define VPIF_C3VCFG0_L3                         (0x000007FFu)
#define VPIF_C3VCFG0_L3_SHIFT                   (0x00000000u)

/* C3VCFG1 */

#define VPIF_C3VCFG1_L5                         (0x07FF0000u)
#define VPIF_C3VCFG1_L5_SHIFT                   (0x00000010u)

#define VPIF_C3VCFG1_L7                         (0x000007FFu)
#define VPIF_C3VCFG1_L7_SHIFT                   (0x00000000u)

/* C3VCFG2 */

#define VPIF_C3VCFG2_L9                         (0x07FF0000u)
#define VPIF_C3VCFG2_L9_SHIFT                   (0x00000010u)

#define VPIF_C3VCFG2_L11                        (0x000007FFu)
#define VPIF_C3VCFG2_L11_SHIFT                  (0x00000000u)

/* C3VSIZE */

#define VPIF_C3VSIZE_VSIZE                      (0x000007FFu)
#define VPIF_C3VSIZE_VSIZE_SHIFT                (0x00000000u)

/* C3THANCPOS */

#define VPIF_C3THANCPOS_VPOS                    (0x07FF0000u)
#define VPIF_C3THANCPOS_VPOS_SHIFT              (0x00000010u)

#define VPIF_C3THANCPOS_HPOS                    (0x000007FFu)
#define VPIF_C3THANCPOS_HPOS_SHIFT              (0x00000000u)

/* C3THANCSIZE */

#define VPIF_C3THANCSIZE_VSIZE                  (0x07FF0000u)
#define VPIF_C3THANCSIZE_VSIZE_SHIFT            (0x00000010u)

#define VPIF_C3THANCSIZE_HSIZE                  (0x000007FFu)
#define VPIF_C3THANCSIZE_HSIZE_SHIFT            (0x00000000u)

/* C3BHANCPOS */

#define VPIF_C3BHANCPOS_VPOS                    (0x07FF0000u)
#define VPIF_C3BHANCPOS_VPOS_SHIFT              (0x00000010u)

#define VPIF_C3BHANCPOS_HPOS                    (0x000007FFu)
#define VPIF_C3BHANCPOS_HPOS_SHIFT              (0x00000000u)

/* C3BHANCSIZE */

#define VPIF_C3BHANCSIZE_VSIZE                  (0x07FF0000u)
#define VPIF_C3BHANCSIZE_VSIZE_SHIFT            (0x00000010u)

#define VPIF_C3BHANCSIZE_HSIZE                  (0x000007FFu)
#define VPIF_C3BHANCSIZE_HSIZE_SHIFT            (0x00000000u)

/* C3TVANCPOS */

#define VPIF_C3TVANCPOS_VPOS                    (0x07FF0000u)
#define VPIF_C3TVANCPOS_VPOS_SHIFT              (0x00000010u)

#define VPIF_C3TVANCPOS_HPOS                    (0x000007FFu)
#define VPIF_C3TVANCPOS_HPOS_SHIFT              (0x00000000u)

/* C3TVANCSIZE */

#define VPIF_C3TVANCSIZE_VSIZE                  (0x07FF0000u)
#define VPIF_C3TVANCSIZE_VSIZE_SHIFT            (0x00000010u)

#define VPIF_C3TVANCSIZE_HSIZE                  (0x000007FFu)
#define VPIF_C3TVANCSIZE_HSIZE_SHIFT            (0x00000000u)

/* C3BVANCPOS */

#define VPIF_C3BVANCPOS_VPOS                    (0x07FF0000u)
#define VPIF_C3BVANCPOS_VPOS_SHIFT              (0x00000010u)

#define VPIF_C3BVANCPOS_HPOS                    (0x000007FFu)
#define VPIF_C3BVANCPOS_HPOS_SHIFT              (0x00000000u)

/* C3BVANCSIZE */

#define VPIF_C3BVANCSIZE_VSIZE                  (0x07FF0000u)
#define VPIF_C3BVANCSIZE_VSIZE_SHIFT            (0x00000010u)

#define VPIF_C3BVANCSIZE_HSIZE                  (0x000007FFu)
#define VPIF_C3BVANCSIZE_HSIZE_SHIFT            (0x00000000u)


#ifdef __cplusplus
}
#endif

#endif
