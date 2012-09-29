/**
 * \file  hw_pllc_OMAPL138.h
 *
 * \brief Hardware definitions for PLLC on OMAPL138
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



#ifndef _HW_PLLC_H_
#define _HW_PLLC_H_

#ifdef __cplusplus
extern "C" {
#endif

#define PLLC_REVID	(0x0)
#define PLLC_RSTYPE	(0xE4)
#define PLLC_PLLCTL	(0x100)
#define PLLC_OCSEL	(0x104)
#define PLLC_PLLM	(0x110)
#define PLLC_PREDIV	(0x114)
#define PLLC_PLLDIV1	(0x118)
#define PLLC_PLLDIV2	(0x11C)
#define PLLC_PLLDIV3	(0x120)
#define PLLC_OSCDIV	(0x124)
#define PLLC_POSTDIV	(0x128)
#define PLLC_PLLCMD	(0x138)
#define PLLC_PLLSTAT	(0x13C)
#define PLLC_ALNCTL	(0x140)
#define PLLC_DCHANGE	(0x144)
#define PLLC_CKEN	(0x148)
#define PLLC_CKSTAT	(0x14C)
#define PLLC_SYSTAT	(0x150)
#define PLLC_PLLDIV4	(0x160)
#define PLLC_PLLDIV5	(0x164)
#define PLLC_PLLDIV6	(0x168)
#define PLLC_PLLDIV7	(0x16C)
#define PLLC_EMUCNT0	(0x1F0)
#define PLLC_EMUCNT1	(0x1F4)

/**************************************************************************\
* Field Definition Macros
\**************************************************************************/

/* REVID */

#define PLLC_REVID_REV          (0xFFFFFFFFu)
#define PLLC_REVID_REV_SHIFT         (0x00000000u)


/* RSTYPE */


#define PLLC_RSTYPE_PLLSWRST    (0x00000004u)
#define PLLC_RSTYPE_PLLSWRST_SHIFT   (0x00000002u)

#define PLLC_RSTYPE_XWRST       (0x00000002u)
#define PLLC_RSTYPE_XWRST_SHIFT      (0x00000001u)

#define PLLC_RSTYPE_POR         (0x00000001u)
#define PLLC_RSTYPE_POR_SHIFT        (0x00000000u)


/* PLLCTL */


#define PLLC_PLLCTL_EXTCLKSRC   (0x00000200u)
#define PLLC_PLLCTL_EXTCLKSRC_SHIFT  (0x00000009u)

#define PLLC_PLLCTL_CLKMODE     (0x00000100u)
#define PLLC_PLLCTL_CLKMODE_SHIFT    (0x00000008u)

#define PLLC_PLLCTL_PLLENSRC    (0x00000020u)
#define PLLC_PLLCTL_PLLENSRC_SHIFT   (0x00000005u)

#define PLLC_PLLCTL_PLLDIS      (0x00000010u)
#define PLLC_PLLCTL_PLLDIS_SHIFT     (0x00000004u)

#define PLLC_PLLCTL_PLLRST      (0x00000008u)
#define PLLC_PLLCTL_PLLRST_SHIFT     (0x00000003u)

#define PLLC_PLLCTL_PLLPWRDN    (0x00000002u)
#define PLLC_PLLCTL_PLLPWRDN_SHIFT   (0x00000001u)

#define PLLC_PLLCTL_PLLEN       (0x00000001u)
#define PLLC_PLLCTL_PLLEN_SHIFT      (0x00000000u)


/* OCSEL */


#define PLLC_OCSEL_OCSRC        (0x0000001Fu)
#define PLLC_OCSEL_OCSRC_SHIFT       (0x00000000u)
/*----OCSRC Tokens----*/
#define PLLC_OCSEL_OCSRC_CLKIN       (0x00000014u)
#define PLLC_OCSEL_OCSRC_SYSCLK1     (0x00000017u)
#define PLLC_OCSEL_OCSRC_SYSCLK2     (0x00000018u)
#define PLLC_OCSEL_OCSRC_SYSCLK3     (0x00000019u)
#define PLLC_OCSEL_OCSRC_SYSCLK4     (0x0000001au)
#define PLLC_OCSEL_OCSRC_SYSCLK5     (0x0000001bu)
#define PLLC_OCSEL_OCSRC_SYSCLK6     (0x0000001cu)
#define PLLC_OCSEL_OCSRC_SYSCLK7     (0x0000001du)
#define PLLC_OCSEL_OCSRC_RSVD        (0x0000001eu)


