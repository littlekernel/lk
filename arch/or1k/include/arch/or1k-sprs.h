/* or1k-sprs.h -- OR1K SPR definitions
   Copyright (c) 2014 OpenRISC Project Maintainers
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following condition
   is met:

   1. Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
   FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
   COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
   INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
   STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
   OF THE POSSIBILITY OF SUCH DAMAGE.
   */

/*
 * Generated from revision 4cab27375dd07cd890d5493e11446cc06ca8265e
 *  on Fri Feb 13 03:15:06 2015
 */

#ifndef _OR1K_SPRS_H_
#define _OR1K_SPRS_H_

#define OR1K_SPR_GROUP_BITS   5
#define OR1K_SPR_GROUP_LSB   11
#define OR1K_SPR_GROUP_MSB   15
#define OR1K_SPR_INDEX_BITS  11
#define OR1K_SPR_INDEX_LSB    0
#define OR1K_SPR_INDEX_MSB   10

#ifdef __ASSEMBLER__
#define OR1K_UNSIGNED(x) x
#else
#define OR1K_UNSIGNED(x) x##U
#endif


/****************/
/* System Group */
/****************/
#define OR1K_SPR_SYS_GROUP 0x00

/* Version Register */
#define OR1K_SPR_SYS_VR_INDEX OR1K_UNSIGNED(0x000)
#define OR1K_SPR_SYS_VR_ADDR  OR1K_UNSIGNED(0x0000)

/* Revision */
#define OR1K_SPR_SYS_VR_REV_LSB    0
#define OR1K_SPR_SYS_VR_REV_MSB    5
#define OR1K_SPR_SYS_VR_REV_BITS   6
#define OR1K_SPR_SYS_VR_REV_MASK   OR1K_UNSIGNED(0x0000003f)
#define OR1K_SPR_SYS_VR_REV_GET(X) (((X) >> 0) & OR1K_UNSIGNED(0x0000003f))
#define OR1K_SPR_SYS_VR_REV_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffffc0)) | ((Y) << 0))

/* Updated Version Registers Present */
#define OR1K_SPR_SYS_VR_UVRP_OFFSET 6
#define OR1K_SPR_SYS_VR_UVRP_MASK   0x00000040
#define OR1K_SPR_SYS_VR_UVRP_GET(X) (((X) >> 6) & 0x1)
#define OR1K_SPR_SYS_VR_UVRP_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffffbf)) | ((!!(Y)) << 6))

/* Configuration Template */
#define OR1K_SPR_SYS_VR_CFG_LSB    16
#define OR1K_SPR_SYS_VR_CFG_MSB    23
#define OR1K_SPR_SYS_VR_CFG_BITS   8
#define OR1K_SPR_SYS_VR_CFG_MASK   OR1K_UNSIGNED(0x00ff0000)
#define OR1K_SPR_SYS_VR_CFG_GET(X) (((X) >> 16) & OR1K_UNSIGNED(0x000000ff))
#define OR1K_SPR_SYS_VR_CFG_SET(X, Y) (((X) & OR1K_UNSIGNED(0xff00ffff)) | ((Y) << 16))

/* Version */
#define OR1K_SPR_SYS_VR_VER_LSB    24
#define OR1K_SPR_SYS_VR_VER_MSB    31
#define OR1K_SPR_SYS_VR_VER_BITS   8
#define OR1K_SPR_SYS_VR_VER_MASK   OR1K_UNSIGNED(0xff000000)
#define OR1K_SPR_SYS_VR_VER_GET(X) (((X) >> 24) & OR1K_UNSIGNED(0x000000ff))
#define OR1K_SPR_SYS_VR_VER_SET(X, Y) (((X) & OR1K_UNSIGNED(0x00ffffff)) | ((Y) << 24))


/* Unit Present Register */
#define OR1K_SPR_SYS_UPR_INDEX OR1K_UNSIGNED(0x001)
#define OR1K_SPR_SYS_UPR_ADDR  OR1K_UNSIGNED(0x0001)

/* UPR Present */
#define OR1K_SPR_SYS_UPR_UP_OFFSET 0
#define OR1K_SPR_SYS_UPR_UP_MASK   0x00000001
#define OR1K_SPR_SYS_UPR_UP_GET(X) (((X) >> 0) & 0x1)
#define OR1K_SPR_SYS_UPR_UP_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffffe)) | ((!!(Y)) << 0))

/* Data Cache Present */
#define OR1K_SPR_SYS_UPR_DCP_OFFSET 1
#define OR1K_SPR_SYS_UPR_DCP_MASK   0x00000002
#define OR1K_SPR_SYS_UPR_DCP_GET(X) (((X) >> 1) & 0x1)
#define OR1K_SPR_SYS_UPR_DCP_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffffd)) | ((!!(Y)) << 1))

/* Instruction Cache Present */
#define OR1K_SPR_SYS_UPR_ICP_OFFSET 2
#define OR1K_SPR_SYS_UPR_ICP_MASK   0x00000004
#define OR1K_SPR_SYS_UPR_ICP_GET(X) (((X) >> 2) & 0x1)
#define OR1K_SPR_SYS_UPR_ICP_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffffb)) | ((!!(Y)) << 2))

/* Data MMU Present */
#define OR1K_SPR_SYS_UPR_DMP_OFFSET 3
#define OR1K_SPR_SYS_UPR_DMP_MASK   0x00000008
#define OR1K_SPR_SYS_UPR_DMP_GET(X) (((X) >> 3) & 0x1)
#define OR1K_SPR_SYS_UPR_DMP_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffff7)) | ((!!(Y)) << 3))

/* Instruction MMU Present */
#define OR1K_SPR_SYS_UPR_IMP_OFFSET 4
#define OR1K_SPR_SYS_UPR_IMP_MASK   0x00000010
#define OR1K_SPR_SYS_UPR_IMP_GET(X) (((X) >> 4) & 0x1)
#define OR1K_SPR_SYS_UPR_IMP_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffffef)) | ((!!(Y)) << 4))

/* MAC Present */
#define OR1K_SPR_SYS_UPR_MP_OFFSET 5
#define OR1K_SPR_SYS_UPR_MP_MASK   0x00000020
#define OR1K_SPR_SYS_UPR_MP_GET(X) (((X) >> 5) & 0x1)
#define OR1K_SPR_SYS_UPR_MP_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffffdf)) | ((!!(Y)) << 5))

/* Debug Unit Present */
#define OR1K_SPR_SYS_UPR_DUP_OFFSET 6
#define OR1K_SPR_SYS_UPR_DUP_MASK   0x00000040
#define OR1K_SPR_SYS_UPR_DUP_GET(X) (((X) >> 6) & 0x1)
#define OR1K_SPR_SYS_UPR_DUP_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffffbf)) | ((!!(Y)) << 6))

/* Performance Counters Unit Present */
#define OR1K_SPR_SYS_UPR_PCUP_OFFSET 7
#define OR1K_SPR_SYS_UPR_PCUP_MASK   0x00000080
#define OR1K_SPR_SYS_UPR_PCUP_GET(X) (((X) >> 7) & 0x1)
#define OR1K_SPR_SYS_UPR_PCUP_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffff7f)) | ((!!(Y)) << 7))

/* Power Management Present */
#define OR1K_SPR_SYS_UPR_PICP_OFFSET 8
#define OR1K_SPR_SYS_UPR_PICP_MASK   0x00000100
#define OR1K_SPR_SYS_UPR_PICP_GET(X) (((X) >> 8) & 0x1)
#define OR1K_SPR_SYS_UPR_PICP_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffeff)) | ((!!(Y)) << 8))

/* Programmable Interrupt Controller Present */
#define OR1K_SPR_SYS_UPR_PMP_OFFSET 9
#define OR1K_SPR_SYS_UPR_PMP_MASK   0x00000200
#define OR1K_SPR_SYS_UPR_PMP_GET(X) (((X) >> 9) & 0x1)
#define OR1K_SPR_SYS_UPR_PMP_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffdff)) | ((!!(Y)) << 9))

/* Tick Timer Present */
#define OR1K_SPR_SYS_UPR_TTP_OFFSET 10
#define OR1K_SPR_SYS_UPR_TTP_MASK   0x00000400
#define OR1K_SPR_SYS_UPR_TTP_GET(X) (((X) >> 10) & 0x1)
#define OR1K_SPR_SYS_UPR_TTP_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffbff)) | ((!!(Y)) << 10))

/* Custom Units Present */
#define OR1K_SPR_SYS_UPR_CUP_LSB    24
#define OR1K_SPR_SYS_UPR_CUP_MSB    31
#define OR1K_SPR_SYS_UPR_CUP_BITS   8
#define OR1K_SPR_SYS_UPR_CUP_MASK   OR1K_UNSIGNED(0xff000000)
#define OR1K_SPR_SYS_UPR_CUP_GET(X) (((X) >> 24) & OR1K_UNSIGNED(0x000000ff))
#define OR1K_SPR_SYS_UPR_CUP_SET(X, Y) (((X) & OR1K_UNSIGNED(0x00ffffff)) | ((Y) << 24))


/* CPU Configuration Register */
#define OR1K_SPR_SYS_CPUCFGR_INDEX OR1K_UNSIGNED(0x002)
#define OR1K_SPR_SYS_CPUCFGR_ADDR  OR1K_UNSIGNED(0x0002)

/* Number of Shadow GPR Files */
#define OR1K_SPR_SYS_CPUCFGR_NSGF_LSB    0
#define OR1K_SPR_SYS_CPUCFGR_NSGF_MSB    3
#define OR1K_SPR_SYS_CPUCFGR_NSGF_BITS   4
#define OR1K_SPR_SYS_CPUCFGR_NSGF_MASK   OR1K_UNSIGNED(0x0000000f)
#define OR1K_SPR_SYS_CPUCFGR_NSGF_GET(X) (((X) >> 0) & OR1K_UNSIGNED(0x0000000f))
#define OR1K_SPR_SYS_CPUCFGR_NSGF_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffff0)) | ((Y) << 0))

/* Custom GPR File */
#define OR1K_SPR_SYS_CPUCFGR_CGF_OFFSET 4
#define OR1K_SPR_SYS_CPUCFGR_CGF_MASK   0x00000010
#define OR1K_SPR_SYS_CPUCFGR_CGF_GET(X) (((X) >> 4) & 0x1)
#define OR1K_SPR_SYS_CPUCFGR_CGF_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffffef)) | ((!!(Y)) << 4))

/* ORBIS32 Supported */
#define OR1K_SPR_SYS_CPUCFGR_OB32S_OFFSET 5
#define OR1K_SPR_SYS_CPUCFGR_OB32S_MASK   0x00000020
#define OR1K_SPR_SYS_CPUCFGR_OB32S_GET(X) (((X) >> 5) & 0x1)
#define OR1K_SPR_SYS_CPUCFGR_OB32S_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffffdf)) | ((!!(Y)) << 5))

/* ORBIS64 Supported */
#define OR1K_SPR_SYS_CPUCFGR_OB64S_OFFSET 6
#define OR1K_SPR_SYS_CPUCFGR_OB64S_MASK   0x00000040
#define OR1K_SPR_SYS_CPUCFGR_OB64S_GET(X) (((X) >> 6) & 0x1)
#define OR1K_SPR_SYS_CPUCFGR_OB64S_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffffbf)) | ((!!(Y)) << 6))

/* ORFPX32 Supported */
#define OR1K_SPR_SYS_CPUCFGR_OF32S_OFFSET 7
#define OR1K_SPR_SYS_CPUCFGR_OF32S_MASK   0x00000080
#define OR1K_SPR_SYS_CPUCFGR_OF32S_GET(X) (((X) >> 7) & 0x1)
#define OR1K_SPR_SYS_CPUCFGR_OF32S_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffff7f)) | ((!!(Y)) << 7))

/* ORFPX64 Supported */
#define OR1K_SPR_SYS_CPUCFGR_OF64S_OFFSET 8
#define OR1K_SPR_SYS_CPUCFGR_OF64S_MASK   0x00000100
#define OR1K_SPR_SYS_CPUCFGR_OF64S_GET(X) (((X) >> 8) & 0x1)
#define OR1K_SPR_SYS_CPUCFGR_OF64S_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffeff)) | ((!!(Y)) << 8))

/* ORVDX64 Supported */
#define OR1K_SPR_SYS_CPUCFGR_OV64S_OFFSET 9
#define OR1K_SPR_SYS_CPUCFGR_OV64S_MASK   0x00000200
#define OR1K_SPR_SYS_CPUCFGR_OV64S_GET(X) (((X) >> 9) & 0x1)
#define OR1K_SPR_SYS_CPUCFGR_OV64S_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffdff)) | ((!!(Y)) << 9))

/* No Delay-Slot */
#define OR1K_SPR_SYS_CPUCFGR_ND_OFFSET 10
#define OR1K_SPR_SYS_CPUCFGR_ND_MASK   0x00000400
#define OR1K_SPR_SYS_CPUCFGR_ND_GET(X) (((X) >> 10) & 0x1)
#define OR1K_SPR_SYS_CPUCFGR_ND_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffbff)) | ((!!(Y)) << 10))

/* Architecture Version Register Present */
#define OR1K_SPR_SYS_CPUCFGR_AVRP_OFFSET 11
#define OR1K_SPR_SYS_CPUCFGR_AVRP_MASK   0x00000800
#define OR1K_SPR_SYS_CPUCFGR_AVRP_GET(X) (((X) >> 11) & 0x1)
#define OR1K_SPR_SYS_CPUCFGR_AVRP_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffff7ff)) | ((!!(Y)) << 11))

/* Exception Vector Base Address Register Present */
#define OR1K_SPR_SYS_CPUCFGR_EVBARP_OFFSET 12
#define OR1K_SPR_SYS_CPUCFGR_EVBARP_MASK   0x00001000
#define OR1K_SPR_SYS_CPUCFGR_EVBARP_GET(X) (((X) >> 12) & 0x1)
#define OR1K_SPR_SYS_CPUCFGR_EVBARP_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffefff)) | ((!!(Y)) << 12))

/* Implementation-Specific Registers (ISR0-7) Present */
#define OR1K_SPR_SYS_CPUCFGR_ISRP_OFFSET 13
#define OR1K_SPR_SYS_CPUCFGR_ISRP_MASK   0x00002000
#define OR1K_SPR_SYS_CPUCFGR_ISRP_GET(X) (((X) >> 13) & 0x1)
#define OR1K_SPR_SYS_CPUCFGR_ISRP_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffdfff)) | ((!!(Y)) << 13))

/* Arithmetic Exception Control/Status Registers Present */
#define OR1K_SPR_SYS_CPUCFGR_AECSRP_OFFSET 14
#define OR1K_SPR_SYS_CPUCFGR_AECSRP_MASK   0x00004000
#define OR1K_SPR_SYS_CPUCFGR_AECSRP_GET(X) (((X) >> 14) & 0x1)
#define OR1K_SPR_SYS_CPUCFGR_AECSRP_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffbfff)) | ((!!(Y)) << 14))


/* Data MMU Configuration Register */
#define OR1K_SPR_SYS_DMMUCFGR_INDEX OR1K_UNSIGNED(0x003)
#define OR1K_SPR_SYS_DMMUCFGR_ADDR  OR1K_UNSIGNED(0x0003)

/* Number of TLB Ways */
#define OR1K_SPR_SYS_DMMUCFGR_NTW_LSB    0
#define OR1K_SPR_SYS_DMMUCFGR_NTW_MSB    1
#define OR1K_SPR_SYS_DMMUCFGR_NTW_BITS   2
#define OR1K_SPR_SYS_DMMUCFGR_NTW_MASK   OR1K_UNSIGNED(0x00000003)
#define OR1K_SPR_SYS_DMMUCFGR_NTW_GET(X) (((X) >> 0) & OR1K_UNSIGNED(0x00000003))
#define OR1K_SPR_SYS_DMMUCFGR_NTW_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffffc)) | ((Y) << 0))

/* Number of TLB Sets */
#define OR1K_SPR_SYS_DMMUCFGR_NTS_LSB    2
#define OR1K_SPR_SYS_DMMUCFGR_NTS_MSB    4
#define OR1K_SPR_SYS_DMMUCFGR_NTS_BITS   3
#define OR1K_SPR_SYS_DMMUCFGR_NTS_MASK   OR1K_UNSIGNED(0x0000001c)
#define OR1K_SPR_SYS_DMMUCFGR_NTS_GET(X) (((X) >> 2) & OR1K_UNSIGNED(0x00000007))
#define OR1K_SPR_SYS_DMMUCFGR_NTS_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffffe3)) | ((Y) << 2))

/* Number of ATB Entries */
#define OR1K_SPR_SYS_DMMUCFGR_NAE_LSB    5
#define OR1K_SPR_SYS_DMMUCFGR_NAE_MSB    7
#define OR1K_SPR_SYS_DMMUCFGR_NAE_BITS   3
#define OR1K_SPR_SYS_DMMUCFGR_NAE_MASK   OR1K_UNSIGNED(0x000000e0)
#define OR1K_SPR_SYS_DMMUCFGR_NAE_GET(X) (((X) >> 5) & OR1K_UNSIGNED(0x00000007))
#define OR1K_SPR_SYS_DMMUCFGR_NAE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffff1f)) | ((Y) << 5))

/* Control Register Implemented */
#define OR1K_SPR_SYS_DMMUCFGR_CRI_OFFSET 8
#define OR1K_SPR_SYS_DMMUCFGR_CRI_MASK   0x00000100
#define OR1K_SPR_SYS_DMMUCFGR_CRI_GET(X) (((X) >> 8) & 0x1)
#define OR1K_SPR_SYS_DMMUCFGR_CRI_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffeff)) | ((!!(Y)) << 8))

/* Protection Register Implemented */
#define OR1K_SPR_SYS_DMMUCFGR_PRI_OFFSET 9
#define OR1K_SPR_SYS_DMMUCFGR_PRI_MASK   0x00000200
#define OR1K_SPR_SYS_DMMUCFGR_PRI_GET(X) (((X) >> 9) & 0x1)
#define OR1K_SPR_SYS_DMMUCFGR_PRI_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffdff)) | ((!!(Y)) << 9))

/* TLB Entry Invalidate Register Implemented */
#define OR1K_SPR_SYS_DMMUCFGR_TEIRI_OFFSET 10
#define OR1K_SPR_SYS_DMMUCFGR_TEIRI_MASK   0x00000400
#define OR1K_SPR_SYS_DMMUCFGR_TEIRI_GET(X) (((X) >> 10) & 0x1)
#define OR1K_SPR_SYS_DMMUCFGR_TEIRI_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffbff)) | ((!!(Y)) << 10))

/* Hardware TLB Reload */
#define OR1K_SPR_SYS_DMMUCFGR_HTR_OFFSET 11
#define OR1K_SPR_SYS_DMMUCFGR_HTR_MASK   0x00000800
#define OR1K_SPR_SYS_DMMUCFGR_HTR_GET(X) (((X) >> 11) & 0x1)
#define OR1K_SPR_SYS_DMMUCFGR_HTR_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffff7ff)) | ((!!(Y)) << 11))

/* DTLB reloaded in software */
#define OR1K_SPR_SYS_DMMUCFGR_HTR_SW 0
/* DTLB reloaded in hardware */
#define OR1K_SPR_SYS_DMMUCFGR_HTR_HW 1

/* Instruction MMU Configuration Register */
#define OR1K_SPR_SYS_IMMUCFGR_INDEX OR1K_UNSIGNED(0x004)
#define OR1K_SPR_SYS_IMMUCFGR_ADDR  OR1K_UNSIGNED(0x0004)

/* Number of TLB Ways */
#define OR1K_SPR_SYS_IMMUCFGR_NTW_LSB    0
#define OR1K_SPR_SYS_IMMUCFGR_NTW_MSB    1
#define OR1K_SPR_SYS_IMMUCFGR_NTW_BITS   2
#define OR1K_SPR_SYS_IMMUCFGR_NTW_MASK   OR1K_UNSIGNED(0x00000003)
#define OR1K_SPR_SYS_IMMUCFGR_NTW_GET(X) (((X) >> 0) & OR1K_UNSIGNED(0x00000003))
#define OR1K_SPR_SYS_IMMUCFGR_NTW_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffffc)) | ((Y) << 0))

/* Number of TLB Sets */
#define OR1K_SPR_SYS_IMMUCFGR_NTS_LSB    2
#define OR1K_SPR_SYS_IMMUCFGR_NTS_MSB    4
#define OR1K_SPR_SYS_IMMUCFGR_NTS_BITS   3
#define OR1K_SPR_SYS_IMMUCFGR_NTS_MASK   OR1K_UNSIGNED(0x0000001c)
#define OR1K_SPR_SYS_IMMUCFGR_NTS_GET(X) (((X) >> 2) & OR1K_UNSIGNED(0x00000007))
#define OR1K_SPR_SYS_IMMUCFGR_NTS_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffffe3)) | ((Y) << 2))

/* Number of ATB Entries */
#define OR1K_SPR_SYS_IMMUCFGR_NAE_LSB    5
#define OR1K_SPR_SYS_IMMUCFGR_NAE_MSB    7
#define OR1K_SPR_SYS_IMMUCFGR_NAE_BITS   3
#define OR1K_SPR_SYS_IMMUCFGR_NAE_MASK   OR1K_UNSIGNED(0x000000e0)
#define OR1K_SPR_SYS_IMMUCFGR_NAE_GET(X) (((X) >> 5) & OR1K_UNSIGNED(0x00000007))
#define OR1K_SPR_SYS_IMMUCFGR_NAE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffff1f)) | ((Y) << 5))

