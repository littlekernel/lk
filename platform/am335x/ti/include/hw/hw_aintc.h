/*
 * hw_aintc.h
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

#ifndef _HW_AINTC_H_
#define _HW_AINTC_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Minimum unit = 1 byte */

#define AINTC_REVID                   (0x0)
#define AINTC_CR                      (0x4)
#define AINTC_GER                     (0x10)
#define AINTC_GNLR                    (0x1c)
#define AINTC_SISR                    (0x20)
#define AINTC_SICR                    (0x24)
#define AINTC_EISR                    (0x28)
#define AINTC_EICR                    (0x2c)
#define AINTC_HIEISR                  (0x34)
#define AINTC_HIEICR                  (0x38)
#define AINTC_VBR                     (0x50)
#define AINTC_VSR                     (0x54)
#define AINTC_VNR                     (0x58)
#define AINTC_GPIR                    (0x80)
#define AINTC_GPVR                    (0x84)
#define AINTC_SRSR(n)                 (0x200 + (n * 4))
#define AINTC_SECR(n)                 (0x280 + (n * 4))
#define AINTC_ESR(n)                  (0x300 + (n * 4))
#define AINTC_ECR(n)                  (0x380 + (n * 4))
#define AINTC_CMR(n)                  (0x400 + (n * 4))
#define AINTC_HIPIR(n)                (0x900 + (n * 4))
#define AINTC_HINLR(n)                (0x1100 + (n * 4))
#define AINTC_HIER                    (0x1500)
#define AINTC_HIPVR(n)                (0x1600 + (n * 4))

/**************************************************************************\
* Field Definition Macros
\**************************************************************************/

/* REVID */

#define AINTC_REVID_REV              (0xFFFFFFFFu)
#define AINTC_REVID_REV_SHIFT        (0x00000000u)

/* CR */


#define AINTC_CR_PRHOLDMODE          (0x00000010u)
#define AINTC_CR_PRHOLDMODE_SHIFT    (0x00000004u)
/*----PRHOLDMODE Tokens----*/
#define AINTC_CR_PRHOLDMODE_NO_PRHOLD (0x00000000u)
#define AINTC_CR_PRHOLDMODE_PRHOLD   (0x00000001u)

#define AINTC_CR_NESTMODE            (0x0000000Cu)
#define AINTC_CR_NESTMODE_SHIFT      (0x00000002u)
/*----NESTMODE Tokens----*/
#define AINTC_CR_NESTMODE_NONEST     (0x00000000u)
#define AINTC_CR_NESTMODE_INDIVIDUAL (0x00000001u)
#define AINTC_CR_NESTMODE_GLOBAL     (0x00000002u)
#define AINTC_CR_NESTMODE_MANUAL     (0x00000003u)


/* GER */

#define AINTC_GER_ENABLE             (0x00000001u)
#define AINTC_GER_ENABLE_SHIFT       (0x00000000u)

/* GNLR */

#define AINTC_GNLR_OVERRIDE          (0x80000000u)
#define AINTC_GNLR_OVERRIDE_SHIFT    (0x0000001Fu)

#define AINTC_GNLR_NESTLVL           (0x000001FFu)
#define AINTC_GNLR_NESTLVL_SHIFT     (0x00000000u)

/* SISR */


#define AINTC_SISR_INDEX             (0x000003FFu)
#define AINTC_SISR_INDEX_SHIFT       (0x00000000u)

/* SICR */


#define AINTC_SICR_INDEX             (0x000003FFu)
#define AINTC_SICR_INDEX_SHIFT       (0x00000000u)

/* EISR */


#define AINTC_EISR_INDEX             (0x000003FFu)
#define AINTC_EISR_INDEX_SHIFT       (0x00000000u)

/* EICR */


#define AINTC_EICR_INDEX             (0x000003FFu)
#define AINTC_EICR_INDEX_SHIFT       (0x00000000u)

/* HIEISR */


#define AINTC_HIEISR_INDEX           (0x000003FFu)
#define AINTC_HIEISR_INDEX_SHIFT     (0x00000000u)

/* HIDISR */