/* PLLM */


#define PLLC_PLLM_PLLM          (0x0000001Fu)
#define PLLC_PLLM_PLLM_SHIFT         (0x00000000u)


/* PREDIV */


#define PLLC_PREDIV_PREDEN      (0x00008000u)
#define PLLC_PREDIV_PREDEN_SHIFT     (0x0000000Fu)

#define PLLC_PREDIV_RATIO       (0x0000001Fu)
#define PLLC_PREDIV_RATIO_SHIFT      (0x00000000u)


/* PLLDIV1 */


#define PLLC_PLLDIV1_D1EN       (0x00008000u)
#define PLLC_PLLDIV1_D1EN_SHIFT      (0x0000000Fu)

#define PLLC_PLLDIV1_RATIO      (0x0000001Fu)
#define PLLC_PLLDIV1_RATIO_SHIFT     (0x00000000u)


/* PLLDIV2 */


#define PLLC_PLLDIV2_D2EN       (0x00008000u)
#define PLLC_PLLDIV2_D2EN_SHIFT      (0x0000000Fu)

#define PLLC_PLLDIV2_RATIO      (0x0000001Fu)
#define PLLC_PLLDIV2_RATIO_SHIFT     (0x00000000u)


/* PLLDIV3 */


#define PLLC_PLLDIV3_D3EN       (0x00008000u)
#define PLLC_PLLDIV3_D3EN_SHIFT      (0x0000000Fu)

#define PLLC_PLLDIV3_RATIO      (0x0000001Fu)
#define PLLC_PLLDIV3_RATIO_SHIFT     (0x00000000u)


/* OSCDIV */


#define PLLC_OSCDIV_OD1EN       (0x00008000u)
#define PLLC_OSCDIV_OD1EN_SHIFT      (0x0000000Fu)

#define PLLC_OSCDIV_RATIO       (0x0000001Fu)
#define PLLC_OSCDIV_RATIO_SHIFT      (0x00000000u)


/* POSTDIV */


#define PLLC_POSTDIV_POSTDEN    (0x00008000u)
#define PLLC_POSTDIV_POSTDEN_SHIFT   (0x0000000Fu)

#define PLLC_POSTDIV_RATIO      (0x0000001Fu)
#define PLLC_POSTDIV_RATIO_SHIFT     (0x00000000u)


/* PLLCMD */


#define PLLC_PLLCMD_GOSET       (0x00000001u)
#define PLLC_PLLCMD_GOSET_SHIFT      (0x00000000u)

/* PLLSTAT */


#define PLLC_PLLSTAT_STABLE     (0x00000004u)
#define PLLC_PLLSTAT_STABLE_SHIFT    (0x00000002u)

#define PLLC_PLLSTAT_GOSTAT     (0x00000001u)
#define PLLC_PLLSTAT_GOSTAT_SHIFT    (0x00000000u)


/* ALNCTL */


#define PLLC_ALNCTL_ALN7        (0x00000040u)
#define PLLC_ALNCTL_ALN7_SHIFT       (0x00000006u)

#define PLLC_ALNCTL_ALN6        (0x00000020u)
#define PLLC_ALNCTL_ALN6_SHIFT       (0x00000005u)

#define PLLC_ALNCTL_ALN5        (0x00000010u)
#define PLLC_ALNCTL_ALN5_SHIFT       (0x00000004u)

#define PLLC_ALNCTL_ALN4        (0x00000008u)
#define PLLC_ALNCTL_ALN4_SHIFT       (0x00000003u)

#define PLLC_ALNCTL_ALN3        (0x00000004u)
#define PLLC_ALNCTL_ALN3_SHIFT       (0x00000002u)

#define PLLC_ALNCTL_ALN2        (0x00000002u)
#define PLLC_ALNCTL_ALN2_SHIFT       (0x00000001u)

#define PLLC_ALNCTL_ALN1        (0x00000001u)
#define PLLC_ALNCTL_ALN1_SHIFT       (0x00000000u)

/* DCHANGE */


#define PLLC_DCHANGE_SYS7       (0x00000040u)
#define PLLC_DCHANGE_SYS7_SHIFT      (0x00000006u)

#define PLLC_DCHANGE_SYS6       (0x00000020u)
#define PLLC_DCHANGE_SYS6_SHIFT      (0x00000005u)

#define PLLC_DCHANGE_SYS5       (0x00000010u)
#define PLLC_DCHANGE_SYS5_SHIFT      (0x00000004u)