/* Control Register Implemented */
#define OR1K_SPR_SYS_IMMUCFGR_CRI_OFFSET 8
#define OR1K_SPR_SYS_IMMUCFGR_CRI_MASK   0x00000100
#define OR1K_SPR_SYS_IMMUCFGR_CRI_GET(X) (((X) >> 8) & 0x1)
#define OR1K_SPR_SYS_IMMUCFGR_CRI_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffeff)) | ((!!(Y)) << 8))

/* Protection Register Implemented */
#define OR1K_SPR_SYS_IMMUCFGR_PRI_OFFSET 9
#define OR1K_SPR_SYS_IMMUCFGR_PRI_MASK   0x00000200
#define OR1K_SPR_SYS_IMMUCFGR_PRI_GET(X) (((X) >> 9) & 0x1)
#define OR1K_SPR_SYS_IMMUCFGR_PRI_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffdff)) | ((!!(Y)) << 9))

/* TLB Entry Invalidate Register Implemented */
#define OR1K_SPR_SYS_IMMUCFGR_TEIRI_OFFSET 10
#define OR1K_SPR_SYS_IMMUCFGR_TEIRI_MASK   0x00000400
#define OR1K_SPR_SYS_IMMUCFGR_TEIRI_GET(X) (((X) >> 10) & 0x1)
#define OR1K_SPR_SYS_IMMUCFGR_TEIRI_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffbff)) | ((!!(Y)) << 10))

/* Hardware TLB Reload */
#define OR1K_SPR_SYS_IMMUCFGR_HTR_OFFSET 11
#define OR1K_SPR_SYS_IMMUCFGR_HTR_MASK   0x00000800
#define OR1K_SPR_SYS_IMMUCFGR_HTR_GET(X) (((X) >> 11) & 0x1)
#define OR1K_SPR_SYS_IMMUCFGR_HTR_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffff7ff)) | ((!!(Y)) << 11))

/* DTLB reloaded in software */
#define OR1K_SPR_SYS_IMMUCFGR_HTR_SW 0
/* DTLB reloaded in hardware */
#define OR1K_SPR_SYS_IMMUCFGR_HTR_HW 1

/* Data Cache Configuration Register */
#define OR1K_SPR_SYS_DCCFGR_INDEX OR1K_UNSIGNED(0x005)
#define OR1K_SPR_SYS_DCCFGR_ADDR  OR1K_UNSIGNED(0x0005)

/* Number of Cache Ways */
#define OR1K_SPR_SYS_DCCFGR_NCW_LSB    0
#define OR1K_SPR_SYS_DCCFGR_NCW_MSB    2
#define OR1K_SPR_SYS_DCCFGR_NCW_BITS   3
#define OR1K_SPR_SYS_DCCFGR_NCW_MASK   OR1K_UNSIGNED(0x00000007)
#define OR1K_SPR_SYS_DCCFGR_NCW_GET(X) (((X) >> 0) & OR1K_UNSIGNED(0x00000007))
#define OR1K_SPR_SYS_DCCFGR_NCW_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffff8)) | ((Y) << 0))

/* Number of Cache Sets */
#define OR1K_SPR_SYS_DCCFGR_NCS_LSB    3
#define OR1K_SPR_SYS_DCCFGR_NCS_MSB    6
#define OR1K_SPR_SYS_DCCFGR_NCS_BITS   4
#define OR1K_SPR_SYS_DCCFGR_NCS_MASK   OR1K_UNSIGNED(0x00000078)
#define OR1K_SPR_SYS_DCCFGR_NCS_GET(X) (((X) >> 3) & OR1K_UNSIGNED(0x0000000f))
#define OR1K_SPR_SYS_DCCFGR_NCS_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffff87)) | ((Y) << 3))

/* Cache Block Size */
#define OR1K_SPR_SYS_DCCFGR_CBS_OFFSET 7
#define OR1K_SPR_SYS_DCCFGR_CBS_MASK   0x00000080
#define OR1K_SPR_SYS_DCCFGR_CBS_GET(X) (((X) >> 7) & 0x1)
#define OR1K_SPR_SYS_DCCFGR_CBS_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffff7f)) | ((!!(Y)) << 7))

/* 16 Bytes */
#define OR1K_SPR_SYS_DCCFGR_CBS_16 0
/* 32 Bytes */
#define OR1K_SPR_SYS_DCCFGR_CBS_32 1
/* Cache Write Strategy */
#define OR1K_SPR_SYS_DCCFGR_CWS_OFFSET 8
#define OR1K_SPR_SYS_DCCFGR_CWS_MASK   0x00000100
#define OR1K_SPR_SYS_DCCFGR_CWS_GET(X) (((X) >> 8) & 0x1)
#define OR1K_SPR_SYS_DCCFGR_CWS_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffeff)) | ((!!(Y)) << 8))

/* Write Through */
#define OR1K_SPR_SYS_DCCFGR_CWS_WT 0
/* Write Back */
#define OR1K_SPR_SYS_DCCFGR_CWS_WB 1
/* Cache Control Register Implemented */
#define OR1K_SPR_SYS_DCCFGR_CCRI_OFFSET 9
#define OR1K_SPR_SYS_DCCFGR_CCRI_MASK   0x00000200
#define OR1K_SPR_SYS_DCCFGR_CCRI_GET(X) (((X) >> 9) & 0x1)
#define OR1K_SPR_SYS_DCCFGR_CCRI_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffdff)) | ((!!(Y)) << 9))

/* Cache Block Invalidate Register Implemented */
#define OR1K_SPR_SYS_DCCFGR_CBIRI_OFFSET 10
#define OR1K_SPR_SYS_DCCFGR_CBIRI_MASK   0x00000400
#define OR1K_SPR_SYS_DCCFGR_CBIRI_GET(X) (((X) >> 10) & 0x1)
#define OR1K_SPR_SYS_DCCFGR_CBIRI_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffbff)) | ((!!(Y)) << 10))

/* Cache Block Prefetch Register Implemented */
#define OR1K_SPR_SYS_DCCFGR_CBPRI_OFFSET 11
#define OR1K_SPR_SYS_DCCFGR_CBPRI_MASK   0x00000800
#define OR1K_SPR_SYS_DCCFGR_CBPRI_GET(X) (((X) >> 11) & 0x1)
#define OR1K_SPR_SYS_DCCFGR_CBPRI_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffff7ff)) | ((!!(Y)) << 11))

/* Cache Block Lock Register Implemented */
#define OR1K_SPR_SYS_DCCFGR_CBLRI_OFFSET 12
#define OR1K_SPR_SYS_DCCFGR_CBLRI_MASK   0x00001000
#define OR1K_SPR_SYS_DCCFGR_CBLRI_GET(X) (((X) >> 12) & 0x1)
#define OR1K_SPR_SYS_DCCFGR_CBLRI_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffefff)) | ((!!(Y)) << 12))

/* Cache Block Flush Register Implemented */
#define OR1K_SPR_SYS_DCCFGR_CBFRI_OFFSET 13
#define OR1K_SPR_SYS_DCCFGR_CBFRI_MASK   0x00002000
#define OR1K_SPR_SYS_DCCFGR_CBFRI_GET(X) (((X) >> 13) & 0x1)
#define OR1K_SPR_SYS_DCCFGR_CBFRI_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffdfff)) | ((!!(Y)) << 13))

/* Cache Block Write-back Register Implemented */
#define OR1K_SPR_SYS_DCCFGR_CBWBRI_OFFSET 14
#define OR1K_SPR_SYS_DCCFGR_CBWBRI_MASK   0x00004000
#define OR1K_SPR_SYS_DCCFGR_CBWBRI_GET(X) (((X) >> 14) & 0x1)
#define OR1K_SPR_SYS_DCCFGR_CBWBRI_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffbfff)) | ((!!(Y)) << 14))


/* Instruction Cache Configuration Register */
#define OR1K_SPR_SYS_ICCFGR_INDEX OR1K_UNSIGNED(0x006)
#define OR1K_SPR_SYS_ICCFGR_ADDR  OR1K_UNSIGNED(0x0006)

/* Number of Cache Ways */
#define OR1K_SPR_SYS_ICCFGR_NCW_LSB    0
#define OR1K_SPR_SYS_ICCFGR_NCW_MSB    2
#define OR1K_SPR_SYS_ICCFGR_NCW_BITS   3
#define OR1K_SPR_SYS_ICCFGR_NCW_MASK   OR1K_UNSIGNED(0x00000007)
#define OR1K_SPR_SYS_ICCFGR_NCW_GET(X) (((X) >> 0) & OR1K_UNSIGNED(0x00000007))
#define OR1K_SPR_SYS_ICCFGR_NCW_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffff8)) | ((Y) << 0))

/* Number of Cache Sets */
#define OR1K_SPR_SYS_ICCFGR_NCS_LSB    3
#define OR1K_SPR_SYS_ICCFGR_NCS_MSB    6
#define OR1K_SPR_SYS_ICCFGR_NCS_BITS   4
#define OR1K_SPR_SYS_ICCFGR_NCS_MASK   OR1K_UNSIGNED(0x00000078)
#define OR1K_SPR_SYS_ICCFGR_NCS_GET(X) (((X) >> 3) & OR1K_UNSIGNED(0x0000000f))
#define OR1K_SPR_SYS_ICCFGR_NCS_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffff87)) | ((Y) << 3))

/* Cache Block Size */
#define OR1K_SPR_SYS_ICCFGR_CBS_OFFSET 7
#define OR1K_SPR_SYS_ICCFGR_CBS_MASK   0x00000080
#define OR1K_SPR_SYS_ICCFGR_CBS_GET(X) (((X) >> 7) & 0x1)
#define OR1K_SPR_SYS_ICCFGR_CBS_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffff7f)) | ((!!(Y)) << 7))

/* 16 Bytes */
#define OR1K_SPR_SYS_ICCFGR_CBS_16 0
/* 32 Bytes */
#define OR1K_SPR_SYS_ICCFGR_CBS_32 1
/* Cache Control Register Implemented */
#define OR1K_SPR_SYS_ICCFGR_CCRI_OFFSET 9
#define OR1K_SPR_SYS_ICCFGR_CCRI_MASK   0x00000200
#define OR1K_SPR_SYS_ICCFGR_CCRI_GET(X) (((X) >> 9) & 0x1)
#define OR1K_SPR_SYS_ICCFGR_CCRI_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffdff)) | ((!!(Y)) << 9))

/* Cache Block Invalidate Register Implemented */
#define OR1K_SPR_SYS_ICCFGR_CBIRI_OFFSET 10
#define OR1K_SPR_SYS_ICCFGR_CBIRI_MASK   0x00000400
#define OR1K_SPR_SYS_ICCFGR_CBIRI_GET(X) (((X) >> 10) & 0x1)
#define OR1K_SPR_SYS_ICCFGR_CBIRI_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffbff)) | ((!!(Y)) << 10))

/* Cache Block Prefetch Register Implemented */
#define OR1K_SPR_SYS_ICCFGR_CBPRI_OFFSET 11
#define OR1K_SPR_SYS_ICCFGR_CBPRI_MASK   0x00000800
#define OR1K_SPR_SYS_ICCFGR_CBPRI_GET(X) (((X) >> 11) & 0x1)
#define OR1K_SPR_SYS_ICCFGR_CBPRI_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffff7ff)) | ((!!(Y)) << 11))

/* Cache Block Lock Register Implemented */
#define OR1K_SPR_SYS_ICCFGR_CBLRI_OFFSET 12
#define OR1K_SPR_SYS_ICCFGR_CBLRI_MASK   0x00001000
#define OR1K_SPR_SYS_ICCFGR_CBLRI_GET(X) (((X) >> 12) & 0x1)
#define OR1K_SPR_SYS_ICCFGR_CBLRI_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffefff)) | ((!!(Y)) << 12))


/* Debug Configuration Register */
#define OR1K_SPR_SYS_DCFGR_INDEX OR1K_UNSIGNED(0x007)
#define OR1K_SPR_SYS_DCFGR_ADDR  OR1K_UNSIGNED(0x0007)

/* Number of Debug Pairs */
#define OR1K_SPR_SYS_DCFGR_NDP_LSB    0
#define OR1K_SPR_SYS_DCFGR_NDP_MSB    2
#define OR1K_SPR_SYS_DCFGR_NDP_BITS   3
#define OR1K_SPR_SYS_DCFGR_NDP_MASK   OR1K_UNSIGNED(0x00000007)
#define OR1K_SPR_SYS_DCFGR_NDP_GET(X) (((X) >> 0) & OR1K_UNSIGNED(0x00000007))
#define OR1K_SPR_SYS_DCFGR_NDP_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffff8)) | ((Y) << 0))

/* Watchpoint Counters Implemented */
#define OR1K_SPR_SYS_DCFGR_WPCI_OFFSET 3
#define OR1K_SPR_SYS_DCFGR_WPCI_MASK   0x00000008
#define OR1K_SPR_SYS_DCFGR_WPCI_GET(X) (((X) >> 3) & 0x1)
#define OR1K_SPR_SYS_DCFGR_WPCI_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffff7)) | ((!!(Y)) << 3))


/* Performance Counters Configuration */
#define OR1K_SPR_SYS_PCCFGR_INDEX OR1K_UNSIGNED(0x008)
#define OR1K_SPR_SYS_PCCFGR_ADDR  OR1K_UNSIGNED(0x0008)

/* Number of Performance Counters */
#define OR1K_SPR_SYS_PCCFGR_NPC_LSB    0
#define OR1K_SPR_SYS_PCCFGR_NPC_MSB    2
#define OR1K_SPR_SYS_PCCFGR_NPC_BITS   3
#define OR1K_SPR_SYS_PCCFGR_NPC_MASK   OR1K_UNSIGNED(0x00000007)
#define OR1K_SPR_SYS_PCCFGR_NPC_GET(X) (((X) >> 0) & OR1K_UNSIGNED(0x00000007))
#define OR1K_SPR_SYS_PCCFGR_NPC_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffff8)) | ((Y) << 0))


/* Version Register 2 */
#define OR1K_SPR_SYS_VR2_INDEX OR1K_UNSIGNED(0x009)
#define OR1K_SPR_SYS_VR2_ADDR  OR1K_UNSIGNED(0x0009)

/* Version */
#define OR1K_SPR_SYS_VR2_VER_LSB    0
#define OR1K_SPR_SYS_VR2_VER_MSB    23
#define OR1K_SPR_SYS_VR2_VER_BITS   24
#define OR1K_SPR_SYS_VR2_VER_MASK   OR1K_UNSIGNED(0x00ffffff)
#define OR1K_SPR_SYS_VR2_VER_GET(X) (((X) >> 0) & OR1K_UNSIGNED(0x00ffffff))
#define OR1K_SPR_SYS_VR2_VER_SET(X, Y) (((X) & OR1K_UNSIGNED(0xff000000)) | ((Y) << 0))

/* CPU Identification Number */
#define OR1K_SPR_SYS_VR2_CPUID_LSB    24
#define OR1K_SPR_SYS_VR2_CPUID_MSB    31
#define OR1K_SPR_SYS_VR2_CPUID_BITS   8
#define OR1K_SPR_SYS_VR2_CPUID_MASK   OR1K_UNSIGNED(0xff000000)
#define OR1K_SPR_SYS_VR2_CPUID_GET(X) (((X) >> 24) & OR1K_UNSIGNED(0x000000ff))
#define OR1K_SPR_SYS_VR2_CPUID_SET(X, Y) (((X) & OR1K_UNSIGNED(0x00ffffff)) | ((Y) << 24))


/* Architecture Version Register */
#define OR1K_SPR_SYS_AVR_INDEX OR1K_UNSIGNED(0x00a)
#define OR1K_SPR_SYS_AVR_ADDR  OR1K_UNSIGNED(0x000a)

/* Major Architecture Version Number */
#define OR1K_SPR_SYS_AVR_REV_LSB    8
#define OR1K_SPR_SYS_AVR_REV_MSB    15
#define OR1K_SPR_SYS_AVR_REV_BITS   8
#define OR1K_SPR_SYS_AVR_REV_MASK   OR1K_UNSIGNED(0x0000ff00)
#define OR1K_SPR_SYS_AVR_REV_GET(X) (((X) >> 8) & OR1K_UNSIGNED(0x000000ff))
#define OR1K_SPR_SYS_AVR_REV_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffff00ff)) | ((Y) << 8))

/* Minor Architecture Version Number */
#define OR1K_SPR_SYS_AVR_MIN_LSB    16
#define OR1K_SPR_SYS_AVR_MIN_MSB    23
#define OR1K_SPR_SYS_AVR_MIN_BITS   8
#define OR1K_SPR_SYS_AVR_MIN_MASK   OR1K_UNSIGNED(0x00ff0000)
#define OR1K_SPR_SYS_AVR_MIN_GET(X) (((X) >> 16) & OR1K_UNSIGNED(0x000000ff))
#define OR1K_SPR_SYS_AVR_MIN_SET(X, Y) (((X) & OR1K_UNSIGNED(0xff00ffff)) | ((Y) << 16))

/* Architecture Revision Number */
#define OR1K_SPR_SYS_AVR_MAJ_LSB    24
#define OR1K_SPR_SYS_AVR_MAJ_MSB    31
#define OR1K_SPR_SYS_AVR_MAJ_BITS   8
#define OR1K_SPR_SYS_AVR_MAJ_MASK   OR1K_UNSIGNED(0xff000000)
#define OR1K_SPR_SYS_AVR_MAJ_GET(X) (((X) >> 24) & OR1K_UNSIGNED(0x000000ff))
#define OR1K_SPR_SYS_AVR_MAJ_SET(X, Y) (((X) & OR1K_UNSIGNED(0x00ffffff)) | ((Y) << 24))


/* Exception Vector Base Address Register */
#define OR1K_SPR_SYS_EVBAR_INDEX OR1K_UNSIGNED(0x00b)
#define OR1K_SPR_SYS_EVBAR_ADDR  OR1K_UNSIGNED(0x000b)

/* Exception Vector Base Address */
#define OR1K_SPR_SYS_EVBAR_EVBA_LSB    13
#define OR1K_SPR_SYS_EVBAR_EVBA_MSB    31
#define OR1K_SPR_SYS_EVBAR_EVBA_BITS   19
#define OR1K_SPR_SYS_EVBAR_EVBA_MASK   OR1K_UNSIGNED(0xffffe000)
#define OR1K_SPR_SYS_EVBAR_EVBA_GET(X) (((X) >> 13) & OR1K_UNSIGNED(0x0007ffff))
#define OR1K_SPR_SYS_EVBAR_EVBA_SET(X, Y) (((X) & OR1K_UNSIGNED(0x00001fff)) | ((Y) << 13))


/* Arithmetic Exception Control Register */
#define OR1K_SPR_SYS_AECR_INDEX OR1K_UNSIGNED(0x00c)
#define OR1K_SPR_SYS_AECR_ADDR  OR1K_UNSIGNED(0x000c)

/* Carry on Add Exception Enabled */
#define OR1K_SPR_SYS_AECR_CYADDE_OFFSET 0
#define OR1K_SPR_SYS_AECR_CYADDE_MASK   0x00000001
#define OR1K_SPR_SYS_AECR_CYADDE_GET(X) (((X) >> 0) & 0x1)
#define OR1K_SPR_SYS_AECR_CYADDE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffffe)) | ((!!(Y)) << 0))

/* Overflow on Add Exception Enabled */
#define OR1K_SPR_SYS_AECR_OVADDE_OFFSET 1
#define OR1K_SPR_SYS_AECR_OVADDE_MASK   0x00000002
#define OR1K_SPR_SYS_AECR_OVADDE_GET(X) (((X) >> 1) & 0x1)
#define OR1K_SPR_SYS_AECR_OVADDE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffffd)) | ((!!(Y)) << 1))

/* Carry on Multiply Exception Enabled */
#define OR1K_SPR_SYS_AECR_CYMULE_OFFSET 2
#define OR1K_SPR_SYS_AECR_CYMULE_MASK   0x00000004
#define OR1K_SPR_SYS_AECR_CYMULE_GET(X) (((X) >> 2) & 0x1)
#define OR1K_SPR_SYS_AECR_CYMULE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffffb)) | ((!!(Y)) << 2))

/* Overflow on Multiply Exception Enabled */
#define OR1K_SPR_SYS_AECR_OVMULE_OFFSET 3
#define OR1K_SPR_SYS_AECR_OVMULE_MASK   0x00000008
#define OR1K_SPR_SYS_AECR_OVMULE_GET(X) (((X) >> 3) & 0x1)
#define OR1K_SPR_SYS_AECR_OVMULE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffff7)) | ((!!(Y)) << 3))

/* Divide by Zero Exception Enabled */
#define OR1K_SPR_SYS_AECR_DBZE_OFFSET 4
#define OR1K_SPR_SYS_AECR_DBZE_MASK   0x00000010
#define OR1K_SPR_SYS_AECR_DBZE_GET(X) (((X) >> 4) & 0x1)
#define OR1K_SPR_SYS_AECR_DBZE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffffef)) | ((!!(Y)) << 4))

/* Carry on MAC Addition Exception Enabled */
#define OR1K_SPR_SYS_AECR_CYMACADDE_OFFSET 5
#define OR1K_SPR_SYS_AECR_CYMACADDE_MASK   0x00000020
#define OR1K_SPR_SYS_AECR_CYMACADDE_GET(X) (((X) >> 5) & 0x1)
#define OR1K_SPR_SYS_AECR_CYMACADDE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffffdf)) | ((!!(Y)) << 5))

