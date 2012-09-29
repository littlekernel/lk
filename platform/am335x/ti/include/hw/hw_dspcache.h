/**
 * \file  hw_cache.h
 *
 * \brief Hardware registers and fields for cache module
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


#ifndef HW_CACHE_H_
#define HW_CACHE_H_

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
**                      DSP MEMORY SYSTEM REGISTER OFFSETS
******************************************************************************/
#define DSPCACHE_L2CFG				(0x0000)
#define DSPCACHE_L1PCFG				(0x0020)
#define DSPCACHE_L1PCC				(0x0024)
#define DSPCACHE_L1DCFG				(0x0040)
#define DSPCACHE_L1DCC				(0x0044)
#define DSPCACHE_L2WBAR				(0x4000)
#define DSPCACHE_L2WWC				(0x4004)
#define DSPCACHE_L2WIBAR			(0x4010)
#define DSPCACHE_L2WIWC				(0x4014)
#define DSPCACHE_L2IBAR				(0x4018)
#define DSPCACHE_L2IWC				(0x401C)
#define DSPCACHE_L1PIBAR			(0x4020)
#define DSPCACHE_L1PIWC				(0x4024)
#define DSPCACHE_L1DWIBAR			(0x4030)
#define DSPCACHE_L1DWIWC			(0x4034)
#define DSPCACHE_L1DWBAR			(0x4040)
#define DSPCACHE_L1DWWC				(0x4044)
#define DSPCACHE_L1DIBAR			(0x4048)
#define DSPCACHE_L1DIWC				(0x404C)
#define DSPCACHE_L2WB				(0x5000)
#define DSPCACHE_L2WBINV			(0x5004)
#define DSPCACHE_L2INV				(0x5008)
#define DSPCACHE_L1PINV				(0x5028)
#define DSPCACHE_L1DWB				(0x5040)
#define DSPCACHE_L1DWBINV			(0x5044)
#define DSPCACHE_L1DINV				(0x5048)
#define DSPCACHE_MAR(n)				(0x8000 + ((n)*4))


/******************************************************************************
**                      FIELD DEFINITION MACROS
******************************************************************************/
/**
 * \registers	L1P CACHE
 *
 * \brief		These registers allow for changing L1P cache modes and manually
 *				initiating cache coherence operations.
 */

/* L1PCFG */
#define DSPCACHE_L1PCFG_L1PMODE				(0x00000007)
#define DSPCACHE_L1PCFG_L1PMODE_SHIFT		(0x00000000)

/* L1PCC */
#define DSPCACHE_L1PCC_POPER				(0x00010000)
#define DSPCACHE_L1PCC_POPER_SHIFT			(0x00000010)
#define DSPCACHE_L1PCC_OPER					(0x00000001)
#define DSPCACHE_L1PCC_OPER_SHIFT			(0x00000000)

/* L1PIWC */
#define DSPCACHE_L1PIWC_L1PIWC				(0x0000FFFF)
#define DSPCACHE_L1PIWC_L1PIWC_SHIFT		(0x00000000)

/* L1PINV */
#define DSPCACHE_L1PINV_I					(0x00000001)
#define DSPCACHE_L1PINV_I_SHIFT				(0x00000000)


/**
 * \registers	L1D CACHE
 *
 * \brief		These registers allow for changing L1D cache modes and manually
 *				initiating cache coherence operations.
 */

/* L1DCFG */
#define DSPCACHE_L1DCFG_L1DMODE				(0x00000007)
#define DSPCACHE_L1DCFG_L1DMODE_SHIFT		(0x00000000)

/* L1DCC */
#define DSPCACHE_L1DCC_POPER				(0x00010000)
#define DSPCACHE_L1DCC_POPER_SHIFT			(0x00000010)
#define DSPCACHE_L1DCC_OPER					(0x00000001)
#define DSPCACHE_L1DCC_OPER_SHIFT			(0x00000000)

/* L1DWIWC */
#define DSPCACHE_L1DWIWC_L1DWIWC			(0x0000FFFF)
#define DSPCACHE_L1DWIWC_L1DWIWC_SHIFT		(0x00000000)

/* L1DWWC */
#define DSPCACHE_L1DWWC_L1DWWC				(0x0000FFFF)
#define DSPCACHE_L1DWWC_L1DWWC_SHIFT		(0x00000000)

/* L1DIWC */
#define DSPCACHE_L1DIWC_L1DIWC				(0x0000FFFF)
#define DSPCACHE_L1DIWC_L1DIWC_SHIFT		(0x00000000)

/* L1DWB */
#define DSPCACHE_L1DWB_C					(0x00000001)
#define DSPCACHE_L1DWB_C_SHIFT				(0x00000000)

/* L1DWBINV */
#define DSPCACHE_L1DWBINV_C					(0x00000001)
#define DSPCACHE_L1DWBINV_C_SHIFT			(0x00000000)

/* L1DINV */
#define DSPCACHE_L1DINV_I					(0x00000001)
#define DSPCACHE_L1DINV_I_SHIFT				(0x00000000)


/**
 * \registers	L2 CACHE
 *
 * \brief		These registers allow for changing L2 cache modes and manually
 *				initiating cache coherence operations.
 */

/* L2CFG */
#define DSPCACHE_L2CFG_L2MODE				(0x00000007)
#define DSPCACHE_L2CFG_L2MODE_SHIFT			(0x00000000)
#define DSPCACHE_L2CFG_L2CC					(0x00000008)
#define DSPCACHE_L2CFG_L2CC_SHIFT			(0x00000003)
#define DSPCACHE_L2CFG_ID					(0x00000100)
#define DSPCACHE_L2CFG_ID_SHIFT				(0x00000008)
#define DSPCACHE_L2CFG_IP					(0x00000200)
#define DSPCACHE_L2CFG_IP_SHIFT				(0x00000009)
#define DSPCACHE_L2CFG_MMID					(0x000F0000)
#define DSPCACHE_L2CFG_MMID_SHIFT			(0x00000010)
#define DSPCACHE_L2CFG_NUM_MM				(0x0F000000)
#define DSPCACHE_L2CFG_NUM_MM_SHIFT			(0x00000018)

/* L2WWC */
#define DSPCACHE_L2WWC_L2WWC				(0x0000FFFF)
#define DSPCACHE_L2WWC_L2WWC_SHIFT			(0x00000000)

/* L2WIWC */
#define DSPCACHE_L2WIWC_L2WIWC				(0x0000FFFF)
#define DSPCACHE_L2WIWC_L2WIWC_SHIFT		(0x00000000)

/* L2IWC */
#define DSPCACHE_L2IWC_L2IWC				(0x0000FFFF)
#define DSPCACHE_L2IWC_L2IWC_SHIFT			(0x00000000)

/* L2WB */
#define DSPCACHE_L2WB_C						(0x00000001)
#define DSPCACHE_L2WB_C_SHIFT				(0x00000000)

/* L2WBINV */
#define DSPCACHE_L2WBINV_C					(0x00000001)
#define DSPCACHE_L2WBINV_C_SHIFT			(0x00000000)

/* L2INV */
#define DSPCACHE_L2INV_I					(0x00000001)
#define DSPCACHE_L2INV_I_SHIFT				(0x00000000)

/* MAR */
#define DSPCACHE_MAR_PC						(0x00000001)
#define DSPCACHE_MAR_PC_SHIFT				(0x00000000)
 
#ifdef __cplusplus
}
#endif

#endif