#define PLLC_DCHANGE_SYS4       (0x00000008u)
#define PLLC_DCHANGE_SYS4_SHIFT      (0x00000003u)

#define PLLC_DCHANGE_SYS3       (0x00000004u)
#define PLLC_DCHANGE_SYS3_SHIFT      (0x00000002u)

#define PLLC_DCHANGE_SYS2       (0x00000002u)
#define PLLC_DCHANGE_SYS2_SHIFT      (0x00000001u)

#define PLLC_DCHANGE_SYS1       (0x00000001u)
#define PLLC_DCHANGE_SYS1_SHIFT      (0x00000000u)

/* CKEN */


#define PLLC_CKEN_OBSEN         (0x00000002u)
#define PLLC_CKEN_OBSEN_SHIFT        (0x00000001u)

#define PLLC_CKEN_AUXEN         (0x00000001u)
#define PLLC_CKEN_AUXEN_SHIFT        (0x00000000u)


/* CKSTAT */


#define PLLC_CKSTAT_OBSON       (0x00000002u)
#define PLLC_CKSTAT_OBSON_SHIFT      (0x00000001u)

#define PLLC_CKSTAT_AUXEN       (0x00000001u)
#define PLLC_CKSTAT_AUXEN_SHIFT      (0x00000000u)

/* SYSTAT */


#define PLLC_SYSTAT_SYS7ON      (0x00000040u)
#define PLLC_SYSTAT_SYS7ON_SHIFT     (0x00000006u)

#define PLLC_SYSTAT_SYS6ON      (0x00000020u)
#define PLLC_SYSTAT_SYS6ON_SHIFT     (0x00000005u)

#define PLLC_SYSTAT_SYS5ON      (0x00000010u)
#define PLLC_SYSTAT_SYS5ON_SHIFT     (0x00000004u)

#define PLLC_SYSTAT_SYS4ON      (0x00000008u)
#define PLLC_SYSTAT_SYS4ON_SHIFT     (0x00000003u)

#define PLLC_SYSTAT_SYS3ON      (0x00000004u)
#define PLLC_SYSTAT_SYS3ON_SHIFT     (0x00000002u)

#define PLLC_SYSTAT_SYS2ON      (0x00000002u)
#define PLLC_SYSTAT_SYS2ON_SHIFT     (0x00000001u)

#define PLLC_SYSTAT_SYS1ON      (0x00000001u)
#define PLLC_SYSTAT_SYS1ON_SHIFT     (0x00000000u)


/* PLLDIV4 */


#define PLLC_PLLDIV4_D4EN       (0x00008000u)
#define PLLC_PLLDIV4_D4EN_SHIFT      (0x0000000Fu)

#define PLLC_PLLDIV4_RATIO      (0x0000001Fu)
#define PLLC_PLLDIV4_RATIO_SHIFT     (0x00000000u)


/* PLLDIV5 */


#define PLLC_PLLDIV5_D5EN       (0x00008000u)
#define PLLC_PLLDIV5_D5EN_SHIFT      (0x0000000Fu)

#define PLLC_PLLDIV5_RATIO      (0x0000001Fu)
#define PLLC_PLLDIV5_RATIO_SHIFT     (0x00000000u)


/* PLLDIV6 */


#define PLLC_PLLDIV6_D6EN       (0x00008000u)
#define PLLC_PLLDIV6_D6EN_SHIFT      (0x0000000Fu)

#define PLLC_PLLDIV6_RATIO      (0x0000001Fu)
#define PLLC_PLLDIV6_RATIO_SHIFT     (0x00000000u)


/* PLLDIV7 */


#define PLLC_PLLDIV7_D7EN       (0x00008000u)
#define PLLC_PLLDIV7_D7EN_SHIFT      (0x0000000Fu)

#define PLLC_PLLDIV7_RATIO      (0x0000001Fu)
#define PLLC_PLLDIV7_RATIO_SHIFT     (0x00000000u)


/* EMUCNT0 */

#define PLLC_EMUCNT0_COUNT      (0xFFFFFFFFu)
#define PLLC_EMUCNT0_COUNT_SHIFT     (0x00000000u)


/* EMUCNT1 */

#define PLLC_EMUCNT1_COUNT      (0xFFFFFFFFu)
#define PLLC_EMUCNT1_COUNT_SHIFT     (0x00000000u)


#ifdef __cplusplus
}
#endif

#endif