/* Overflow on MAC Addition Exception Enabled */
#define OR1K_SPR_SYS_AECR_OVMACADDE_OFFSET 6
#define OR1K_SPR_SYS_AECR_OVMACADDE_MASK   0x00000040
#define OR1K_SPR_SYS_AECR_OVMACADDE_GET(X) (((X) >> 6) & 0x1)
#define OR1K_SPR_SYS_AECR_OVMACADDE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffffbf)) | ((!!(Y)) << 6))


/* Arithmetic Exception Status Register */
#define OR1K_SPR_SYS_AESR_INDEX OR1K_UNSIGNED(0x00d)
#define OR1K_SPR_SYS_AESR_ADDR  OR1K_UNSIGNED(0x000d)

/* Carry on Add Exception */
#define OR1K_SPR_SYS_AESR_CYADDE_OFFSET 0
#define OR1K_SPR_SYS_AESR_CYADDE_MASK   0x00000001
#define OR1K_SPR_SYS_AESR_CYADDE_GET(X) (((X) >> 0) & 0x1)
#define OR1K_SPR_SYS_AESR_CYADDE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffffe)) | ((!!(Y)) << 0))

/* Overflow on Add Exception */
#define OR1K_SPR_SYS_AESR_OVADDE_OFFSET 1
#define OR1K_SPR_SYS_AESR_OVADDE_MASK   0x00000002
#define OR1K_SPR_SYS_AESR_OVADDE_GET(X) (((X) >> 1) & 0x1)
#define OR1K_SPR_SYS_AESR_OVADDE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffffd)) | ((!!(Y)) << 1))

/* Carry on Multiply Exception */
#define OR1K_SPR_SYS_AESR_CYMULE_OFFSET 2
#define OR1K_SPR_SYS_AESR_CYMULE_MASK   0x00000004
#define OR1K_SPR_SYS_AESR_CYMULE_GET(X) (((X) >> 2) & 0x1)
#define OR1K_SPR_SYS_AESR_CYMULE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffffb)) | ((!!(Y)) << 2))

/* Overflow on Multiply Exception */
#define OR1K_SPR_SYS_AESR_OVMULE_OFFSET 3
#define OR1K_SPR_SYS_AESR_OVMULE_MASK   0x00000008
#define OR1K_SPR_SYS_AESR_OVMULE_GET(X) (((X) >> 3) & 0x1)
#define OR1K_SPR_SYS_AESR_OVMULE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffff7)) | ((!!(Y)) << 3))

/* Divide by Zero Exception */
#define OR1K_SPR_SYS_AESR_DBZE_OFFSET 4
#define OR1K_SPR_SYS_AESR_DBZE_MASK   0x00000010
#define OR1K_SPR_SYS_AESR_DBZE_GET(X) (((X) >> 4) & 0x1)
#define OR1K_SPR_SYS_AESR_DBZE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffffef)) | ((!!(Y)) << 4))

/* Carry on MAC Addition Exception */
#define OR1K_SPR_SYS_AESR_CYMACADDE_OFFSET 5
#define OR1K_SPR_SYS_AESR_CYMACADDE_MASK   0x00000020
#define OR1K_SPR_SYS_AESR_CYMACADDE_GET(X) (((X) >> 5) & 0x1)
#define OR1K_SPR_SYS_AESR_CYMACADDE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffffdf)) | ((!!(Y)) << 5))

/* Overflow on MAC Addition Exception */
#define OR1K_SPR_SYS_AESR_OVMACADDE_OFFSET 6
#define OR1K_SPR_SYS_AESR_OVMACADDE_MASK   0x00000040
#define OR1K_SPR_SYS_AESR_OVMACADDE_GET(X) (((X) >> 6) & 0x1)
#define OR1K_SPR_SYS_AESR_OVMACADDE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffffbf)) | ((!!(Y)) << 6))


/* Next Program Counter */
#define OR1K_SPR_SYS_NPC_INDEX OR1K_UNSIGNED(0x010)
#define OR1K_SPR_SYS_NPC_ADDR  OR1K_UNSIGNED(0x0010)


/* Supervision Register */
#define OR1K_SPR_SYS_SR_INDEX OR1K_UNSIGNED(0x011)
#define OR1K_SPR_SYS_SR_ADDR  OR1K_UNSIGNED(0x0011)

/* Supervisor Mode */
#define OR1K_SPR_SYS_SR_SM_OFFSET 0
#define OR1K_SPR_SYS_SR_SM_MASK   0x00000001
#define OR1K_SPR_SYS_SR_SM_GET(X) (((X) >> 0) & 0x1)
#define OR1K_SPR_SYS_SR_SM_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffffe)) | ((!!(Y)) << 0))

/* Tick Timer Exception Enabled */
#define OR1K_SPR_SYS_SR_TEE_OFFSET 1
#define OR1K_SPR_SYS_SR_TEE_MASK   0x00000002
#define OR1K_SPR_SYS_SR_TEE_GET(X) (((X) >> 1) & 0x1)
#define OR1K_SPR_SYS_SR_TEE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffffd)) | ((!!(Y)) << 1))

/* Interrupt Exception Enabled */
#define OR1K_SPR_SYS_SR_IEE_OFFSET 2
#define OR1K_SPR_SYS_SR_IEE_MASK   0x00000004
#define OR1K_SPR_SYS_SR_IEE_GET(X) (((X) >> 2) & 0x1)
#define OR1K_SPR_SYS_SR_IEE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffffb)) | ((!!(Y)) << 2))

/* Data Cache Enabled */
#define OR1K_SPR_SYS_SR_DCE_OFFSET 3
#define OR1K_SPR_SYS_SR_DCE_MASK   0x00000008
#define OR1K_SPR_SYS_SR_DCE_GET(X) (((X) >> 3) & 0x1)
#define OR1K_SPR_SYS_SR_DCE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffff7)) | ((!!(Y)) << 3))

/* Instruction Cache Enabled */
#define OR1K_SPR_SYS_SR_ICE_OFFSET 4
#define OR1K_SPR_SYS_SR_ICE_MASK   0x00000010
#define OR1K_SPR_SYS_SR_ICE_GET(X) (((X) >> 4) & 0x1)
#define OR1K_SPR_SYS_SR_ICE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffffef)) | ((!!(Y)) << 4))

/* Data MMU Enabled */
#define OR1K_SPR_SYS_SR_DME_OFFSET 5
#define OR1K_SPR_SYS_SR_DME_MASK   0x00000020
#define OR1K_SPR_SYS_SR_DME_GET(X) (((X) >> 5) & 0x1)
#define OR1K_SPR_SYS_SR_DME_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffffdf)) | ((!!(Y)) << 5))

/* Instruction MMU Enabled */
#define OR1K_SPR_SYS_SR_IME_OFFSET 6
#define OR1K_SPR_SYS_SR_IME_MASK   0x00000040
#define OR1K_SPR_SYS_SR_IME_GET(X) (((X) >> 6) & 0x1)
#define OR1K_SPR_SYS_SR_IME_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffffbf)) | ((!!(Y)) << 6))

/* Little Endian Enabled */
#define OR1K_SPR_SYS_SR_LEE_OFFSET 7
#define OR1K_SPR_SYS_SR_LEE_MASK   0x00000080
#define OR1K_SPR_SYS_SR_LEE_GET(X) (((X) >> 7) & 0x1)
#define OR1K_SPR_SYS_SR_LEE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffff7f)) | ((!!(Y)) << 7))

/* CID Enable */
#define OR1K_SPR_SYS_SR_CE_OFFSET 8
#define OR1K_SPR_SYS_SR_CE_MASK   0x00000100
#define OR1K_SPR_SYS_SR_CE_GET(X) (((X) >> 8) & 0x1)
#define OR1K_SPR_SYS_SR_CE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffeff)) | ((!!(Y)) << 8))

/* Flag */
#define OR1K_SPR_SYS_SR_F_OFFSET 9
#define OR1K_SPR_SYS_SR_F_MASK   0x00000200
#define OR1K_SPR_SYS_SR_F_GET(X) (((X) >> 9) & 0x1)
#define OR1K_SPR_SYS_SR_F_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffdff)) | ((!!(Y)) << 9))

/* Carry */
#define OR1K_SPR_SYS_SR_CY_OFFSET 10
#define OR1K_SPR_SYS_SR_CY_MASK   0x00000400
#define OR1K_SPR_SYS_SR_CY_GET(X) (((X) >> 10) & 0x1)
#define OR1K_SPR_SYS_SR_CY_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffbff)) | ((!!(Y)) << 10))

/* Overflow */
#define OR1K_SPR_SYS_SR_OV_OFFSET 11
#define OR1K_SPR_SYS_SR_OV_MASK   0x00000800
#define OR1K_SPR_SYS_SR_OV_GET(X) (((X) >> 11) & 0x1)
#define OR1K_SPR_SYS_SR_OV_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffff7ff)) | ((!!(Y)) << 11))

/* Overflow Exception Enabled */
#define OR1K_SPR_SYS_SR_OVE_OFFSET 12
#define OR1K_SPR_SYS_SR_OVE_MASK   0x00001000
#define OR1K_SPR_SYS_SR_OVE_GET(X) (((X) >> 12) & 0x1)
#define OR1K_SPR_SYS_SR_OVE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffefff)) | ((!!(Y)) << 12))

/* Delay-slot Exception */
#define OR1K_SPR_SYS_SR_DSX_OFFSET 13
#define OR1K_SPR_SYS_SR_DSX_MASK   0x00002000
#define OR1K_SPR_SYS_SR_DSX_GET(X) (((X) >> 13) & 0x1)
#define OR1K_SPR_SYS_SR_DSX_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffdfff)) | ((!!(Y)) << 13))

/* Exception Prefix High */
#define OR1K_SPR_SYS_SR_EPH_OFFSET 14
#define OR1K_SPR_SYS_SR_EPH_MASK   0x00004000
#define OR1K_SPR_SYS_SR_EPH_GET(X) (((X) >> 14) & 0x1)
#define OR1K_SPR_SYS_SR_EPH_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffbfff)) | ((!!(Y)) << 14))

/* Fixed One */
#define OR1K_SPR_SYS_SR_FO_OFFSET 15
#define OR1K_SPR_SYS_SR_FO_MASK   0x00008000
#define OR1K_SPR_SYS_SR_FO_GET(X) (((X) >> 15) & 0x1)
#define OR1K_SPR_SYS_SR_FO_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffff7fff)) | ((!!(Y)) << 15))

/* SPR User Mode Read Access */
#define OR1K_SPR_SYS_SR_SUMRA_OFFSET 16
#define OR1K_SPR_SYS_SR_SUMRA_MASK   0x00010000
#define OR1K_SPR_SYS_SR_SUMRA_GET(X) (((X) >> 16) & 0x1)
#define OR1K_SPR_SYS_SR_SUMRA_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffeffff)) | ((!!(Y)) << 16))

/* Context ID */
#define OR1K_SPR_SYS_SR_CID_LSB    28
#define OR1K_SPR_SYS_SR_CID_MSB    31
#define OR1K_SPR_SYS_SR_CID_BITS   4
#define OR1K_SPR_SYS_SR_CID_MASK   OR1K_UNSIGNED(0xf0000000)
#define OR1K_SPR_SYS_SR_CID_GET(X) (((X) >> 28) & OR1K_UNSIGNED(0x0000000f))
#define OR1K_SPR_SYS_SR_CID_SET(X, Y) (((X) & OR1K_UNSIGNED(0x0fffffff)) | ((Y) << 28))


/* Previous Program Counter */
#define OR1K_SPR_SYS_PPC_INDEX OR1K_UNSIGNED(0x012)
#define OR1K_SPR_SYS_PPC_ADDR  OR1K_UNSIGNED(0x0012)


/* Floating Point Control Status Register */
#define OR1K_SPR_SYS_FPCSR_INDEX OR1K_UNSIGNED(0x014)
#define OR1K_SPR_SYS_FPCSR_ADDR  OR1K_UNSIGNED(0x0014)

/* Floating Point Exception Enabled */
#define OR1K_SPR_SYS_FPCSR_FPEE_OFFSET 0
#define OR1K_SPR_SYS_FPCSR_FPEE_MASK   0x00000001
#define OR1K_SPR_SYS_FPCSR_FPEE_GET(X) (((X) >> 0) & 0x1)
#define OR1K_SPR_SYS_FPCSR_FPEE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffffe)) | ((!!(Y)) << 0))

/* Rounding Mode */
#define OR1K_SPR_SYS_FPCSR_RM_LSB    1
#define OR1K_SPR_SYS_FPCSR_RM_MSB    2
#define OR1K_SPR_SYS_FPCSR_RM_BITS   2
#define OR1K_SPR_SYS_FPCSR_RM_MASK   OR1K_UNSIGNED(0x00000006)
#define OR1K_SPR_SYS_FPCSR_RM_GET(X) (((X) >> 1) & OR1K_UNSIGNED(0x00000003))
#define OR1K_SPR_SYS_FPCSR_RM_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffff9)) | ((Y) << 1))

/* Round to nearest */
#define OR1K_SPR_SYS_FPCSR_RM_NEAREST 0
/* Round to zero */
#define OR1K_SPR_SYS_FPCSR_RM_ZERO 1
/* Round to infinity+ */
#define OR1K_SPR_SYS_FPCSR_RM_INFPLUS 2
/* Round to infinity- */
#define OR1K_SPR_SYS_FPCSR_RM_INFMINUS 3
/* Overflow Flag */
#define OR1K_SPR_SYS_FPCSR_OVF_OFFSET 3
#define OR1K_SPR_SYS_FPCSR_OVF_MASK   0x00000008
#define OR1K_SPR_SYS_FPCSR_OVF_GET(X) (((X) >> 3) & 0x1)
#define OR1K_SPR_SYS_FPCSR_OVF_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffff7)) | ((!!(Y)) << 3))

/* Underflow Flag */
#define OR1K_SPR_SYS_FPCSR_UNF_OFFSET 4
#define OR1K_SPR_SYS_FPCSR_UNF_MASK   0x00000010
#define OR1K_SPR_SYS_FPCSR_UNF_GET(X) (((X) >> 4) & 0x1)
#define OR1K_SPR_SYS_FPCSR_UNF_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffffef)) | ((!!(Y)) << 4))

/* SNAN Flag */
#define OR1K_SPR_SYS_FPCSR_SNF_OFFSET 5
#define OR1K_SPR_SYS_FPCSR_SNF_MASK   0x00000020
#define OR1K_SPR_SYS_FPCSR_SNF_GET(X) (((X) >> 5) & 0x1)
#define OR1K_SPR_SYS_FPCSR_SNF_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffffdf)) | ((!!(Y)) << 5))

/* QNAN Flag */
#define OR1K_SPR_SYS_FPCSR_QNF_OFFSET 6
#define OR1K_SPR_SYS_FPCSR_QNF_MASK   0x00000040
#define OR1K_SPR_SYS_FPCSR_QNF_GET(X) (((X) >> 6) & 0x1)
#define OR1K_SPR_SYS_FPCSR_QNF_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffffbf)) | ((!!(Y)) << 6))

/* Zero Flag */
#define OR1K_SPR_SYS_FPCSR_ZF_OFFSET 7
#define OR1K_SPR_SYS_FPCSR_ZF_MASK   0x00000080
#define OR1K_SPR_SYS_FPCSR_ZF_GET(X) (((X) >> 7) & 0x1)
#define OR1K_SPR_SYS_FPCSR_ZF_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffff7f)) | ((!!(Y)) << 7))

/* Inexact Flag */
#define OR1K_SPR_SYS_FPCSR_IXF_OFFSET 8
#define OR1K_SPR_SYS_FPCSR_IXF_MASK   0x00000100
#define OR1K_SPR_SYS_FPCSR_IXF_GET(X) (((X) >> 8) & 0x1)
#define OR1K_SPR_SYS_FPCSR_IXF_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffeff)) | ((!!(Y)) << 8))

/* Invalid Flag */
#define OR1K_SPR_SYS_FPCSR_IVF_OFFSET 9
#define OR1K_SPR_SYS_FPCSR_IVF_MASK   0x00000200
#define OR1K_SPR_SYS_FPCSR_IVF_GET(X) (((X) >> 9) & 0x1)
#define OR1K_SPR_SYS_FPCSR_IVF_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffdff)) | ((!!(Y)) << 9))

/* Infinity Flag */
#define OR1K_SPR_SYS_FPCSR_INF_OFFSET 10
#define OR1K_SPR_SYS_FPCSR_INF_MASK   0x00000400
#define OR1K_SPR_SYS_FPCSR_INF_GET(X) (((X) >> 10) & 0x1)
#define OR1K_SPR_SYS_FPCSR_INF_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffbff)) | ((!!(Y)) << 10))

/* Divide by Zero Flag */
#define OR1K_SPR_SYS_FPCSR_DZF_OFFSET 11
#define OR1K_SPR_SYS_FPCSR_DZF_MASK   0x00000800
#define OR1K_SPR_SYS_FPCSR_DZF_GET(X) (((X) >> 11) & 0x1)
#define OR1K_SPR_SYS_FPCSR_DZF_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffff7ff)) | ((!!(Y)) << 11))


/* Implementation-specific Registers */
#define OR1K_SPR_SYS_ISR_BASE     OR1K_UNSIGNED(0x015)
#define OR1K_SPR_SYS_ISR_COUNT    OR1K_UNSIGNED(0x008)
#define OR1K_SPR_SYS_ISR_STEP     OR1K_UNSIGNED(0x001)
#define OR1K_SPR_SYS_ISR_INDEX(N) (OR1K_SPR_SYS_ISR_BASE + ((N) * OR1K_SPR_SYS_ISR_STEP))
#define OR1K_SPR_SYS_ISR_ADDR(N)  ((OR1K_SPR_SYS_GROUP << OR1K_SPR_GROUP_LSB) | OR1K_SPR_SYS_ISR_INDEX(N))


/* Exception PC Registers */
#define OR1K_SPR_SYS_EPCR_BASE     OR1K_UNSIGNED(0x020)
#define OR1K_SPR_SYS_EPCR_COUNT    OR1K_UNSIGNED(0x010)
#define OR1K_SPR_SYS_EPCR_STEP     OR1K_UNSIGNED(0x001)
#define OR1K_SPR_SYS_EPCR_INDEX(N) (OR1K_SPR_SYS_EPCR_BASE + ((N) * OR1K_SPR_SYS_EPCR_STEP))
#define OR1K_SPR_SYS_EPCR_ADDR(N)  ((OR1K_SPR_SYS_GROUP << OR1K_SPR_GROUP_LSB) | OR1K_SPR_SYS_EPCR_INDEX(N))


/* Exception Effective Address Registers */
#define OR1K_SPR_SYS_EEAR_BASE     OR1K_UNSIGNED(0x030)
#define OR1K_SPR_SYS_EEAR_COUNT    OR1K_UNSIGNED(0x010)
#define OR1K_SPR_SYS_EEAR_STEP     OR1K_UNSIGNED(0x001)
#define OR1K_SPR_SYS_EEAR_INDEX(N) (OR1K_SPR_SYS_EEAR_BASE + ((N) * OR1K_SPR_SYS_EEAR_STEP))
#define OR1K_SPR_SYS_EEAR_ADDR(N)  ((OR1K_SPR_SYS_GROUP << OR1K_SPR_GROUP_LSB) | OR1K_SPR_SYS_EEAR_INDEX(N))


/* Exception Supervision Registers */
#define OR1K_SPR_SYS_ESR_BASE     OR1K_UNSIGNED(0x040)
#define OR1K_SPR_SYS_ESR_COUNT    OR1K_UNSIGNED(0x010)
#define OR1K_SPR_SYS_ESR_STEP     OR1K_UNSIGNED(0x001)
#define OR1K_SPR_SYS_ESR_INDEX(N) (OR1K_SPR_SYS_ESR_BASE + ((N) * OR1K_SPR_SYS_ESR_STEP))
#define OR1K_SPR_SYS_ESR_ADDR(N)  ((OR1K_SPR_SYS_GROUP << OR1K_SPR_GROUP_LSB) | OR1K_SPR_SYS_ESR_INDEX(N))

/* Supervisor Mode */
#define OR1K_SPR_SYS_ESR_SM_OFFSET 0
#define OR1K_SPR_SYS_ESR_SM_MASK   0x00000001
#define OR1K_SPR_SYS_ESR_SM_GET(X) (((X) >> 0) & 0x1)
#define OR1K_SPR_SYS_ESR_SM_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffffe)) | ((!!(Y)) << 0))

/* Tick Timer Exception Enabled */
#define OR1K_SPR_SYS_ESR_TEE_OFFSET 1
#define OR1K_SPR_SYS_ESR_TEE_MASK   0x00000002
#define OR1K_SPR_SYS_ESR_TEE_GET(X) (((X) >> 1) & 0x1)
#define OR1K_SPR_SYS_ESR_TEE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffffd)) | ((!!(Y)) << 1))

/* Interrupt Exception Enabled */
#define OR1K_SPR_SYS_ESR_IEE_OFFSET 2
#define OR1K_SPR_SYS_ESR_IEE_MASK   0x00000004
#define OR1K_SPR_SYS_ESR_IEE_GET(X) (((X) >> 2) & 0x1)
#define OR1K_SPR_SYS_ESR_IEE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffffb)) | ((!!(Y)) << 2))

/* Data Cache Enabled */
#define OR1K_SPR_SYS_ESR_DCE_OFFSET 3
#define OR1K_SPR_SYS_ESR_DCE_MASK   0x00000008
#define OR1K_SPR_SYS_ESR_DCE_GET(X) (((X) >> 3) & 0x1)
#define OR1K_SPR_SYS_ESR_DCE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffff7)) | ((!!(Y)) << 3))