#define AINTC_HIDISR_INDEX           (0x000003FFu)
#define AINTC_HIDISR_INDEX_SHIFT     (0x00000000u)

/* VBR */

#define AINTC_VBR_BASE               (0xFFFFFFFFu)
#define AINTC_VBR_BASE_SHIFT         (0x00000000u)

/* VSR */


#define AINTC_VSR_SIZE               (0x000000FFu)
#define AINTC_VSR_SIZE_SHIFT         (0x00000000u)

/* VNR */

#define AINTC_VNR_NULL               (0xFFFFFFFFu)
#define AINTC_VNR_NULL_SHIFT         (0x00000000u)

/* GPIR */

#define AINTC_GPIR_NONE              (0x80000000u)
#define AINTC_GPIR_NONE_SHIFT        (0x0000001Fu)


#define AINTC_GPIR_PRI_INDX          (0x000003FFu)
#define AINTC_GPIR_PRI_INDX_SHIFT    (0x00000000u)

/* GPVR */

#define AINTC_GPVR_ADDR              (0xFFFFFFFFu)
#define AINTC_GPVR_ADDR_SHIFT        (0x00000000u)

/* SRSR */

#define AINTC_SRSR_RAW_STATUS        (0xFFFFFFFFu)
#define AINTC_SRSR_RAW_STATUS_SHIFT  (0x00000000u)

/* SECR */

#define AINTC_SECR_ENBL_STATUS       (0xFFFFFFFFu)
#define AINTC_SECR_ENBL_STATUS_SHIFT (0x00000000u)

/* ESR */

#define AINTC_ESR_ENABLE_SHIFT       (0x00000000u)

/* ECR */
#define AINTC_ECR_DISABLE            (0xFFFFFFFFu)
#define AINTC_ECR_DISABLE_SHIFT      (0x00000000u)

/* CMR */

#define AINTC_CMR_CHNL_NPLUS3        (0xFF000000u)
#define AINTC_CMR_CHNL_NPLUS3_SHIFT  (0x00000018u)

#define AINTC_CMR_CHNL_NPLUS2        (0x00FF0000u)
#define AINTC_CMR_CHNL_NPLUS2_SHIFT  (0x00000010u)

#define AINTC_CMR_CHNL_NPLUS1        (0x0000FF00u)
#define AINTC_CMR_CHNL_NPLUS1_SHIFT  (0x00000008u)

#define AINTC_CMR_CHNL_N             (0x000000FFu)
#define AINTC_CMR_CHNL_N_SHIFT       (0x00000000u)

/* HIPIR */

#define AINTC_HIPIR_NONE             (0x80000000u)
#define AINTC_HIPIR_NONE_SHIFT       (0x0000001Fu)
#define AINTC_HIPIR_PRI_INDX         (0x000003FFu)
#define AINTC_HIPIR_PRI_INDX_SHIFT   (0x00000000u)

/* HINLR */

#define AINTC_HINLR_OVERRIDE         (0x80000000u)
#define AINTC_HINLR_OVERRIDE_SHIFT   (0x0000001Fu)
#define AINTC_HINLR_NEST_LVL         (0x000001FFu)
#define AINTC_HINLR_NEST_LVL_SHIFT   (0x00000000u)


/* HINLR */

#define AINTC_HINLR_OVERRIDE         (0x80000000u)
#define AINTC_HINLR_OVERRIDE_SHIFT   (0x0000001Fu)
#define AINTC_HINLR_NEST_LVL         (0x000001FFu)
#define AINTC_HINLR_NEST_LVL_SHIFT   (0x00000000u)


/* HIER */

#define AINTC_HIER_IRQ               (0x00000002u)
#define AINTC_HIER_IRQ_SHIFT         (0x00000001u)
/*----IRQ Tokens----*/

#define AINTC_HIER_FIQ               (0x00000001u)
#define AINTC_HIER_FIQ_SHIFT         (0x00000000u)
/*----FIQ Tokens----*/


/* HIPVR */

#define AINTC_HIPVR_ADDR             (0xFFFFFFFFu)
#define AINTC_HIPVR_ADDR_SHIFT       (0x00000000u)

#ifdef __cplusplus
}
#endif

#endif