/* Instruction Cache Enabled */
#define OR1K_SPR_SYS_ESR_ICE_OFFSET 4
#define OR1K_SPR_SYS_ESR_ICE_MASK   0x00000010
#define OR1K_SPR_SYS_ESR_ICE_GET(X) (((X) >> 4) & 0x1)
#define OR1K_SPR_SYS_ESR_ICE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffffef)) | ((!!(Y)) << 4))

/* Data MMU Enabled */
#define OR1K_SPR_SYS_ESR_DME_OFFSET 5
#define OR1K_SPR_SYS_ESR_DME_MASK   0x00000020
#define OR1K_SPR_SYS_ESR_DME_GET(X) (((X) >> 5) & 0x1)
#define OR1K_SPR_SYS_ESR_DME_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffffdf)) | ((!!(Y)) << 5))

/* Instruction MMU Enabled */
#define OR1K_SPR_SYS_ESR_IME_OFFSET 6
#define OR1K_SPR_SYS_ESR_IME_MASK   0x00000040
#define OR1K_SPR_SYS_ESR_IME_GET(X) (((X) >> 6) & 0x1)
#define OR1K_SPR_SYS_ESR_IME_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffffbf)) | ((!!(Y)) << 6))

/* Little Endian Enabled */
#define OR1K_SPR_SYS_ESR_LEE_OFFSET 7
#define OR1K_SPR_SYS_ESR_LEE_MASK   0x00000080
#define OR1K_SPR_SYS_ESR_LEE_GET(X) (((X) >> 7) & 0x1)
#define OR1K_SPR_SYS_ESR_LEE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffff7f)) | ((!!(Y)) << 7))

/* CID Enable */
#define OR1K_SPR_SYS_ESR_CE_OFFSET 8
#define OR1K_SPR_SYS_ESR_CE_MASK   0x00000100
#define OR1K_SPR_SYS_ESR_CE_GET(X) (((X) >> 8) & 0x1)
#define OR1K_SPR_SYS_ESR_CE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffeff)) | ((!!(Y)) << 8))

/* Flag */
#define OR1K_SPR_SYS_ESR_F_OFFSET 9
#define OR1K_SPR_SYS_ESR_F_MASK   0x00000200
#define OR1K_SPR_SYS_ESR_F_GET(X) (((X) >> 9) & 0x1)
#define OR1K_SPR_SYS_ESR_F_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffdff)) | ((!!(Y)) << 9))

/* Carry */
#define OR1K_SPR_SYS_ESR_CY_OFFSET 10
#define OR1K_SPR_SYS_ESR_CY_MASK   0x00000400
#define OR1K_SPR_SYS_ESR_CY_GET(X) (((X) >> 10) & 0x1)
#define OR1K_SPR_SYS_ESR_CY_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffbff)) | ((!!(Y)) << 10))

/* Overflow */
#define OR1K_SPR_SYS_ESR_OV_OFFSET 11
#define OR1K_SPR_SYS_ESR_OV_MASK   0x00000800
#define OR1K_SPR_SYS_ESR_OV_GET(X) (((X) >> 11) & 0x1)
#define OR1K_SPR_SYS_ESR_OV_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffff7ff)) | ((!!(Y)) << 11))

/* Overflow Exception Enabled */
#define OR1K_SPR_SYS_ESR_OVE_OFFSET 12
#define OR1K_SPR_SYS_ESR_OVE_MASK   0x00001000
#define OR1K_SPR_SYS_ESR_OVE_GET(X) (((X) >> 12) & 0x1)
#define OR1K_SPR_SYS_ESR_OVE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffefff)) | ((!!(Y)) << 12))

/* Delay-slot Exception */
#define OR1K_SPR_SYS_ESR_DSX_OFFSET 13
#define OR1K_SPR_SYS_ESR_DSX_MASK   0x00002000
#define OR1K_SPR_SYS_ESR_DSX_GET(X) (((X) >> 13) & 0x1)
#define OR1K_SPR_SYS_ESR_DSX_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffdfff)) | ((!!(Y)) << 13))

/* Exception Prefix High */
#define OR1K_SPR_SYS_ESR_EPH_OFFSET 14
#define OR1K_SPR_SYS_ESR_EPH_MASK   0x00004000
#define OR1K_SPR_SYS_ESR_EPH_GET(X) (((X) >> 14) & 0x1)
#define OR1K_SPR_SYS_ESR_EPH_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffbfff)) | ((!!(Y)) << 14))

/* Fixed One */
#define OR1K_SPR_SYS_ESR_FO_OFFSET 15
#define OR1K_SPR_SYS_ESR_FO_MASK   0x00008000
#define OR1K_SPR_SYS_ESR_FO_GET(X) (((X) >> 15) & 0x1)
#define OR1K_SPR_SYS_ESR_FO_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffff7fff)) | ((!!(Y)) << 15))

/* SPR User Mode Read Access */
#define OR1K_SPR_SYS_ESR_SUMRA_OFFSET 16
#define OR1K_SPR_SYS_ESR_SUMRA_MASK   0x00010000
#define OR1K_SPR_SYS_ESR_SUMRA_GET(X) (((X) >> 16) & 0x1)
#define OR1K_SPR_SYS_ESR_SUMRA_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffeffff)) | ((!!(Y)) << 16))

/* Context ID */
#define OR1K_SPR_SYS_ESR_CID_LSB    28
#define OR1K_SPR_SYS_ESR_CID_MSB    31
#define OR1K_SPR_SYS_ESR_CID_BITS   4
#define OR1K_SPR_SYS_ESR_CID_MASK   OR1K_UNSIGNED(0xf0000000)
#define OR1K_SPR_SYS_ESR_CID_GET(X) (((X) >> 28) & OR1K_UNSIGNED(0x0000000f))
#define OR1K_SPR_SYS_ESR_CID_SET(X, Y) (((X) & OR1K_UNSIGNED(0x0fffffff)) | ((Y) << 28))


/* Core identifier (multicore) */
#define OR1K_SPR_SYS_COREID_INDEX OR1K_UNSIGNED(0x080)
#define OR1K_SPR_SYS_COREID_ADDR  OR1K_UNSIGNED(0x0080)


/* Number of cores (multicore) */
#define OR1K_SPR_SYS_NUMCORES_INDEX OR1K_UNSIGNED(0x081)
#define OR1K_SPR_SYS_NUMCORES_ADDR  OR1K_UNSIGNED(0x0081)


/* General Purpose Registers */
#define OR1K_SPR_SYS_GPR_BASE     OR1K_UNSIGNED(0x400)
#define OR1K_SPR_SYS_GPR_COUNT    OR1K_UNSIGNED(0x100)
#define OR1K_SPR_SYS_GPR_STEP     OR1K_UNSIGNED(0x001)
#define OR1K_SPR_SYS_GPR_INDEX(N) (OR1K_SPR_SYS_GPR_BASE + ((N) * OR1K_SPR_SYS_GPR_STEP))
#define OR1K_SPR_SYS_GPR_ADDR(N)  ((OR1K_SPR_SYS_GROUP << OR1K_SPR_GROUP_LSB) | OR1K_SPR_SYS_GPR_INDEX(N))


/******************/
/* Data MMU Group */
/******************/
#define OR1K_SPR_DMMU_GROUP 0x01

/* Instruction MMU Control Register */
#define OR1K_SPR_DMMU_DMMUCR_INDEX OR1K_UNSIGNED(0x000)
#define OR1K_SPR_DMMU_DMMUCR_ADDR  OR1K_UNSIGNED(0x0800)

/* DTLB Flush */
#define OR1K_SPR_DMMU_DMMUCR_DTF_OFFSET 0
#define OR1K_SPR_DMMU_DMMUCR_DTF_MASK   0x00000001
#define OR1K_SPR_DMMU_DMMUCR_DTF_GET(X) (((X) >> 0) & 0x1)
#define OR1K_SPR_DMMU_DMMUCR_DTF_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffffe)) | ((!!(Y)) << 0))

/* Page Table Base Pointer */
#define OR1K_SPR_DMMU_DMMUCR_PTBP_LSB    10
#define OR1K_SPR_DMMU_DMMUCR_PTBP_MSB    31
#define OR1K_SPR_DMMU_DMMUCR_PTBP_BITS   22
#define OR1K_SPR_DMMU_DMMUCR_PTBP_MASK   OR1K_UNSIGNED(0xfffffc00)
#define OR1K_SPR_DMMU_DMMUCR_PTBP_GET(X) (((X) >> 10) & OR1K_UNSIGNED(0x003fffff))
#define OR1K_SPR_DMMU_DMMUCR_PTBP_SET(X, Y) (((X) & OR1K_UNSIGNED(0x000003ff)) | ((Y) << 10))


/* Data MMU Protection Register */
#define OR1K_SPR_DMMU_DMMUPR_INDEX OR1K_UNSIGNED(0x001)
#define OR1K_SPR_DMMU_DMMUPR_ADDR  OR1K_UNSIGNED(0x0801)

/* Supervisor Read Enable 1 */
#define OR1K_SPR_DMMU_DMMUPR_SRE1_OFFSET 0
#define OR1K_SPR_DMMU_DMMUPR_SRE1_MASK   0x00000001
#define OR1K_SPR_DMMU_DMMUPR_SRE1_GET(X) (((X) >> 0) & 0x1)
#define OR1K_SPR_DMMU_DMMUPR_SRE1_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffffe)) | ((!!(Y)) << 0))

/* Supervisor Write Enable 1 */
#define OR1K_SPR_DMMU_DMMUPR_SWE1_OFFSET 1
#define OR1K_SPR_DMMU_DMMUPR_SWE1_MASK   0x00000002
#define OR1K_SPR_DMMU_DMMUPR_SWE1_GET(X) (((X) >> 1) & 0x1)
#define OR1K_SPR_DMMU_DMMUPR_SWE1_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffffd)) | ((!!(Y)) << 1))

/* User Read Enable 1 */
#define OR1K_SPR_DMMU_DMMUPR_URE1_OFFSET 2
#define OR1K_SPR_DMMU_DMMUPR_URE1_MASK   0x00000004
#define OR1K_SPR_DMMU_DMMUPR_URE1_GET(X) (((X) >> 2) & 0x1)
#define OR1K_SPR_DMMU_DMMUPR_URE1_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffffb)) | ((!!(Y)) << 2))

/* User Write Enable 1 */
#define OR1K_SPR_DMMU_DMMUPR_UWE1_OFFSET 3
#define OR1K_SPR_DMMU_DMMUPR_UWE1_MASK   0x00000008
#define OR1K_SPR_DMMU_DMMUPR_UWE1_GET(X) (((X) >> 3) & 0x1)
#define OR1K_SPR_DMMU_DMMUPR_UWE1_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffff7)) | ((!!(Y)) << 3))

/* Supervisor Read Enable 2 */
#define OR1K_SPR_DMMU_DMMUPR_SRE2_OFFSET 4
#define OR1K_SPR_DMMU_DMMUPR_SRE2_MASK   0x00000010
#define OR1K_SPR_DMMU_DMMUPR_SRE2_GET(X) (((X) >> 4) & 0x1)
#define OR1K_SPR_DMMU_DMMUPR_SRE2_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffffef)) | ((!!(Y)) << 4))

/* Supervisor Write Enable 2 */
#define OR1K_SPR_DMMU_DMMUPR_SWE2_OFFSET 5
#define OR1K_SPR_DMMU_DMMUPR_SWE2_MASK   0x00000020
#define OR1K_SPR_DMMU_DMMUPR_SWE2_GET(X) (((X) >> 5) & 0x1)
#define OR1K_SPR_DMMU_DMMUPR_SWE2_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffffdf)) | ((!!(Y)) << 5))

/* User Read Enable 2 */
#define OR1K_SPR_DMMU_DMMUPR_URE2_OFFSET 6
#define OR1K_SPR_DMMU_DMMUPR_URE2_MASK   0x00000040
#define OR1K_SPR_DMMU_DMMUPR_URE2_GET(X) (((X) >> 6) & 0x1)
#define OR1K_SPR_DMMU_DMMUPR_URE2_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffffbf)) | ((!!(Y)) << 6))

/* User Write Enable 2 */
#define OR1K_SPR_DMMU_DMMUPR_UWE2_OFFSET 7
#define OR1K_SPR_DMMU_DMMUPR_UWE2_MASK   0x00000080
#define OR1K_SPR_DMMU_DMMUPR_UWE2_GET(X) (((X) >> 7) & 0x1)
#define OR1K_SPR_DMMU_DMMUPR_UWE2_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffff7f)) | ((!!(Y)) << 7))

/* Supervisor Read Enable 3 */
#define OR1K_SPR_DMMU_DMMUPR_SRE3_OFFSET 8
#define OR1K_SPR_DMMU_DMMUPR_SRE3_MASK   0x00000100
#define OR1K_SPR_DMMU_DMMUPR_SRE3_GET(X) (((X) >> 8) & 0x1)
#define OR1K_SPR_DMMU_DMMUPR_SRE3_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffeff)) | ((!!(Y)) << 8))

/* Supervisor Write Enable 3 */
#define OR1K_SPR_DMMU_DMMUPR_SWE3_OFFSET 9
#define OR1K_SPR_DMMU_DMMUPR_SWE3_MASK   0x00000200
#define OR1K_SPR_DMMU_DMMUPR_SWE3_GET(X) (((X) >> 9) & 0x1)
#define OR1K_SPR_DMMU_DMMUPR_SWE3_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffdff)) | ((!!(Y)) << 9))

/* User Read Enable 3 */
#define OR1K_SPR_DMMU_DMMUPR_URE3_OFFSET 10
#define OR1K_SPR_DMMU_DMMUPR_URE3_MASK   0x00000400
#define OR1K_SPR_DMMU_DMMUPR_URE3_GET(X) (((X) >> 10) & 0x1)
#define OR1K_SPR_DMMU_DMMUPR_URE3_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffbff)) | ((!!(Y)) << 10))

/* User Write Enable 3 */
#define OR1K_SPR_DMMU_DMMUPR_UWE3_OFFSET 11
#define OR1K_SPR_DMMU_DMMUPR_UWE3_MASK   0x00000800
#define OR1K_SPR_DMMU_DMMUPR_UWE3_GET(X) (((X) >> 11) & 0x1)
#define OR1K_SPR_DMMU_DMMUPR_UWE3_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffff7ff)) | ((!!(Y)) << 11))

/* Supervisor Read Enable 4 */
#define OR1K_SPR_DMMU_DMMUPR_SRE4_OFFSET 12
#define OR1K_SPR_DMMU_DMMUPR_SRE4_MASK   0x00001000
#define OR1K_SPR_DMMU_DMMUPR_SRE4_GET(X) (((X) >> 12) & 0x1)
#define OR1K_SPR_DMMU_DMMUPR_SRE4_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffefff)) | ((!!(Y)) << 12))

/* Supervisor Write Enable 4 */
#define OR1K_SPR_DMMU_DMMUPR_SWE4_OFFSET 13
#define OR1K_SPR_DMMU_DMMUPR_SWE4_MASK   0x00002000
#define OR1K_SPR_DMMU_DMMUPR_SWE4_GET(X) (((X) >> 13) & 0x1)
#define OR1K_SPR_DMMU_DMMUPR_SWE4_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffdfff)) | ((!!(Y)) << 13))

/* User Read Enable 4 */
#define OR1K_SPR_DMMU_DMMUPR_URE4_OFFSET 14
#define OR1K_SPR_DMMU_DMMUPR_URE4_MASK   0x00004000
#define OR1K_SPR_DMMU_DMMUPR_URE4_GET(X) (((X) >> 14) & 0x1)
#define OR1K_SPR_DMMU_DMMUPR_URE4_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffbfff)) | ((!!(Y)) << 14))

/* User Write Enable 4 */
#define OR1K_SPR_DMMU_DMMUPR_UWE4_OFFSET 15
#define OR1K_SPR_DMMU_DMMUPR_UWE4_MASK   0x00008000
#define OR1K_SPR_DMMU_DMMUPR_UWE4_GET(X) (((X) >> 15) & 0x1)
#define OR1K_SPR_DMMU_DMMUPR_UWE4_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffff7fff)) | ((!!(Y)) << 15))

/* Supervisor Read Enable 5 */
#define OR1K_SPR_DMMU_DMMUPR_SRE5_OFFSET 16
#define OR1K_SPR_DMMU_DMMUPR_SRE5_MASK   0x00010000
#define OR1K_SPR_DMMU_DMMUPR_SRE5_GET(X) (((X) >> 16) & 0x1)
#define OR1K_SPR_DMMU_DMMUPR_SRE5_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffeffff)) | ((!!(Y)) << 16))

/* Supervisor Write Enable 5 */
#define OR1K_SPR_DMMU_DMMUPR_SWE5_OFFSET 17
#define OR1K_SPR_DMMU_DMMUPR_SWE5_MASK   0x00020000
#define OR1K_SPR_DMMU_DMMUPR_SWE5_GET(X) (((X) >> 17) & 0x1)
#define OR1K_SPR_DMMU_DMMUPR_SWE5_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffdffff)) | ((!!(Y)) << 17))

/* User Read Enable 5 */
#define OR1K_SPR_DMMU_DMMUPR_URE5_OFFSET 18
#define OR1K_SPR_DMMU_DMMUPR_URE5_MASK   0x00040000
#define OR1K_SPR_DMMU_DMMUPR_URE5_GET(X) (((X) >> 18) & 0x1)
#define OR1K_SPR_DMMU_DMMUPR_URE5_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffbffff)) | ((!!(Y)) << 18))

/* User Write Enable 5 */
#define OR1K_SPR_DMMU_DMMUPR_UWE5_OFFSET 19
#define OR1K_SPR_DMMU_DMMUPR_UWE5_MASK   0x00080000
#define OR1K_SPR_DMMU_DMMUPR_UWE5_GET(X) (((X) >> 19) & 0x1)
#define OR1K_SPR_DMMU_DMMUPR_UWE5_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfff7ffff)) | ((!!(Y)) << 19))

/* Supervisor Read Enable 6 */
#define OR1K_SPR_DMMU_DMMUPR_SRE6_OFFSET 20
#define OR1K_SPR_DMMU_DMMUPR_SRE6_MASK   0x00100000
#define OR1K_SPR_DMMU_DMMUPR_SRE6_GET(X) (((X) >> 20) & 0x1)
#define OR1K_SPR_DMMU_DMMUPR_SRE6_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffefffff)) | ((!!(Y)) << 20))

/* Supervisor Write Enable 6 */
#define OR1K_SPR_DMMU_DMMUPR_SWE6_OFFSET 21
#define OR1K_SPR_DMMU_DMMUPR_SWE6_MASK   0x00200000
#define OR1K_SPR_DMMU_DMMUPR_SWE6_GET(X) (((X) >> 21) & 0x1)
#define OR1K_SPR_DMMU_DMMUPR_SWE6_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffdfffff)) | ((!!(Y)) << 21))

/* User Read Enable 6 */
#define OR1K_SPR_DMMU_DMMUPR_URE6_OFFSET 22
#define OR1K_SPR_DMMU_DMMUPR_URE6_MASK   0x00400000
#define OR1K_SPR_DMMU_DMMUPR_URE6_GET(X) (((X) >> 22) & 0x1)
#define OR1K_SPR_DMMU_DMMUPR_URE6_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffbfffff)) | ((!!(Y)) << 22))

/* User Write Enable 6 */
#define OR1K_SPR_DMMU_DMMUPR_UWE6_OFFSET 23
#define OR1K_SPR_DMMU_DMMUPR_UWE6_MASK   0x00800000
#define OR1K_SPR_DMMU_DMMUPR_UWE6_GET(X) (((X) >> 23) & 0x1)
#define OR1K_SPR_DMMU_DMMUPR_UWE6_SET(X, Y) (((X) & OR1K_UNSIGNED(0xff7fffff)) | ((!!(Y)) << 23))

/* Supervisor Read Enable 7 */
#define OR1K_SPR_DMMU_DMMUPR_SRE7_OFFSET 24
#define OR1K_SPR_DMMU_DMMUPR_SRE7_MASK   0x01000000
#define OR1K_SPR_DMMU_DMMUPR_SRE7_GET(X) (((X) >> 24) & 0x1)
#define OR1K_SPR_DMMU_DMMUPR_SRE7_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfeffffff)) | ((!!(Y)) << 24))

/* Supervisor Write Enable 7 */
#define OR1K_SPR_DMMU_DMMUPR_SWE7_OFFSET 25
#define OR1K_SPR_DMMU_DMMUPR_SWE7_MASK   0x02000000
#define OR1K_SPR_DMMU_DMMUPR_SWE7_GET(X) (((X) >> 25) & 0x1)
#define OR1K_SPR_DMMU_DMMUPR_SWE7_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfdffffff)) | ((!!(Y)) << 25))

/* User Read Enable 7 */
#define OR1K_SPR_DMMU_DMMUPR_URE7_OFFSET 26
#define OR1K_SPR_DMMU_DMMUPR_URE7_MASK   0x04000000
#define OR1K_SPR_DMMU_DMMUPR_URE7_GET(X) (((X) >> 26) & 0x1)
#define OR1K_SPR_DMMU_DMMUPR_URE7_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfbffffff)) | ((!!(Y)) << 26))

/* User Write Enable 7 */
#define OR1K_SPR_DMMU_DMMUPR_UWE7_OFFSET 27
#define OR1K_SPR_DMMU_DMMUPR_UWE7_MASK   0x08000000
#define OR1K_SPR_DMMU_DMMUPR_UWE7_GET(X) (((X) >> 27) & 0x1)
#define OR1K_SPR_DMMU_DMMUPR_UWE7_SET(X, Y) (((X) & OR1K_UNSIGNED(0xf7ffffff)) | ((!!(Y)) << 27))


/* Data TLB Entry Invalidate Register */
#define OR1K_SPR_DMMU_DTLBEIR_INDEX OR1K_UNSIGNED(0x002)
#define OR1K_SPR_DMMU_DTLBEIR_ADDR  OR1K_UNSIGNED(0x0802)


/* Data ATB Match Registers */
#define OR1K_SPR_DMMU_DATBMR_BASE     OR1K_UNSIGNED(0x004)
#define OR1K_SPR_DMMU_DATBMR_COUNT    OR1K_UNSIGNED(0x004)
#define OR1K_SPR_DMMU_DATBMR_STEP     OR1K_UNSIGNED(0x001)
#define OR1K_SPR_DMMU_DATBMR_INDEX(N) (OR1K_SPR_DMMU_DATBMR_BASE + ((N) * OR1K_SPR_DMMU_DATBMR_STEP))
#define OR1K_SPR_DMMU_DATBMR_ADDR(N)  ((OR1K_SPR_DMMU_GROUP << OR1K_SPR_GROUP_LSB) | OR1K_SPR_DMMU_DATBMR_INDEX(N))

/* Valid */
#define OR1K_SPR_DMMU_DATBMR_V_OFFSET 0
#define OR1K_SPR_DMMU_DATBMR_V_MASK   0x00000001
#define OR1K_SPR_DMMU_DATBMR_V_GET(X) (((X) >> 0) & 0x1)
#define OR1K_SPR_DMMU_DATBMR_V_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffffe)) | ((!!(Y)) << 0))

/* Context ID */
#define OR1K_SPR_DMMU_DATBMR_CID_LSB    1
#define OR1K_SPR_DMMU_DATBMR_CID_MSB    4
#define OR1K_SPR_DMMU_DATBMR_CID_BITS   4
#define OR1K_SPR_DMMU_DATBMR_CID_MASK   OR1K_UNSIGNED(0x0000001e)
#define OR1K_SPR_DMMU_DATBMR_CID_GET(X) (((X) >> 1) & OR1K_UNSIGNED(0x0000000f))
#define OR1K_SPR_DMMU_DATBMR_CID_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffffe1)) | ((Y) << 1))

/* Page Size */
#define OR1K_SPR_DMMU_DATBMR_PS_OFFSET 5
#define OR1K_SPR_DMMU_DATBMR_PS_MASK   0x00000020
#define OR1K_SPR_DMMU_DATBMR_PS_GET(X) (((X) >> 5) & 0x1)
#define OR1K_SPR_DMMU_DATBMR_PS_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffffdf)) | ((!!(Y)) << 5))

/* Virtual Page Number */
#define OR1K_SPR_DMMU_DATBMR_VPN_LSB    10
#define OR1K_SPR_DMMU_DATBMR_VPN_MSB    31
#define OR1K_SPR_DMMU_DATBMR_VPN_BITS   22
#define OR1K_SPR_DMMU_DATBMR_VPN_MASK   OR1K_UNSIGNED(0xfffffc00)
#define OR1K_SPR_DMMU_DATBMR_VPN_GET(X) (((X) >> 10) & OR1K_UNSIGNED(0x003fffff))
#define OR1K_SPR_DMMU_DATBMR_VPN_SET(X, Y) (((X) & OR1K_UNSIGNED(0x000003ff)) | ((Y) << 10))


/* Data ATB Translate Registers */
#define OR1K_SPR_DMMU_DATBTR_BASE     OR1K_UNSIGNED(0x008)
#define OR1K_SPR_DMMU_DATBTR_COUNT    OR1K_UNSIGNED(0x004)
#define OR1K_SPR_DMMU_DATBTR_STEP     OR1K_UNSIGNED(0x001)
#define OR1K_SPR_DMMU_DATBTR_INDEX(N) (OR1K_SPR_DMMU_DATBTR_BASE + ((N) * OR1K_SPR_DMMU_DATBTR_STEP))
#define OR1K_SPR_DMMU_DATBTR_ADDR(N)  ((OR1K_SPR_DMMU_GROUP << OR1K_SPR_GROUP_LSB) | OR1K_SPR_DMMU_DATBTR_INDEX(N))

/* Cache Coherency */
#define OR1K_SPR_DMMU_DATBTR_CC_OFFSET 0
#define OR1K_SPR_DMMU_DATBTR_CC_MASK   0x00000001
#define OR1K_SPR_DMMU_DATBTR_CC_GET(X) (((X) >> 0) & 0x1)
#define OR1K_SPR_DMMU_DATBTR_CC_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffffe)) | ((!!(Y)) << 0))

/* Cache Inhibit */
#define OR1K_SPR_DMMU_DATBTR_CI_OFFSET 1
#define OR1K_SPR_DMMU_DATBTR_CI_MASK   0x00000002
#define OR1K_SPR_DMMU_DATBTR_CI_GET(X) (((X) >> 1) & 0x1)
#define OR1K_SPR_DMMU_DATBTR_CI_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffffd)) | ((!!(Y)) << 1))

/* Write-back Cache */
#define OR1K_SPR_DMMU_DATBTR_WBC_OFFSET 2
#define OR1K_SPR_DMMU_DATBTR_WBC_MASK   0x00000004
#define OR1K_SPR_DMMU_DATBTR_WBC_GET(X) (((X) >> 2) & 0x1)
#define OR1K_SPR_DMMU_DATBTR_WBC_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffffb)) | ((!!(Y)) << 2))

/* Weakly-ordered Memory */
#define OR1K_SPR_DMMU_DATBTR_WOM_OFFSET 3
#define OR1K_SPR_DMMU_DATBTR_WOM_MASK   0x00000008
#define OR1K_SPR_DMMU_DATBTR_WOM_GET(X) (((X) >> 3) & 0x1)
#define OR1K_SPR_DMMU_DATBTR_WOM_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffff7)) | ((!!(Y)) << 3))

/* Accessed */
#define OR1K_SPR_DMMU_DATBTR_A_OFFSET 4
#define OR1K_SPR_DMMU_DATBTR_A_MASK   0x00000010
#define OR1K_SPR_DMMU_DATBTR_A_GET(X) (((X) >> 4) & 0x1)
#define OR1K_SPR_DMMU_DATBTR_A_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffffef)) | ((!!(Y)) << 4))

/* Dirty */
#define OR1K_SPR_DMMU_DATBTR_D_OFFSET 5
#define OR1K_SPR_DMMU_DATBTR_D_MASK   0x00000020
#define OR1K_SPR_DMMU_DATBTR_D_GET(X) (((X) >> 5) & 0x1)
#define OR1K_SPR_DMMU_DATBTR_D_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffffdf)) | ((!!(Y)) << 5))

/* Supervisor Read Enable */
#define OR1K_SPR_DMMU_DATBTR_SRE_OFFSET 6
#define OR1K_SPR_DMMU_DATBTR_SRE_MASK   0x00000040
#define OR1K_SPR_DMMU_DATBTR_SRE_GET(X) (((X) >> 6) & 0x1)
#define OR1K_SPR_DMMU_DATBTR_SRE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffffbf)) | ((!!(Y)) << 6))

/* Supervisor Write Enable */
#define OR1K_SPR_DMMU_DATBTR_SWE_OFFSET 7
#define OR1K_SPR_DMMU_DATBTR_SWE_MASK   0x00000080
#define OR1K_SPR_DMMU_DATBTR_SWE_GET(X) (((X) >> 7) & 0x1)
#define OR1K_SPR_DMMU_DATBTR_SWE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffff7f)) | ((!!(Y)) << 7))

/* User Read Enable */
#define OR1K_SPR_DMMU_DATBTR_URE_OFFSET 8
#define OR1K_SPR_DMMU_DATBTR_URE_MASK   0x00000100
#define OR1K_SPR_DMMU_DATBTR_URE_GET(X) (((X) >> 8) & 0x1)
#define OR1K_SPR_DMMU_DATBTR_URE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffeff)) | ((!!(Y)) << 8))

/* User Write Enable */
#define OR1K_SPR_DMMU_DATBTR_UWE_OFFSET 9
#define OR1K_SPR_DMMU_DATBTR_UWE_MASK   0x00000200
#define OR1K_SPR_DMMU_DATBTR_UWE_GET(X) (((X) >> 9) & 0x1)
#define OR1K_SPR_DMMU_DATBTR_UWE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffdff)) | ((!!(Y)) << 9))

/* Physical Page Number */
#define OR1K_SPR_DMMU_DATBTR_PPN_LSB    10
#define OR1K_SPR_DMMU_DATBTR_PPN_MSB    31
#define OR1K_SPR_DMMU_DATBTR_PPN_BITS   22
#define OR1K_SPR_DMMU_DATBTR_PPN_MASK   OR1K_UNSIGNED(0xfffffc00)
#define OR1K_SPR_DMMU_DATBTR_PPN_GET(X) (((X) >> 10) & OR1K_UNSIGNED(0x003fffff))
#define OR1K_SPR_DMMU_DATBTR_PPN_SET(X, Y) (((X) & OR1K_UNSIGNED(0x000003ff)) | ((Y) << 10))


/* Data TLB */
#define OR1K_SPR_DMMU_DTLBW_BASE  OR1K_UNSIGNED(0x200)
#define OR1K_SPR_DMMU_DTLBW_COUNT OR1K_UNSIGNED(0x004)
#define OR1K_SPR_DMMU_DTLBW_STEP  OR1K_UNSIGNED(0x100)
#define OR1K_SPR_DMMU_DTLBW_SUBBASE(N0) (OR1K_SPR_DMMU_DTLBW_BASE + ((N0)*OR1K_SPR_DMMU_DTLBW_STEP))

/* Data TLB Match Registers */
#define OR1K_SPR_DMMU_DTLBW_MR_BASE  OR1K_UNSIGNED(0x000)
#define OR1K_SPR_DMMU_DTLBW_MR_COUNT OR1K_UNSIGNED(0x080)
#define OR1K_SPR_DMMU_DTLBW_MR_STEP  OR1K_UNSIGNED(0x001)

#define OR1K_SPR_DMMU_DTLBW_MR_INDEX(N0, N1) (OR1K_SPR_DMMU_DTLBW_SUBBASE(N0) + OR1K_SPR_DMMU_DTLBW_MR_BASE + ((N1) * OR1K_SPR_DMMU_DTLBW_MR_STEP))
#define OR1K_SPR_DMMU_DTLBW_MR_ADDR(N0, N1)  ((OR1K_SPR_DMMU_GROUP << OR1K_SPR_GROUP_LSB) | OR1K_SPR_DMMU_DTLBW_MR_INDEX(N0, N1))

/* Valid */
#define OR1K_SPR_DMMU_DTLBW_MR_V_OFFSET 0
#define OR1K_SPR_DMMU_DTLBW_MR_V_MASK   0x00000001
#define OR1K_SPR_DMMU_DTLBW_MR_V_GET(X) (((X) >> 0) & 0x1)
#define OR1K_SPR_DMMU_DTLBW_MR_V_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffffe)) | ((!!(Y)) << 0))

/* Page Level 1 */
#define OR1K_SPR_DMMU_DTLBW_MR_PL1_OFFSET 1
#define OR1K_SPR_DMMU_DTLBW_MR_PL1_MASK   0x00000002
#define OR1K_SPR_DMMU_DTLBW_MR_PL1_GET(X) (((X) >> 1) & 0x1)
#define OR1K_SPR_DMMU_DTLBW_MR_PL1_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffffd)) | ((!!(Y)) << 1))

/* Context ID */
#define OR1K_SPR_DMMU_DTLBW_MR_CID_LSB    2
#define OR1K_SPR_DMMU_DTLBW_MR_CID_MSB    5
#define OR1K_SPR_DMMU_DTLBW_MR_CID_BITS   4
#define OR1K_SPR_DMMU_DTLBW_MR_CID_MASK   OR1K_UNSIGNED(0x0000003c)
#define OR1K_SPR_DMMU_DTLBW_MR_CID_GET(X) (((X) >> 2) & OR1K_UNSIGNED(0x0000000f))
#define OR1K_SPR_DMMU_DTLBW_MR_CID_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffffc3)) | ((Y) << 2))

/* Least Recently Used */
#define OR1K_SPR_DMMU_DTLBW_MR_LRU_LSB    6
#define OR1K_SPR_DMMU_DTLBW_MR_LRU_MSB    7
#define OR1K_SPR_DMMU_DTLBW_MR_LRU_BITS   2
#define OR1K_SPR_DMMU_DTLBW_MR_LRU_MASK   OR1K_UNSIGNED(0x000000c0)
#define OR1K_SPR_DMMU_DTLBW_MR_LRU_GET(X) (((X) >> 6) & OR1K_UNSIGNED(0x00000003))
#define OR1K_SPR_DMMU_DTLBW_MR_LRU_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffff3f)) | ((Y) << 6))

/* Virtual Page Number */
#define OR1K_SPR_DMMU_DTLBW_MR_VPN_LSB    13
#define OR1K_SPR_DMMU_DTLBW_MR_VPN_MSB    31
#define OR1K_SPR_DMMU_DTLBW_MR_VPN_BITS   19
#define OR1K_SPR_DMMU_DTLBW_MR_VPN_MASK   OR1K_UNSIGNED(0xffffe000)
#define OR1K_SPR_DMMU_DTLBW_MR_VPN_GET(X) (((X) >> 13) & OR1K_UNSIGNED(0x0007ffff))
#define OR1K_SPR_DMMU_DTLBW_MR_VPN_SET(X, Y) (((X) & OR1K_UNSIGNED(0x00001fff)) | ((Y) << 13))

/* Data TLB Translate Registers */
#define OR1K_SPR_DMMU_DTLBW_TR_BASE  OR1K_UNSIGNED(0x080)
#define OR1K_SPR_DMMU_DTLBW_TR_COUNT OR1K_UNSIGNED(0x080)
#define OR1K_SPR_DMMU_DTLBW_TR_STEP  OR1K_UNSIGNED(0x001)

#define OR1K_SPR_DMMU_DTLBW_TR_INDEX(N0, N1) (OR1K_SPR_DMMU_DTLBW_SUBBASE(N0) + OR1K_SPR_DMMU_DTLBW_TR_BASE + ((N1) * OR1K_SPR_DMMU_DTLBW_TR_STEP))
#define OR1K_SPR_DMMU_DTLBW_TR_ADDR(N0, N1)  ((OR1K_SPR_DMMU_GROUP << OR1K_SPR_GROUP_LSB) | OR1K_SPR_DMMU_DTLBW_TR_INDEX(N0, N1))

/* Cache Coherency */
#define OR1K_SPR_DMMU_DTLBW_TR_CC_OFFSET 0
#define OR1K_SPR_DMMU_DTLBW_TR_CC_MASK   0x00000001
#define OR1K_SPR_DMMU_DTLBW_TR_CC_GET(X) (((X) >> 0) & 0x1)
#define OR1K_SPR_DMMU_DTLBW_TR_CC_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffffe)) | ((!!(Y)) << 0))

/* Cache Inhibit */
#define OR1K_SPR_DMMU_DTLBW_TR_CI_OFFSET 1
#define OR1K_SPR_DMMU_DTLBW_TR_CI_MASK   0x00000002
#define OR1K_SPR_DMMU_DTLBW_TR_CI_GET(X) (((X) >> 1) & 0x1)
#define OR1K_SPR_DMMU_DTLBW_TR_CI_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffffd)) | ((!!(Y)) << 1))

/* Write-back Cache */
#define OR1K_SPR_DMMU_DTLBW_TR_WBC_OFFSET 2
#define OR1K_SPR_DMMU_DTLBW_TR_WBC_MASK   0x00000004
#define OR1K_SPR_DMMU_DTLBW_TR_WBC_GET(X) (((X) >> 2) & 0x1)
#define OR1K_SPR_DMMU_DTLBW_TR_WBC_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffffb)) | ((!!(Y)) << 2))

/* Weakly-ordered Memory */
#define OR1K_SPR_DMMU_DTLBW_TR_WOM_OFFSET 3
#define OR1K_SPR_DMMU_DTLBW_TR_WOM_MASK   0x00000008
#define OR1K_SPR_DMMU_DTLBW_TR_WOM_GET(X) (((X) >> 3) & 0x1)
#define OR1K_SPR_DMMU_DTLBW_TR_WOM_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffff7)) | ((!!(Y)) << 3))

/* Accessed */
#define OR1K_SPR_DMMU_DTLBW_TR_A_OFFSET 4
#define OR1K_SPR_DMMU_DTLBW_TR_A_MASK   0x00000010
#define OR1K_SPR_DMMU_DTLBW_TR_A_GET(X) (((X) >> 4) & 0x1)
#define OR1K_SPR_DMMU_DTLBW_TR_A_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffffef)) | ((!!(Y)) << 4))

/* Dirty */
#define OR1K_SPR_DMMU_DTLBW_TR_D_OFFSET 5
#define OR1K_SPR_DMMU_DTLBW_TR_D_MASK   0x00000020
#define OR1K_SPR_DMMU_DTLBW_TR_D_GET(X) (((X) >> 5) & 0x1)
#define OR1K_SPR_DMMU_DTLBW_TR_D_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffffdf)) | ((!!(Y)) << 5))

/* User Read Enable */
#define OR1K_SPR_DMMU_DTLBW_TR_URE_OFFSET 6
#define OR1K_SPR_DMMU_DTLBW_TR_URE_MASK   0x00000040
#define OR1K_SPR_DMMU_DTLBW_TR_URE_GET(X) (((X) >> 6) & 0x1)
#define OR1K_SPR_DMMU_DTLBW_TR_URE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffffbf)) | ((!!(Y)) << 6))

/* User Write Enable */
#define OR1K_SPR_DMMU_DTLBW_TR_UWE_OFFSET 7
#define OR1K_SPR_DMMU_DTLBW_TR_UWE_MASK   0x00000080
#define OR1K_SPR_DMMU_DTLBW_TR_UWE_GET(X) (((X) >> 7) & 0x1)
#define OR1K_SPR_DMMU_DTLBW_TR_UWE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffff7f)) | ((!!(Y)) << 7))

/* Supervisor Read Enable */
#define OR1K_SPR_DMMU_DTLBW_TR_SRE_OFFSET 8
#define OR1K_SPR_DMMU_DTLBW_TR_SRE_MASK   0x00000100
#define OR1K_SPR_DMMU_DTLBW_TR_SRE_GET(X) (((X) >> 8) & 0x1)
#define OR1K_SPR_DMMU_DTLBW_TR_SRE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffeff)) | ((!!(Y)) << 8))

/* Supervisor Write Enable */
#define OR1K_SPR_DMMU_DTLBW_TR_SWE_OFFSET 9
#define OR1K_SPR_DMMU_DTLBW_TR_SWE_MASK   0x00000200
#define OR1K_SPR_DMMU_DTLBW_TR_SWE_GET(X) (((X) >> 9) & 0x1)
#define OR1K_SPR_DMMU_DTLBW_TR_SWE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffdff)) | ((!!(Y)) << 9))

/* Physical Page Number */
#define OR1K_SPR_DMMU_DTLBW_TR_PPN_LSB    13
#define OR1K_SPR_DMMU_DTLBW_TR_PPN_MSB    31
#define OR1K_SPR_DMMU_DTLBW_TR_PPN_BITS   19
#define OR1K_SPR_DMMU_DTLBW_TR_PPN_MASK   OR1K_UNSIGNED(0xffffe000)
#define OR1K_SPR_DMMU_DTLBW_TR_PPN_GET(X) (((X) >> 13) & OR1K_UNSIGNED(0x0007ffff))
#define OR1K_SPR_DMMU_DTLBW_TR_PPN_SET(X, Y) (((X) & OR1K_UNSIGNED(0x00001fff)) | ((Y) << 13))


/*************************/
/* Instruction MMU Group */
/*************************/
#define OR1K_SPR_IMMU_GROUP 0x02

/* Instruction MMU Control Register */
#define OR1K_SPR_IMMU_IMMUCR_INDEX OR1K_UNSIGNED(0x000)
#define OR1K_SPR_IMMU_IMMUCR_ADDR  OR1K_UNSIGNED(0x1000)

/* ITLB Flush */
#define OR1K_SPR_IMMU_IMMUCR_ITF_OFFSET 0
#define OR1K_SPR_IMMU_IMMUCR_ITF_MASK   0x00000001
#define OR1K_SPR_IMMU_IMMUCR_ITF_GET(X) (((X) >> 0) & 0x1)
#define OR1K_SPR_IMMU_IMMUCR_ITF_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffffe)) | ((!!(Y)) << 0))

/* Page Table Base Pointer */
#define OR1K_SPR_IMMU_IMMUCR_PTBP_LSB    10
#define OR1K_SPR_IMMU_IMMUCR_PTBP_MSB    31
#define OR1K_SPR_IMMU_IMMUCR_PTBP_BITS   22
#define OR1K_SPR_IMMU_IMMUCR_PTBP_MASK   OR1K_UNSIGNED(0xfffffc00)
#define OR1K_SPR_IMMU_IMMUCR_PTBP_GET(X) (((X) >> 10) & OR1K_UNSIGNED(0x003fffff))
#define OR1K_SPR_IMMU_IMMUCR_PTBP_SET(X, Y) (((X) & OR1K_UNSIGNED(0x000003ff)) | ((Y) << 10))


/* Instruction MMU Protection Register */
#define OR1K_SPR_IMMU_IMMUPR_INDEX OR1K_UNSIGNED(0x001)
#define OR1K_SPR_IMMU_IMMUPR_ADDR  OR1K_UNSIGNED(0x1001)

/* Supervisor Execute Enable 1 */
#define OR1K_SPR_IMMU_IMMUPR_SXE1_OFFSET 0
#define OR1K_SPR_IMMU_IMMUPR_SXE1_MASK   0x00000001
#define OR1K_SPR_IMMU_IMMUPR_SXE1_GET(X) (((X) >> 0) & 0x1)
#define OR1K_SPR_IMMU_IMMUPR_SXE1_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffffe)) | ((!!(Y)) << 0))

/* User Execute Enable 1 */
#define OR1K_SPR_IMMU_IMMUPR_UXE1_OFFSET 1
#define OR1K_SPR_IMMU_IMMUPR_UXE1_MASK   0x00000002
#define OR1K_SPR_IMMU_IMMUPR_UXE1_GET(X) (((X) >> 1) & 0x1)
#define OR1K_SPR_IMMU_IMMUPR_UXE1_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffffd)) | ((!!(Y)) << 1))

/* Supervisor Execute Enable 2 */
#define OR1K_SPR_IMMU_IMMUPR_SXE2_OFFSET 2
#define OR1K_SPR_IMMU_IMMUPR_SXE2_MASK   0x00000004
#define OR1K_SPR_IMMU_IMMUPR_SXE2_GET(X) (((X) >> 2) & 0x1)
#define OR1K_SPR_IMMU_IMMUPR_SXE2_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffffb)) | ((!!(Y)) << 2))

/* User Execute Enable 2 */
#define OR1K_SPR_IMMU_IMMUPR_UXE2_OFFSET 3
#define OR1K_SPR_IMMU_IMMUPR_UXE2_MASK   0x00000008
#define OR1K_SPR_IMMU_IMMUPR_UXE2_GET(X) (((X) >> 3) & 0x1)
#define OR1K_SPR_IMMU_IMMUPR_UXE2_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffff7)) | ((!!(Y)) << 3))

/* Supervisor Execute Enable 3 */
#define OR1K_SPR_IMMU_IMMUPR_SXE3_OFFSET 4
#define OR1K_SPR_IMMU_IMMUPR_SXE3_MASK   0x00000010
#define OR1K_SPR_IMMU_IMMUPR_SXE3_GET(X) (((X) >> 4) & 0x1)
#define OR1K_SPR_IMMU_IMMUPR_SXE3_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffffef)) | ((!!(Y)) << 4))

/* User Execute Enable 3 */
#define OR1K_SPR_IMMU_IMMUPR_UXE3_OFFSET 5
#define OR1K_SPR_IMMU_IMMUPR_UXE3_MASK   0x00000020
#define OR1K_SPR_IMMU_IMMUPR_UXE3_GET(X) (((X) >> 5) & 0x1)
#define OR1K_SPR_IMMU_IMMUPR_UXE3_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffffdf)) | ((!!(Y)) << 5))

/* Supervisor Execute Enable 4 */
#define OR1K_SPR_IMMU_IMMUPR_SXE4_OFFSET 6
#define OR1K_SPR_IMMU_IMMUPR_SXE4_MASK   0x00000040
#define OR1K_SPR_IMMU_IMMUPR_SXE4_GET(X) (((X) >> 6) & 0x1)
#define OR1K_SPR_IMMU_IMMUPR_SXE4_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffffbf)) | ((!!(Y)) << 6))

/* User Execute Enable 4 */
#define OR1K_SPR_IMMU_IMMUPR_UXE4_OFFSET 7
#define OR1K_SPR_IMMU_IMMUPR_UXE4_MASK   0x00000080
#define OR1K_SPR_IMMU_IMMUPR_UXE4_GET(X) (((X) >> 7) & 0x1)
#define OR1K_SPR_IMMU_IMMUPR_UXE4_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffff7f)) | ((!!(Y)) << 7))

/* Supervisor Execute Enable 5 */
#define OR1K_SPR_IMMU_IMMUPR_SXE5_OFFSET 8
#define OR1K_SPR_IMMU_IMMUPR_SXE5_MASK   0x00000100
#define OR1K_SPR_IMMU_IMMUPR_SXE5_GET(X) (((X) >> 8) & 0x1)
#define OR1K_SPR_IMMU_IMMUPR_SXE5_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffeff)) | ((!!(Y)) << 8))

/* User Execute Enable 5 */
#define OR1K_SPR_IMMU_IMMUPR_UXE5_OFFSET 9
#define OR1K_SPR_IMMU_IMMUPR_UXE5_MASK   0x00000200
#define OR1K_SPR_IMMU_IMMUPR_UXE5_GET(X) (((X) >> 9) & 0x1)
#define OR1K_SPR_IMMU_IMMUPR_UXE5_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffdff)) | ((!!(Y)) << 9))

/* Supervisor Execute Enable 6 */
#define OR1K_SPR_IMMU_IMMUPR_SXE6_OFFSET 10
#define OR1K_SPR_IMMU_IMMUPR_SXE6_MASK   0x00000400
#define OR1K_SPR_IMMU_IMMUPR_SXE6_GET(X) (((X) >> 10) & 0x1)
#define OR1K_SPR_IMMU_IMMUPR_SXE6_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffbff)) | ((!!(Y)) << 10))

/* User Execute Enable 6 */
#define OR1K_SPR_IMMU_IMMUPR_UXE6_OFFSET 11
#define OR1K_SPR_IMMU_IMMUPR_UXE6_MASK   0x00000800
#define OR1K_SPR_IMMU_IMMUPR_UXE6_GET(X) (((X) >> 11) & 0x1)
#define OR1K_SPR_IMMU_IMMUPR_UXE6_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffff7ff)) | ((!!(Y)) << 11))

/* Supervisor Execute Enable 7 */
#define OR1K_SPR_IMMU_IMMUPR_SXE7_OFFSET 12
#define OR1K_SPR_IMMU_IMMUPR_SXE7_MASK   0x00001000
#define OR1K_SPR_IMMU_IMMUPR_SXE7_GET(X) (((X) >> 12) & 0x1)
#define OR1K_SPR_IMMU_IMMUPR_SXE7_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffefff)) | ((!!(Y)) << 12))

/* User Execute Enable 7 */
#define OR1K_SPR_IMMU_IMMUPR_UXE7_OFFSET 13
#define OR1K_SPR_IMMU_IMMUPR_UXE7_MASK   0x00002000
#define OR1K_SPR_IMMU_IMMUPR_UXE7_GET(X) (((X) >> 13) & 0x1)
#define OR1K_SPR_IMMU_IMMUPR_UXE7_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffdfff)) | ((!!(Y)) << 13))


/* Instruction TLB Entry Invalidate Register */
#define OR1K_SPR_IMMU_ITLBEIR_INDEX OR1K_UNSIGNED(0x002)
#define OR1K_SPR_IMMU_ITLBEIR_ADDR  OR1K_UNSIGNED(0x1002)


/* Instruction ATB Match Registers */
#define OR1K_SPR_IMMU_IATBMR_BASE     OR1K_UNSIGNED(0x004)
#define OR1K_SPR_IMMU_IATBMR_COUNT    OR1K_UNSIGNED(0x004)
#define OR1K_SPR_IMMU_IATBMR_STEP     OR1K_UNSIGNED(0x001)
#define OR1K_SPR_IMMU_IATBMR_INDEX(N) (OR1K_SPR_IMMU_IATBMR_BASE + ((N) * OR1K_SPR_IMMU_IATBMR_STEP))
#define OR1K_SPR_IMMU_IATBMR_ADDR(N)  ((OR1K_SPR_IMMU_GROUP << OR1K_SPR_GROUP_LSB) | OR1K_SPR_IMMU_IATBMR_INDEX(N))

/* Valid */
#define OR1K_SPR_IMMU_IATBMR_V_OFFSET 0
#define OR1K_SPR_IMMU_IATBMR_V_MASK   0x00000001
#define OR1K_SPR_IMMU_IATBMR_V_GET(X) (((X) >> 0) & 0x1)
#define OR1K_SPR_IMMU_IATBMR_V_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffffe)) | ((!!(Y)) << 0))

/* Context ID */
#define OR1K_SPR_IMMU_IATBMR_CID_LSB    1
#define OR1K_SPR_IMMU_IATBMR_CID_MSB    4
#define OR1K_SPR_IMMU_IATBMR_CID_BITS   4
#define OR1K_SPR_IMMU_IATBMR_CID_MASK   OR1K_UNSIGNED(0x0000001e)
#define OR1K_SPR_IMMU_IATBMR_CID_GET(X) (((X) >> 1) & OR1K_UNSIGNED(0x0000000f))
#define OR1K_SPR_IMMU_IATBMR_CID_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffffe1)) | ((Y) << 1))

/* Page Size */
#define OR1K_SPR_IMMU_IATBMR_PS_OFFSET 5
#define OR1K_SPR_IMMU_IATBMR_PS_MASK   0x00000020
#define OR1K_SPR_IMMU_IATBMR_PS_GET(X) (((X) >> 5) & 0x1)
#define OR1K_SPR_IMMU_IATBMR_PS_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffffdf)) | ((!!(Y)) << 5))

/* Virtual Page Number */
#define OR1K_SPR_IMMU_IATBMR_VPN_LSB    10
#define OR1K_SPR_IMMU_IATBMR_VPN_MSB    31
#define OR1K_SPR_IMMU_IATBMR_VPN_BITS   22
#define OR1K_SPR_IMMU_IATBMR_VPN_MASK   OR1K_UNSIGNED(0xfffffc00)
#define OR1K_SPR_IMMU_IATBMR_VPN_GET(X) (((X) >> 10) & OR1K_UNSIGNED(0x003fffff))
#define OR1K_SPR_IMMU_IATBMR_VPN_SET(X, Y) (((X) & OR1K_UNSIGNED(0x000003ff)) | ((Y) << 10))


/* Instruction ATB Translate Registers */
#define OR1K_SPR_IMMU_IATBTR_BASE     OR1K_UNSIGNED(0x008)
#define OR1K_SPR_IMMU_IATBTR_COUNT    OR1K_UNSIGNED(0x004)
#define OR1K_SPR_IMMU_IATBTR_STEP     OR1K_UNSIGNED(0x001)
#define OR1K_SPR_IMMU_IATBTR_INDEX(N) (OR1K_SPR_IMMU_IATBTR_BASE + ((N) * OR1K_SPR_IMMU_IATBTR_STEP))
#define OR1K_SPR_IMMU_IATBTR_ADDR(N)  ((OR1K_SPR_IMMU_GROUP << OR1K_SPR_GROUP_LSB) | OR1K_SPR_IMMU_IATBTR_INDEX(N))

/* Cache Coherency */
#define OR1K_SPR_IMMU_IATBTR_CC_OFFSET 0
#define OR1K_SPR_IMMU_IATBTR_CC_MASK   0x00000001
#define OR1K_SPR_IMMU_IATBTR_CC_GET(X) (((X) >> 0) & 0x1)
#define OR1K_SPR_IMMU_IATBTR_CC_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffffe)) | ((!!(Y)) << 0))

/* Cache Inhibit */
#define OR1K_SPR_IMMU_IATBTR_CI_OFFSET 1
#define OR1K_SPR_IMMU_IATBTR_CI_MASK   0x00000002
#define OR1K_SPR_IMMU_IATBTR_CI_GET(X) (((X) >> 1) & 0x1)
#define OR1K_SPR_IMMU_IATBTR_CI_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffffd)) | ((!!(Y)) << 1))

/* Write-back Cache */
#define OR1K_SPR_IMMU_IATBTR_WBC_OFFSET 2
#define OR1K_SPR_IMMU_IATBTR_WBC_MASK   0x00000004
#define OR1K_SPR_IMMU_IATBTR_WBC_GET(X) (((X) >> 2) & 0x1)
#define OR1K_SPR_IMMU_IATBTR_WBC_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffffb)) | ((!!(Y)) << 2))

/* Weakly-ordered Memory */
#define OR1K_SPR_IMMU_IATBTR_WOM_OFFSET 3
#define OR1K_SPR_IMMU_IATBTR_WOM_MASK   0x00000008
#define OR1K_SPR_IMMU_IATBTR_WOM_GET(X) (((X) >> 3) & 0x1)
#define OR1K_SPR_IMMU_IATBTR_WOM_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffff7)) | ((!!(Y)) << 3))

/* Accessed */
#define OR1K_SPR_IMMU_IATBTR_A_OFFSET 4
#define OR1K_SPR_IMMU_IATBTR_A_MASK   0x00000010
#define OR1K_SPR_IMMU_IATBTR_A_GET(X) (((X) >> 4) & 0x1)
#define OR1K_SPR_IMMU_IATBTR_A_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffffef)) | ((!!(Y)) << 4))

/* Dirty */
#define OR1K_SPR_IMMU_IATBTR_D_OFFSET 5
#define OR1K_SPR_IMMU_IATBTR_D_MASK   0x00000020
#define OR1K_SPR_IMMU_IATBTR_D_GET(X) (((X) >> 5) & 0x1)
#define OR1K_SPR_IMMU_IATBTR_D_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffffdf)) | ((!!(Y)) << 5))

/* Supervisor Execute Enable */
#define OR1K_SPR_IMMU_IATBTR_SRE_OFFSET 6
#define OR1K_SPR_IMMU_IATBTR_SRE_MASK   0x00000040
#define OR1K_SPR_IMMU_IATBTR_SRE_GET(X) (((X) >> 6) & 0x1)
#define OR1K_SPR_IMMU_IATBTR_SRE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffffbf)) | ((!!(Y)) << 6))

/* User Execute Enable */
#define OR1K_SPR_IMMU_IATBTR_URE_OFFSET 7
#define OR1K_SPR_IMMU_IATBTR_URE_MASK   0x00000080
#define OR1K_SPR_IMMU_IATBTR_URE_GET(X) (((X) >> 7) & 0x1)
#define OR1K_SPR_IMMU_IATBTR_URE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffff7f)) | ((!!(Y)) << 7))

/* Physical Page Number */
#define OR1K_SPR_IMMU_IATBTR_PPN_LSB    10
#define OR1K_SPR_IMMU_IATBTR_PPN_MSB    31
#define OR1K_SPR_IMMU_IATBTR_PPN_BITS   22
#define OR1K_SPR_IMMU_IATBTR_PPN_MASK   OR1K_UNSIGNED(0xfffffc00)
#define OR1K_SPR_IMMU_IATBTR_PPN_GET(X) (((X) >> 10) & OR1K_UNSIGNED(0x003fffff))
#define OR1K_SPR_IMMU_IATBTR_PPN_SET(X, Y) (((X) & OR1K_UNSIGNED(0x000003ff)) | ((Y) << 10))


/* Instruction TLB */
#define OR1K_SPR_IMMU_ITLBW_BASE  OR1K_UNSIGNED(0x200)
#define OR1K_SPR_IMMU_ITLBW_COUNT OR1K_UNSIGNED(0x004)
#define OR1K_SPR_IMMU_ITLBW_STEP  OR1K_UNSIGNED(0x100)
#define OR1K_SPR_IMMU_ITLBW_SUBBASE(N0) (OR1K_SPR_IMMU_ITLBW_BASE + ((N0)*OR1K_SPR_IMMU_ITLBW_STEP))

/* Instruction TLB Match Registers */
#define OR1K_SPR_IMMU_ITLBW_MR_BASE  OR1K_UNSIGNED(0x000)
#define OR1K_SPR_IMMU_ITLBW_MR_COUNT OR1K_UNSIGNED(0x080)
#define OR1K_SPR_IMMU_ITLBW_MR_STEP  OR1K_UNSIGNED(0x001)

#define OR1K_SPR_IMMU_ITLBW_MR_INDEX(N0, N1) (OR1K_SPR_IMMU_ITLBW_SUBBASE(N0) + OR1K_SPR_IMMU_ITLBW_MR_BASE + ((N1) * OR1K_SPR_IMMU_ITLBW_MR_STEP))
#define OR1K_SPR_IMMU_ITLBW_MR_ADDR(N0, N1)  ((OR1K_SPR_IMMU_GROUP << OR1K_SPR_GROUP_LSB) | OR1K_SPR_IMMU_ITLBW_MR_INDEX(N0, N1))

/* Valid */
#define OR1K_SPR_IMMU_ITLBW_MR_V_OFFSET 0
#define OR1K_SPR_IMMU_ITLBW_MR_V_MASK   0x00000001
#define OR1K_SPR_IMMU_ITLBW_MR_V_GET(X) (((X) >> 0) & 0x1)
#define OR1K_SPR_IMMU_ITLBW_MR_V_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffffe)) | ((!!(Y)) << 0))

/* Page Level 1 */
#define OR1K_SPR_IMMU_ITLBW_MR_PL1_OFFSET 1
#define OR1K_SPR_IMMU_ITLBW_MR_PL1_MASK   0x00000002
#define OR1K_SPR_IMMU_ITLBW_MR_PL1_GET(X) (((X) >> 1) & 0x1)
#define OR1K_SPR_IMMU_ITLBW_MR_PL1_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffffd)) | ((!!(Y)) << 1))

/* Context ID */
#define OR1K_SPR_IMMU_ITLBW_MR_CID_LSB    2
#define OR1K_SPR_IMMU_ITLBW_MR_CID_MSB    5
#define OR1K_SPR_IMMU_ITLBW_MR_CID_BITS   4
#define OR1K_SPR_IMMU_ITLBW_MR_CID_MASK   OR1K_UNSIGNED(0x0000003c)
#define OR1K_SPR_IMMU_ITLBW_MR_CID_GET(X) (((X) >> 2) & OR1K_UNSIGNED(0x0000000f))
#define OR1K_SPR_IMMU_ITLBW_MR_CID_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffffc3)) | ((Y) << 2))

/* Least Recently Used */
#define OR1K_SPR_IMMU_ITLBW_MR_LRU_LSB    6
#define OR1K_SPR_IMMU_ITLBW_MR_LRU_MSB    7
#define OR1K_SPR_IMMU_ITLBW_MR_LRU_BITS   2
#define OR1K_SPR_IMMU_ITLBW_MR_LRU_MASK   OR1K_UNSIGNED(0x000000c0)
#define OR1K_SPR_IMMU_ITLBW_MR_LRU_GET(X) (((X) >> 6) & OR1K_UNSIGNED(0x00000003))
#define OR1K_SPR_IMMU_ITLBW_MR_LRU_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffff3f)) | ((Y) << 6))

/* Virtual Page Number */
#define OR1K_SPR_IMMU_ITLBW_MR_VPN_LSB    13
#define OR1K_SPR_IMMU_ITLBW_MR_VPN_MSB    31
#define OR1K_SPR_IMMU_ITLBW_MR_VPN_BITS   19
#define OR1K_SPR_IMMU_ITLBW_MR_VPN_MASK   OR1K_UNSIGNED(0xffffe000)
#define OR1K_SPR_IMMU_ITLBW_MR_VPN_GET(X) (((X) >> 13) & OR1K_UNSIGNED(0x0007ffff))
#define OR1K_SPR_IMMU_ITLBW_MR_VPN_SET(X, Y) (((X) & OR1K_UNSIGNED(0x00001fff)) | ((Y) << 13))

/* Instruction TLB Translate Registers */
#define OR1K_SPR_IMMU_ITLBW_TR_BASE  OR1K_UNSIGNED(0x080)
#define OR1K_SPR_IMMU_ITLBW_TR_COUNT OR1K_UNSIGNED(0x080)
#define OR1K_SPR_IMMU_ITLBW_TR_STEP  OR1K_UNSIGNED(0x001)

#define OR1K_SPR_IMMU_ITLBW_TR_INDEX(N0, N1) (OR1K_SPR_IMMU_ITLBW_SUBBASE(N0) + OR1K_SPR_IMMU_ITLBW_TR_BASE + ((N1) * OR1K_SPR_IMMU_ITLBW_TR_STEP))
#define OR1K_SPR_IMMU_ITLBW_TR_ADDR(N0, N1)  ((OR1K_SPR_IMMU_GROUP << OR1K_SPR_GROUP_LSB) | OR1K_SPR_IMMU_ITLBW_TR_INDEX(N0, N1))

/* Cache Coherency */
#define OR1K_SPR_IMMU_ITLBW_TR_CC_OFFSET 0
#define OR1K_SPR_IMMU_ITLBW_TR_CC_MASK   0x00000001
#define OR1K_SPR_IMMU_ITLBW_TR_CC_GET(X) (((X) >> 0) & 0x1)
#define OR1K_SPR_IMMU_ITLBW_TR_CC_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffffe)) | ((!!(Y)) << 0))

/* Cache Inhibit */
#define OR1K_SPR_IMMU_ITLBW_TR_CI_OFFSET 1
#define OR1K_SPR_IMMU_ITLBW_TR_CI_MASK   0x00000002
#define OR1K_SPR_IMMU_ITLBW_TR_CI_GET(X) (((X) >> 1) & 0x1)
#define OR1K_SPR_IMMU_ITLBW_TR_CI_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffffd)) | ((!!(Y)) << 1))

/* Write-back Cache */
#define OR1K_SPR_IMMU_ITLBW_TR_WBC_OFFSET 2
#define OR1K_SPR_IMMU_ITLBW_TR_WBC_MASK   0x00000004
#define OR1K_SPR_IMMU_ITLBW_TR_WBC_GET(X) (((X) >> 2) & 0x1)
#define OR1K_SPR_IMMU_ITLBW_TR_WBC_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffffb)) | ((!!(Y)) << 2))

/* Weakly-ordered Memory */
#define OR1K_SPR_IMMU_ITLBW_TR_WOM_OFFSET 3
#define OR1K_SPR_IMMU_ITLBW_TR_WOM_MASK   0x00000008
#define OR1K_SPR_IMMU_ITLBW_TR_WOM_GET(X) (((X) >> 3) & 0x1)
#define OR1K_SPR_IMMU_ITLBW_TR_WOM_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffff7)) | ((!!(Y)) << 3))

/* Accessed */
#define OR1K_SPR_IMMU_ITLBW_TR_A_OFFSET 4
#define OR1K_SPR_IMMU_ITLBW_TR_A_MASK   0x00000010
#define OR1K_SPR_IMMU_ITLBW_TR_A_GET(X) (((X) >> 4) & 0x1)
#define OR1K_SPR_IMMU_ITLBW_TR_A_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffffef)) | ((!!(Y)) << 4))

/* Dirty */
#define OR1K_SPR_IMMU_ITLBW_TR_D_OFFSET 5
#define OR1K_SPR_IMMU_ITLBW_TR_D_MASK   0x00000020
#define OR1K_SPR_IMMU_ITLBW_TR_D_GET(X) (((X) >> 5) & 0x1)
#define OR1K_SPR_IMMU_ITLBW_TR_D_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffffdf)) | ((!!(Y)) << 5))

/* User Execute Enable */
#define OR1K_SPR_IMMU_ITLBW_TR_UXE_OFFSET 6
#define OR1K_SPR_IMMU_ITLBW_TR_UXE_MASK   0x00000040
#define OR1K_SPR_IMMU_ITLBW_TR_UXE_GET(X) (((X) >> 6) & 0x1)
#define OR1K_SPR_IMMU_ITLBW_TR_UXE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffffbf)) | ((!!(Y)) << 6))

/* Supervisor Execute Enable */
#define OR1K_SPR_IMMU_ITLBW_TR_SXE_OFFSET 7
#define OR1K_SPR_IMMU_ITLBW_TR_SXE_MASK   0x00000080
#define OR1K_SPR_IMMU_ITLBW_TR_SXE_GET(X) (((X) >> 7) & 0x1)
#define OR1K_SPR_IMMU_ITLBW_TR_SXE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffff7f)) | ((!!(Y)) << 7))

/* Physical Page Number */
#define OR1K_SPR_IMMU_ITLBW_TR_PPN_LSB    13
#define OR1K_SPR_IMMU_ITLBW_TR_PPN_MSB    31
#define OR1K_SPR_IMMU_ITLBW_TR_PPN_BITS   19
#define OR1K_SPR_IMMU_ITLBW_TR_PPN_MASK   OR1K_UNSIGNED(0xffffe000)
#define OR1K_SPR_IMMU_ITLBW_TR_PPN_GET(X) (((X) >> 13) & OR1K_UNSIGNED(0x0007ffff))
#define OR1K_SPR_IMMU_ITLBW_TR_PPN_SET(X, Y) (((X) & OR1K_UNSIGNED(0x00001fff)) | ((Y) << 13))


/********************/
/* Data Cache Group */
/********************/
#define OR1K_SPR_DCACHE_GROUP 0x03

/* Data Cache Control Register */
#define OR1K_SPR_DCACHE_DCCR_INDEX OR1K_UNSIGNED(0x000)
#define OR1K_SPR_DCACHE_DCCR_ADDR  OR1K_UNSIGNED(0x1800)

/* Enable Ways */
#define OR1K_SPR_DCACHE_DCCR_EW_LSB    0
#define OR1K_SPR_DCACHE_DCCR_EW_MSB    7
#define OR1K_SPR_DCACHE_DCCR_EW_BITS   8
#define OR1K_SPR_DCACHE_DCCR_EW_MASK   OR1K_UNSIGNED(0x000000ff)
#define OR1K_SPR_DCACHE_DCCR_EW_GET(X) (((X) >> 0) & OR1K_UNSIGNED(0x000000ff))
#define OR1K_SPR_DCACHE_DCCR_EW_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffff00)) | ((Y) << 0))


/* Data Cache Block Prefetch Register */
#define OR1K_SPR_DCACHE_DCBPR_INDEX OR1K_UNSIGNED(0x001)
#define OR1K_SPR_DCACHE_DCBPR_ADDR  OR1K_UNSIGNED(0x1801)


/* Data Cache Block Flush Register */
#define OR1K_SPR_DCACHE_DCBFR_INDEX OR1K_UNSIGNED(0x002)
#define OR1K_SPR_DCACHE_DCBFR_ADDR  OR1K_UNSIGNED(0x1802)


/* Data Cache Block Invalidate Register */
#define OR1K_SPR_DCACHE_DCBIR_INDEX OR1K_UNSIGNED(0x003)
#define OR1K_SPR_DCACHE_DCBIR_ADDR  OR1K_UNSIGNED(0x1803)


/* Data Cache Block Write-back Register */
#define OR1K_SPR_DCACHE_DCBWR_INDEX OR1K_UNSIGNED(0x004)
#define OR1K_SPR_DCACHE_DCBWR_ADDR  OR1K_UNSIGNED(0x1804)


/* Data Cache Block Lock Register */
#define OR1K_SPR_DCACHE_DCBLR_INDEX OR1K_UNSIGNED(0x005)
#define OR1K_SPR_DCACHE_DCBLR_ADDR  OR1K_UNSIGNED(0x1805)


/***************************/
/* Instruction Cache Group */
/***************************/
#define OR1K_SPR_ICACHE_GROUP 0x04

/* Instruction Cache Control Register */
#define OR1K_SPR_ICACHE_ICCR_INDEX OR1K_UNSIGNED(0x000)
#define OR1K_SPR_ICACHE_ICCR_ADDR  OR1K_UNSIGNED(0x2000)

/* Enable Ways */
#define OR1K_SPR_ICACHE_ICCR_EW_LSB    0
#define OR1K_SPR_ICACHE_ICCR_EW_MSB    7
#define OR1K_SPR_ICACHE_ICCR_EW_BITS   8
#define OR1K_SPR_ICACHE_ICCR_EW_MASK   OR1K_UNSIGNED(0x000000ff)
#define OR1K_SPR_ICACHE_ICCR_EW_GET(X) (((X) >> 0) & OR1K_UNSIGNED(0x000000ff))
#define OR1K_SPR_ICACHE_ICCR_EW_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffff00)) | ((Y) << 0))


/* Instruction Cache Block Prefetch Register */
#define OR1K_SPR_ICACHE_ICBPR_INDEX OR1K_UNSIGNED(0x001)
#define OR1K_SPR_ICACHE_ICBPR_ADDR  OR1K_UNSIGNED(0x2001)


/* Instruction Cache Block Invalidate Register */
#define OR1K_SPR_ICACHE_ICBIR_INDEX OR1K_UNSIGNED(0x002)
#define OR1K_SPR_ICACHE_ICBIR_ADDR  OR1K_UNSIGNED(0x2002)


/* Instruction Cache Block Lock Register */
#define OR1K_SPR_ICACHE_ICBLR_INDEX OR1K_UNSIGNED(0x003)
#define OR1K_SPR_ICACHE_ICBLR_ADDR  OR1K_UNSIGNED(0x2003)


/*********************************/
/* Multiply and Accumulate Group */
/*********************************/
#define OR1K_SPR_MAC_GROUP 0x05

/* MAC Result Low Word */
#define OR1K_SPR_MAC_MACLO_INDEX OR1K_UNSIGNED(0x001)
#define OR1K_SPR_MAC_MACLO_ADDR  OR1K_UNSIGNED(0x2801)


/* MAC Result High Word */
#define OR1K_SPR_MAC_MACHI_INDEX OR1K_UNSIGNED(0x002)
#define OR1K_SPR_MAC_MACHI_ADDR  OR1K_UNSIGNED(0x2802)


/***************/
/* Debug Group */
/***************/
#define OR1K_SPR_DEBUG_GROUP 0x06

/* Debug Value Registers */
#define OR1K_SPR_DEBUG_DVR_BASE     OR1K_UNSIGNED(0x000)
#define OR1K_SPR_DEBUG_DVR_COUNT    OR1K_UNSIGNED(0x008)
#define OR1K_SPR_DEBUG_DVR_STEP     OR1K_UNSIGNED(0x001)
#define OR1K_SPR_DEBUG_DVR_INDEX(N) (OR1K_SPR_DEBUG_DVR_BASE + ((N) * OR1K_SPR_DEBUG_DVR_STEP))
#define OR1K_SPR_DEBUG_DVR_ADDR(N)  ((OR1K_SPR_DEBUG_GROUP << OR1K_SPR_GROUP_LSB) | OR1K_SPR_DEBUG_DVR_INDEX(N))


/* Debug Control Registers */
#define OR1K_SPR_DEBUG_DCR_BASE     OR1K_UNSIGNED(0x008)
#define OR1K_SPR_DEBUG_DCR_COUNT    OR1K_UNSIGNED(0x008)
#define OR1K_SPR_DEBUG_DCR_STEP     OR1K_UNSIGNED(0x001)
#define OR1K_SPR_DEBUG_DCR_INDEX(N) (OR1K_SPR_DEBUG_DCR_BASE + ((N) * OR1K_SPR_DEBUG_DCR_STEP))
#define OR1K_SPR_DEBUG_DCR_ADDR(N)  ((OR1K_SPR_DEBUG_GROUP << OR1K_SPR_GROUP_LSB) | OR1K_SPR_DEBUG_DCR_INDEX(N))

/* DVR/DCR Present */
#define OR1K_SPR_DEBUG_DCR_DP_OFFSET 0
#define OR1K_SPR_DEBUG_DCR_DP_MASK   0x00000001
#define OR1K_SPR_DEBUG_DCR_DP_GET(X) (((X) >> 0) & 0x1)
#define OR1K_SPR_DEBUG_DCR_DP_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffffe)) | ((!!(Y)) << 0))

/* Compare Condition */
#define OR1K_SPR_DEBUG_DCR_CC_LSB    1
#define OR1K_SPR_DEBUG_DCR_CC_MSB    3
#define OR1K_SPR_DEBUG_DCR_CC_BITS   3
#define OR1K_SPR_DEBUG_DCR_CC_MASK   OR1K_UNSIGNED(0x0000000e)
#define OR1K_SPR_DEBUG_DCR_CC_GET(X) (((X) >> 1) & OR1K_UNSIGNED(0x00000007))
#define OR1K_SPR_DEBUG_DCR_CC_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffff1)) | ((Y) << 1))

/* Masked */
#define OR1K_SPR_DEBUG_DCR_CC_MASKED 0
/* Equal */
#define OR1K_SPR_DEBUG_DCR_CC_EQ 1
/* Less than */
#define OR1K_SPR_DEBUG_DCR_CC_LT 2
/* Less than or equal */
#define OR1K_SPR_DEBUG_DCR_CC_LTE 3
/* Greater than */
#define OR1K_SPR_DEBUG_DCR_CC_GT 4
/* Greater than or equal */
#define OR1K_SPR_DEBUG_DCR_CC_GTE 5
/* Not equal */
#define OR1K_SPR_DEBUG_DCR_CC_NEQ 6
/* Signed Comparison */
#define OR1K_SPR_DEBUG_DCR_SC_OFFSET 4
#define OR1K_SPR_DEBUG_DCR_SC_MASK   0x00000010
#define OR1K_SPR_DEBUG_DCR_SC_GET(X) (((X) >> 4) & 0x1)
#define OR1K_SPR_DEBUG_DCR_SC_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffffef)) | ((!!(Y)) << 4))

/* Compare To */
#define OR1K_SPR_DEBUG_DCR_CT_LSB    5
#define OR1K_SPR_DEBUG_DCR_CT_MSB    7
#define OR1K_SPR_DEBUG_DCR_CT_BITS   3
#define OR1K_SPR_DEBUG_DCR_CT_MASK   OR1K_UNSIGNED(0x000000e0)
#define OR1K_SPR_DEBUG_DCR_CT_GET(X) (((X) >> 5) & OR1K_UNSIGNED(0x00000007))
#define OR1K_SPR_DEBUG_DCR_CT_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffff1f)) | ((Y) << 5))

/* Comparison disabled */
#define OR1K_SPR_DEBUG_DCR_CT_DISABLED 0
/* Instruction fetch EA */
#define OR1K_SPR_DEBUG_DCR_CT_FEA 1
/* Load EA */
#define OR1K_SPR_DEBUG_DCR_CT_LEA 2
/* Store EA */
#define OR1K_SPR_DEBUG_DCR_CT_SEA 3
/* Load data */
#define OR1K_SPR_DEBUG_DCR_CT_LD 4
/* Store data */
#define OR1K_SPR_DEBUG_DCR_CT_SD 5
/* Load/store EA */
#define OR1K_SPR_DEBUG_DCR_CT_LSEA 6
/* Load/store data */
#define OR1K_SPR_DEBUG_DCR_CT_LSD 7

/* Debug Mode Register 1 */
#define OR1K_SPR_DEBUG_DMR1_INDEX OR1K_UNSIGNED(0x010)
#define OR1K_SPR_DEBUG_DMR1_ADDR  OR1K_UNSIGNED(0x3010)

/* Chain Watchpoint 0 */
#define OR1K_SPR_DEBUG_DMR1_CW0_LSB    0
#define OR1K_SPR_DEBUG_DMR1_CW0_MSB    1
#define OR1K_SPR_DEBUG_DMR1_CW0_BITS   2
#define OR1K_SPR_DEBUG_DMR1_CW0_MASK   OR1K_UNSIGNED(0x00000003)
#define OR1K_SPR_DEBUG_DMR1_CW0_GET(X) (((X) >> 0) & OR1K_UNSIGNED(0x00000003))
#define OR1K_SPR_DEBUG_DMR1_CW0_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffffc)) | ((Y) << 0))

/* Chain Watchpoint 1 */
#define OR1K_SPR_DEBUG_DMR1_CW1_LSB    2
#define OR1K_SPR_DEBUG_DMR1_CW1_MSB    3
#define OR1K_SPR_DEBUG_DMR1_CW1_BITS   2
#define OR1K_SPR_DEBUG_DMR1_CW1_MASK   OR1K_UNSIGNED(0x0000000c)
#define OR1K_SPR_DEBUG_DMR1_CW1_GET(X) (((X) >> 2) & OR1K_UNSIGNED(0x00000003))
#define OR1K_SPR_DEBUG_DMR1_CW1_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffff3)) | ((Y) << 2))

/* Chain Watchpoint 2 */
#define OR1K_SPR_DEBUG_DMR1_CW2_LSB    4
#define OR1K_SPR_DEBUG_DMR1_CW2_MSB    5
#define OR1K_SPR_DEBUG_DMR1_CW2_BITS   2
#define OR1K_SPR_DEBUG_DMR1_CW2_MASK   OR1K_UNSIGNED(0x00000030)
#define OR1K_SPR_DEBUG_DMR1_CW2_GET(X) (((X) >> 4) & OR1K_UNSIGNED(0x00000003))
#define OR1K_SPR_DEBUG_DMR1_CW2_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffffcf)) | ((Y) << 4))

/* Chain Watchpoint 3 */
#define OR1K_SPR_DEBUG_DMR1_CW3_LSB    6
#define OR1K_SPR_DEBUG_DMR1_CW3_MSB    7
#define OR1K_SPR_DEBUG_DMR1_CW3_BITS   2
#define OR1K_SPR_DEBUG_DMR1_CW3_MASK   OR1K_UNSIGNED(0x000000c0)
#define OR1K_SPR_DEBUG_DMR1_CW3_GET(X) (((X) >> 6) & OR1K_UNSIGNED(0x00000003))
#define OR1K_SPR_DEBUG_DMR1_CW3_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffff3f)) | ((Y) << 6))

/* Chain Watchpoint 4 */
#define OR1K_SPR_DEBUG_DMR1_CW4_LSB    9
#define OR1K_SPR_DEBUG_DMR1_CW4_MSB    9
#define OR1K_SPR_DEBUG_DMR1_CW4_BITS   1
#define OR1K_SPR_DEBUG_DMR1_CW4_MASK   OR1K_UNSIGNED(0x00000200)
#define OR1K_SPR_DEBUG_DMR1_CW4_GET(X) (((X) >> 9) & OR1K_UNSIGNED(0x00000001))
#define OR1K_SPR_DEBUG_DMR1_CW4_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffdff)) | ((Y) << 9))

/* Chain Watchpoint 5 */
#define OR1K_SPR_DEBUG_DMR1_CW5_LSB    10
#define OR1K_SPR_DEBUG_DMR1_CW5_MSB    11
#define OR1K_SPR_DEBUG_DMR1_CW5_BITS   2
#define OR1K_SPR_DEBUG_DMR1_CW5_MASK   OR1K_UNSIGNED(0x00000c00)
#define OR1K_SPR_DEBUG_DMR1_CW5_GET(X) (((X) >> 10) & OR1K_UNSIGNED(0x00000003))
#define OR1K_SPR_DEBUG_DMR1_CW5_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffff3ff)) | ((Y) << 10))

/* Chain Watchpoint 6 */
#define OR1K_SPR_DEBUG_DMR1_CW6_LSB    12
#define OR1K_SPR_DEBUG_DMR1_CW6_MSB    13
#define OR1K_SPR_DEBUG_DMR1_CW6_BITS   2
#define OR1K_SPR_DEBUG_DMR1_CW6_MASK   OR1K_UNSIGNED(0x00003000)
#define OR1K_SPR_DEBUG_DMR1_CW6_GET(X) (((X) >> 12) & OR1K_UNSIGNED(0x00000003))
#define OR1K_SPR_DEBUG_DMR1_CW6_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffcfff)) | ((Y) << 12))

/* Chain Watchpoint 7 */
#define OR1K_SPR_DEBUG_DMR1_CW7_LSB    14
#define OR1K_SPR_DEBUG_DMR1_CW7_MSB    15
#define OR1K_SPR_DEBUG_DMR1_CW7_BITS   2
#define OR1K_SPR_DEBUG_DMR1_CW7_MASK   OR1K_UNSIGNED(0x0000c000)
#define OR1K_SPR_DEBUG_DMR1_CW7_GET(X) (((X) >> 14) & OR1K_UNSIGNED(0x00000003))
#define OR1K_SPR_DEBUG_DMR1_CW7_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffff3fff)) | ((Y) << 14))

/* Chain Watchpoint 8 */
#define OR1K_SPR_DEBUG_DMR1_CW8_LSB    16
#define OR1K_SPR_DEBUG_DMR1_CW8_MSB    17
#define OR1K_SPR_DEBUG_DMR1_CW8_BITS   2
#define OR1K_SPR_DEBUG_DMR1_CW8_MASK   OR1K_UNSIGNED(0x00030000)
#define OR1K_SPR_DEBUG_DMR1_CW8_GET(X) (((X) >> 16) & OR1K_UNSIGNED(0x00000003))
#define OR1K_SPR_DEBUG_DMR1_CW8_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffcffff)) | ((Y) << 16))

/* Chain Watchpoint 9 */
#define OR1K_SPR_DEBUG_DMR1_CW9_LSB    18
#define OR1K_SPR_DEBUG_DMR1_CW9_MSB    19
#define OR1K_SPR_DEBUG_DMR1_CW9_BITS   2
#define OR1K_SPR_DEBUG_DMR1_CW9_MASK   OR1K_UNSIGNED(0x000c0000)
#define OR1K_SPR_DEBUG_DMR1_CW9_GET(X) (((X) >> 18) & OR1K_UNSIGNED(0x00000003))
#define OR1K_SPR_DEBUG_DMR1_CW9_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfff3ffff)) | ((Y) << 18))

/* Single-step Trace */
#define OR1K_SPR_DEBUG_DMR1_ST_OFFSET 22
#define OR1K_SPR_DEBUG_DMR1_ST_MASK   0x00400000
#define OR1K_SPR_DEBUG_DMR1_ST_GET(X) (((X) >> 22) & 0x1)
#define OR1K_SPR_DEBUG_DMR1_ST_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffbfffff)) | ((!!(Y)) << 22))

/* Branch Trace */
#define OR1K_SPR_DEBUG_DMR1_BT_OFFSET 23
#define OR1K_SPR_DEBUG_DMR1_BT_MASK   0x00800000
#define OR1K_SPR_DEBUG_DMR1_BT_GET(X) (((X) >> 23) & 0x1)
#define OR1K_SPR_DEBUG_DMR1_BT_SET(X, Y) (((X) & OR1K_UNSIGNED(0xff7fffff)) | ((!!(Y)) << 23))


/* Debug Mode Register 2 */
#define OR1K_SPR_DEBUG_DMR2_INDEX OR1K_UNSIGNED(0x011)
#define OR1K_SPR_DEBUG_DMR2_ADDR  OR1K_UNSIGNED(0x3011)

/* Watchpoint Counter Enable 0 */
#define OR1K_SPR_DEBUG_DMR2_WCE0_OFFSET 0
#define OR1K_SPR_DEBUG_DMR2_WCE0_MASK   0x00000001
#define OR1K_SPR_DEBUG_DMR2_WCE0_GET(X) (((X) >> 0) & 0x1)
#define OR1K_SPR_DEBUG_DMR2_WCE0_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffffe)) | ((!!(Y)) << 0))

/* Watchpoint Counter Enable 1 */
#define OR1K_SPR_DEBUG_DMR2_WCE1_OFFSET 1
#define OR1K_SPR_DEBUG_DMR2_WCE1_MASK   0x00000002
#define OR1K_SPR_DEBUG_DMR2_WCE1_GET(X) (((X) >> 1) & 0x1)
#define OR1K_SPR_DEBUG_DMR2_WCE1_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffffd)) | ((!!(Y)) << 1))

/* Assign Watchpoints to Counter */
#define OR1K_SPR_DEBUG_DMR2_AWTC_LSB    2
#define OR1K_SPR_DEBUG_DMR2_AWTC_MSB    11
#define OR1K_SPR_DEBUG_DMR2_AWTC_BITS   10
#define OR1K_SPR_DEBUG_DMR2_AWTC_MASK   OR1K_UNSIGNED(0x00000ffc)
#define OR1K_SPR_DEBUG_DMR2_AWTC_GET(X) (((X) >> 2) & OR1K_UNSIGNED(0x000003ff))
#define OR1K_SPR_DEBUG_DMR2_AWTC_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffff003)) | ((Y) << 2))

/* Watchpoints Generating Breakpoint */
#define OR1K_SPR_DEBUG_DMR2_WGB_LSB    12
#define OR1K_SPR_DEBUG_DMR2_WGB_MSB    21
#define OR1K_SPR_DEBUG_DMR2_WGB_BITS   10
#define OR1K_SPR_DEBUG_DMR2_WGB_MASK   OR1K_UNSIGNED(0x003ff000)
#define OR1K_SPR_DEBUG_DMR2_WGB_GET(X) (((X) >> 12) & OR1K_UNSIGNED(0x000003ff))
#define OR1K_SPR_DEBUG_DMR2_WGB_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffc00fff)) | ((Y) << 12))

/* Watchpoints Breakpoint Status */
#define OR1K_SPR_DEBUG_DMR2_WBS_LSB    22
#define OR1K_SPR_DEBUG_DMR2_WBS_MSB    31
#define OR1K_SPR_DEBUG_DMR2_WBS_BITS   10
#define OR1K_SPR_DEBUG_DMR2_WBS_MASK   OR1K_UNSIGNED(0xffc00000)
#define OR1K_SPR_DEBUG_DMR2_WBS_GET(X) (((X) >> 22) & OR1K_UNSIGNED(0x000003ff))
#define OR1K_SPR_DEBUG_DMR2_WBS_SET(X, Y) (((X) & OR1K_UNSIGNED(0x003fffff)) | ((Y) << 22))


/* Debug Watchpoint Counter Registers */
#define OR1K_SPR_DEBUG_DCWR_BASE     OR1K_UNSIGNED(0x012)
#define OR1K_SPR_DEBUG_DCWR_COUNT    OR1K_UNSIGNED(0x002)
#define OR1K_SPR_DEBUG_DCWR_STEP     OR1K_UNSIGNED(0x001)
#define OR1K_SPR_DEBUG_DCWR_INDEX(N) (OR1K_SPR_DEBUG_DCWR_BASE + ((N) * OR1K_SPR_DEBUG_DCWR_STEP))
#define OR1K_SPR_DEBUG_DCWR_ADDR(N)  ((OR1K_SPR_DEBUG_GROUP << OR1K_SPR_GROUP_LSB) | OR1K_SPR_DEBUG_DCWR_INDEX(N))


/* Debug Stop Register */
#define OR1K_SPR_DEBUG_DSR_INDEX OR1K_UNSIGNED(0x014)
#define OR1K_SPR_DEBUG_DSR_ADDR  OR1K_UNSIGNED(0x3014)

/* Reset Exception */
#define OR1K_SPR_DEBUG_DSR_RSTE_OFFSET 0
#define OR1K_SPR_DEBUG_DSR_RSTE_MASK   0x00000001
#define OR1K_SPR_DEBUG_DSR_RSTE_GET(X) (((X) >> 0) & 0x1)
#define OR1K_SPR_DEBUG_DSR_RSTE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffffe)) | ((!!(Y)) << 0))

/* Bus Error Exception */
#define OR1K_SPR_DEBUG_DSR_BUSEE_OFFSET 1
#define OR1K_SPR_DEBUG_DSR_BUSEE_MASK   0x00000002
#define OR1K_SPR_DEBUG_DSR_BUSEE_GET(X) (((X) >> 1) & 0x1)
#define OR1K_SPR_DEBUG_DSR_BUSEE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffffd)) | ((!!(Y)) << 1))

/* Data Page Fault Exception */
#define OR1K_SPR_DEBUG_DSR_DPFE_OFFSET 2
#define OR1K_SPR_DEBUG_DSR_DPFE_MASK   0x00000004
#define OR1K_SPR_DEBUG_DSR_DPFE_GET(X) (((X) >> 2) & 0x1)
#define OR1K_SPR_DEBUG_DSR_DPFE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffffb)) | ((!!(Y)) << 2))

/* Instruction Page Fault Exception */
#define OR1K_SPR_DEBUG_DSR_IPFE_OFFSET 3
#define OR1K_SPR_DEBUG_DSR_IPFE_MASK   0x00000008
#define OR1K_SPR_DEBUG_DSR_IPFE_GET(X) (((X) >> 3) & 0x1)
#define OR1K_SPR_DEBUG_DSR_IPFE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffff7)) | ((!!(Y)) << 3))

/* Tick Timer Exception */
#define OR1K_SPR_DEBUG_DSR_TTE_OFFSET 4
#define OR1K_SPR_DEBUG_DSR_TTE_MASK   0x00000010
#define OR1K_SPR_DEBUG_DSR_TTE_GET(X) (((X) >> 4) & 0x1)
#define OR1K_SPR_DEBUG_DSR_TTE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffffef)) | ((!!(Y)) << 4))

/* Alignment Exception */
#define OR1K_SPR_DEBUG_DSR_AE_OFFSET 5
#define OR1K_SPR_DEBUG_DSR_AE_MASK   0x00000020
#define OR1K_SPR_DEBUG_DSR_AE_GET(X) (((X) >> 5) & 0x1)
#define OR1K_SPR_DEBUG_DSR_AE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffffdf)) | ((!!(Y)) << 5))

/* Illegal Instruction Exception */
#define OR1K_SPR_DEBUG_DSR_IIE_OFFSET 6
#define OR1K_SPR_DEBUG_DSR_IIE_MASK   0x00000040
#define OR1K_SPR_DEBUG_DSR_IIE_GET(X) (((X) >> 6) & 0x1)
#define OR1K_SPR_DEBUG_DSR_IIE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffffbf)) | ((!!(Y)) << 6))

/* Interrupt Exception */
#define OR1K_SPR_DEBUG_DSR_INTE_OFFSET 7
#define OR1K_SPR_DEBUG_DSR_INTE_MASK   0x00000080
#define OR1K_SPR_DEBUG_DSR_INTE_GET(X) (((X) >> 7) & 0x1)
#define OR1K_SPR_DEBUG_DSR_INTE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffff7f)) | ((!!(Y)) << 7))

/* DTLB Miss Exception */
#define OR1K_SPR_DEBUG_DSR_DME_OFFSET 8
#define OR1K_SPR_DEBUG_DSR_DME_MASK   0x00000100
#define OR1K_SPR_DEBUG_DSR_DME_GET(X) (((X) >> 8) & 0x1)
#define OR1K_SPR_DEBUG_DSR_DME_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffeff)) | ((!!(Y)) << 8))

/* ITLB Miss Exception */
#define OR1K_SPR_DEBUG_DSR_IME_OFFSET 9
#define OR1K_SPR_DEBUG_DSR_IME_MASK   0x00000200
#define OR1K_SPR_DEBUG_DSR_IME_GET(X) (((X) >> 9) & 0x1)
#define OR1K_SPR_DEBUG_DSR_IME_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffdff)) | ((!!(Y)) << 9))

/* Range Exception */
#define OR1K_SPR_DEBUG_DSR_RE_OFFSET 10
#define OR1K_SPR_DEBUG_DSR_RE_MASK   0x00000400
#define OR1K_SPR_DEBUG_DSR_RE_GET(X) (((X) >> 10) & 0x1)
#define OR1K_SPR_DEBUG_DSR_RE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffbff)) | ((!!(Y)) << 10))

/* System Call Exception */
#define OR1K_SPR_DEBUG_DSR_SCE_OFFSET 11
#define OR1K_SPR_DEBUG_DSR_SCE_MASK   0x00000800
#define OR1K_SPR_DEBUG_DSR_SCE_GET(X) (((X) >> 11) & 0x1)
#define OR1K_SPR_DEBUG_DSR_SCE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffff7ff)) | ((!!(Y)) << 11))

/* Floating Point Exception */
#define OR1K_SPR_DEBUG_DSR_FPE_OFFSET 12
#define OR1K_SPR_DEBUG_DSR_FPE_MASK   0x00001000
#define OR1K_SPR_DEBUG_DSR_FPE_GET(X) (((X) >> 12) & 0x1)
#define OR1K_SPR_DEBUG_DSR_FPE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffefff)) | ((!!(Y)) << 12))

/* Trap Exception */
#define OR1K_SPR_DEBUG_DSR_TE_OFFSET 13
#define OR1K_SPR_DEBUG_DSR_TE_MASK   0x00002000
#define OR1K_SPR_DEBUG_DSR_TE_GET(X) (((X) >> 13) & 0x1)
#define OR1K_SPR_DEBUG_DSR_TE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffdfff)) | ((!!(Y)) << 13))


/* Debug Reason Register */
#define OR1K_SPR_DEBUG_DRR_INDEX OR1K_UNSIGNED(0x015)
#define OR1K_SPR_DEBUG_DRR_ADDR  OR1K_UNSIGNED(0x3015)

/* Reset Exception */
#define OR1K_SPR_DEBUG_DRR_RSTE_OFFSET 0
#define OR1K_SPR_DEBUG_DRR_RSTE_MASK   0x00000001
#define OR1K_SPR_DEBUG_DRR_RSTE_GET(X) (((X) >> 0) & 0x1)
#define OR1K_SPR_DEBUG_DRR_RSTE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffffe)) | ((!!(Y)) << 0))

/* Bus Error Exception */
#define OR1K_SPR_DEBUG_DRR_BUSEE_OFFSET 1
#define OR1K_SPR_DEBUG_DRR_BUSEE_MASK   0x00000002
#define OR1K_SPR_DEBUG_DRR_BUSEE_GET(X) (((X) >> 1) & 0x1)
#define OR1K_SPR_DEBUG_DRR_BUSEE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffffd)) | ((!!(Y)) << 1))

/* Data Page Fault Exception */
#define OR1K_SPR_DEBUG_DRR_DPFE_OFFSET 2
#define OR1K_SPR_DEBUG_DRR_DPFE_MASK   0x00000004
#define OR1K_SPR_DEBUG_DRR_DPFE_GET(X) (((X) >> 2) & 0x1)
#define OR1K_SPR_DEBUG_DRR_DPFE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffffb)) | ((!!(Y)) << 2))

/* Instruction Page Fault Exception */
#define OR1K_SPR_DEBUG_DRR_IPFE_OFFSET 3
#define OR1K_SPR_DEBUG_DRR_IPFE_MASK   0x00000008
#define OR1K_SPR_DEBUG_DRR_IPFE_GET(X) (((X) >> 3) & 0x1)
#define OR1K_SPR_DEBUG_DRR_IPFE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffff7)) | ((!!(Y)) << 3))

/* Tick Timer Exception */
#define OR1K_SPR_DEBUG_DRR_TTE_OFFSET 4
#define OR1K_SPR_DEBUG_DRR_TTE_MASK   0x00000010
#define OR1K_SPR_DEBUG_DRR_TTE_GET(X) (((X) >> 4) & 0x1)
#define OR1K_SPR_DEBUG_DRR_TTE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffffef)) | ((!!(Y)) << 4))

/* Alignment Exception */
#define OR1K_SPR_DEBUG_DRR_AE_OFFSET 5
#define OR1K_SPR_DEBUG_DRR_AE_MASK   0x00000020
#define OR1K_SPR_DEBUG_DRR_AE_GET(X) (((X) >> 5) & 0x1)
#define OR1K_SPR_DEBUG_DRR_AE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffffdf)) | ((!!(Y)) << 5))

/* Illegal Instruction Exception */
#define OR1K_SPR_DEBUG_DRR_IIE_OFFSET 6
#define OR1K_SPR_DEBUG_DRR_IIE_MASK   0x00000040
#define OR1K_SPR_DEBUG_DRR_IIE_GET(X) (((X) >> 6) & 0x1)
#define OR1K_SPR_DEBUG_DRR_IIE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffffbf)) | ((!!(Y)) << 6))

/* Interrupt Exception */
#define OR1K_SPR_DEBUG_DRR_INTE_OFFSET 7
#define OR1K_SPR_DEBUG_DRR_INTE_MASK   0x00000080
#define OR1K_SPR_DEBUG_DRR_INTE_GET(X) (((X) >> 7) & 0x1)
#define OR1K_SPR_DEBUG_DRR_INTE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffff7f)) | ((!!(Y)) << 7))

/* DTLB Miss Exception */
#define OR1K_SPR_DEBUG_DRR_DME_OFFSET 8
#define OR1K_SPR_DEBUG_DRR_DME_MASK   0x00000100
#define OR1K_SPR_DEBUG_DRR_DME_GET(X) (((X) >> 8) & 0x1)
#define OR1K_SPR_DEBUG_DRR_DME_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffeff)) | ((!!(Y)) << 8))

/* ITLB Miss Exception */
#define OR1K_SPR_DEBUG_DRR_IME_OFFSET 9
#define OR1K_SPR_DEBUG_DRR_IME_MASK   0x00000200
#define OR1K_SPR_DEBUG_DRR_IME_GET(X) (((X) >> 9) & 0x1)
#define OR1K_SPR_DEBUG_DRR_IME_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffdff)) | ((!!(Y)) << 9))

/* Range Exception */
#define OR1K_SPR_DEBUG_DRR_RE_OFFSET 10
#define OR1K_SPR_DEBUG_DRR_RE_MASK   0x00000400
#define OR1K_SPR_DEBUG_DRR_RE_GET(X) (((X) >> 10) & 0x1)
#define OR1K_SPR_DEBUG_DRR_RE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffffbff)) | ((!!(Y)) << 10))

/* System Call Exception */
#define OR1K_SPR_DEBUG_DRR_SCE_OFFSET 11
#define OR1K_SPR_DEBUG_DRR_SCE_MASK   0x00000800
#define OR1K_SPR_DEBUG_DRR_SCE_GET(X) (((X) >> 11) & 0x1)
#define OR1K_SPR_DEBUG_DRR_SCE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xfffff7ff)) | ((!!(Y)) << 11))

/* Floating Point Exception */
#define OR1K_SPR_DEBUG_DRR_FPE_OFFSET 12
#define OR1K_SPR_DEBUG_DRR_FPE_MASK   0x00001000
#define OR1K_SPR_DEBUG_DRR_FPE_GET(X) (((X) >> 12) & 0x1)
#define OR1K_SPR_DEBUG_DRR_FPE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffefff)) | ((!!(Y)) << 12))

/* Trap Exception */
#define OR1K_SPR_DEBUG_DRR_TE_OFFSET 13
#define OR1K_SPR_DEBUG_DRR_TE_MASK   0x00002000
#define OR1K_SPR_DEBUG_DRR_TE_GET(X) (((X) >> 13) & 0x1)
#define OR1K_SPR_DEBUG_DRR_TE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xffffdfff)) | ((!!(Y)) << 13))


/******************************/
/* Performance Counters Group */
/******************************/
#define OR1K_SPR_PERF_GROUP 0x07

/* Performance Counters Count Registers */
#define OR1K_SPR_PERF_PCCR_BASE     OR1K_UNSIGNED(0x000)
#define OR1K_SPR_PERF_PCCR_COUNT    OR1K_UNSIGNED(0x008)
#define OR1K_SPR_PERF_PCCR_STEP     OR1K_UNSIGNED(0x001)
#define OR1K_SPR_PERF_PCCR_INDEX(N) (OR1K_SPR_PERF_PCCR_BASE + ((N) * OR1K_SPR_PERF_PCCR_STEP))
#define OR1K_SPR_PERF_PCCR_ADDR(N)  ((OR1K_SPR_PERF_GROUP << OR1K_SPR_GROUP_LSB) | OR1K_SPR_PERF_PCCR_INDEX(N))


/* Performance Counters Mode Registers */
#define OR1K_SPR_PERF_PCMR_BASE     OR1K_UNSIGNED(0x008)
#define OR1K_SPR_PERF_PCMR_COUNT    OR1K_UNSIGNED(0x008)
#define OR1K_SPR_PERF_PCMR_STEP     OR1K_UNSIGNED(0x001)
#define OR1K_SPR_PERF_PCMR_INDEX(N) (OR1K_SPR_PERF_PCMR_BASE + ((N) * OR1K_SPR_PERF_PCMR_STEP))
#define OR1K_SPR_PERF_PCMR_ADDR(N)  ((OR1K_SPR_PERF_GROUP << OR1K_SPR_GROUP_LSB) | OR1K_SPR_PERF_PCMR_INDEX(N))


/**************************/
/* Power Management Group */
/**************************/
#define OR1K_SPR_POWER_GROUP 0x08

/* Power Management Register */
#define OR1K_SPR_POWER_PMR_INDEX OR1K_UNSIGNED(0x000)
#define OR1K_SPR_POWER_PMR_ADDR  OR1K_UNSIGNED(0x4000)


/*******************************************/
/* Programmable Interrupt Controller Group */
/*******************************************/
#define OR1K_SPR_PIC_GROUP 0x09

/* PIC Mask Register */
#define OR1K_SPR_PIC_PICMR_INDEX OR1K_UNSIGNED(0x000)
#define OR1K_SPR_PIC_PICMR_ADDR  OR1K_UNSIGNED(0x4800)


/* PIC Status Register */
#define OR1K_SPR_PIC_PICSR_INDEX OR1K_UNSIGNED(0x002)
#define OR1K_SPR_PIC_PICSR_ADDR  OR1K_UNSIGNED(0x4802)


/********************/
/* Tick Timer Group */
/********************/
#define OR1K_SPR_TICK_GROUP 0x0a

/* Tick Timer Mode Register */
#define OR1K_SPR_TICK_TTMR_INDEX OR1K_UNSIGNED(0x000)
#define OR1K_SPR_TICK_TTMR_ADDR  OR1K_UNSIGNED(0x5000)

/* Time Period */
#define OR1K_SPR_TICK_TTMR_TP_LSB    0
#define OR1K_SPR_TICK_TTMR_TP_MSB    27
#define OR1K_SPR_TICK_TTMR_TP_BITS   28
#define OR1K_SPR_TICK_TTMR_TP_MASK   OR1K_UNSIGNED(0x0fffffff)
#define OR1K_SPR_TICK_TTMR_TP_GET(X) (((X) >> 0) & OR1K_UNSIGNED(0x0fffffff))
#define OR1K_SPR_TICK_TTMR_TP_SET(X, Y) (((X) & OR1K_UNSIGNED(0xf0000000)) | ((Y) << 0))

/* Interrupt Pending */
#define OR1K_SPR_TICK_TTMR_IP_OFFSET 28
#define OR1K_SPR_TICK_TTMR_IP_MASK   0x10000000
#define OR1K_SPR_TICK_TTMR_IP_GET(X) (((X) >> 28) & 0x1)
#define OR1K_SPR_TICK_TTMR_IP_SET(X, Y) (((X) & OR1K_UNSIGNED(0xefffffff)) | ((!!(Y)) << 28))

/* Interrupt Enable */
#define OR1K_SPR_TICK_TTMR_IE_OFFSET 29
#define OR1K_SPR_TICK_TTMR_IE_MASK   0x20000000
#define OR1K_SPR_TICK_TTMR_IE_GET(X) (((X) >> 29) & 0x1)
#define OR1K_SPR_TICK_TTMR_IE_SET(X, Y) (((X) & OR1K_UNSIGNED(0xdfffffff)) | ((!!(Y)) << 29))

/* Mode */
#define OR1K_SPR_TICK_TTMR_MODE_LSB    30
#define OR1K_SPR_TICK_TTMR_MODE_MSB    31
#define OR1K_SPR_TICK_TTMR_MODE_BITS   2
#define OR1K_SPR_TICK_TTMR_MODE_MASK   OR1K_UNSIGNED(0xc0000000)
#define OR1K_SPR_TICK_TTMR_MODE_GET(X) (((X) >> 30) & OR1K_UNSIGNED(0x00000003))
#define OR1K_SPR_TICK_TTMR_MODE_SET(X, Y) (((X) & OR1K_UNSIGNED(0x3fffffff)) | ((Y) << 30))

/* Disabled */
#define OR1K_SPR_TICK_TTMR_MODE_DISABLE 0
/* Restart counting when TTMR[TP]==TTCR */
#define OR1K_SPR_TICK_TTMR_MODE_RESTART 1
/* Stop counting when TTMR[TP]==TTCR */
#define OR1K_SPR_TICK_TTMR_MODE_STOP 2
/* Continue counting when TTMR[TP]==TTCR */
#define OR1K_SPR_TICK_TTMR_MODE_CONTINUE 3

/* Tick Timer Count Register */
#define OR1K_SPR_TICK_TTCR_INDEX OR1K_UNSIGNED(0x001)
#define OR1K_SPR_TICK_TTCR_ADDR  OR1K_UNSIGNED(0x5001)


/*****************************/
/* Floating Point Unit Group */
/*****************************/
#define OR1K_SPR_FPU_GROUP 0x0b

#endif
