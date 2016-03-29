/******************************************************************************
*  Filename:       rf_patch_cpe_genfsk.h
*  Revised:        2015-09-11 13:15:59 +0100 (fr, 11 Sep 2015)
*  Revision:       
*
*  Description:    Patch for CC13XX Generic FSK
*
*  Copyright (c) 2015, Texas Instruments Incorporated
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions are met:
*
*  1) Redistributions of source code must retain the above copyright notice,
*     this list of conditions and the following disclaimer.
*
*  2) Redistributions in binary form must reproduce the above copyright notice,
*     this list of conditions and the following disclaimer in the documentation
*     and/or other materials provided with the distribution.
*
*  3) Neither the name of the ORGANIZATION nor the names of its contributors may
*     be used to endorse or promote products derived from this software without
*     specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
*  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
*  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
*  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
*  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
*  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
*  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
*  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
*  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
*  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*
******************************************************************************/

#ifndef _RF_PATCH_CPE_GENFSK_H
#define _RF_PATCH_CPE_GENFSK_H

#include <stdint.h>
#include <string.h>

#ifndef CPE_PATCH_TYPE
#define CPE_PATCH_TYPE static const uint32_t
#endif

#ifndef SYS_PATCH_TYPE
#define SYS_PATCH_TYPE static const uint32_t
#endif

#ifndef PATCH_FUN_SPEC
#define PATCH_FUN_SPEC static inline
#endif


CPE_PATCH_TYPE patchImageGenfsk[] = {
   0x21000479,
   0x210004dd,
   0x210004fd,
   0x21000527,
   0x2100055d,
   0x21000465,
   0x4710b5f8,
   0x460eb5f8,
   0x25012100,
   0x473004ad,
   0x4904b570,
   0xb5704708,
   0x47084903,
   0x4903b5f8,
   0x00004708,
   0x00004bc9,
   0x00004a69,
   0x00004ce5,
   0x21004a02,
   0xb4f06011,
   0x47084901,
   0x40044014,
   0x000068cf,
   0x4603b570,
   0x29014615,
   0x2900d006,
   0x4a11d006,
   0xf7ff4628,
   0xbd70ffd3,
   0xe000480f,
   0x2405480f,
   0xd8034283,
   0x1e640840,
   0xdcf92c00,
   0x200140e3,
   0x18180340,
   0x29010b82,
   0x4906d007,
   0x31802300,
   0xf7ff4628,
   0xb2e0ffbd,
   0x4902bd70,
   0x316c4b04,
   0x0000e7f6,
   0x00005c83,
   0x2386bca0,
   0x230d8300,
   0x21000660,
   0x481ab510,
   0x56412100,
   0xd1021c49,
   0x7ac94918,
   0x49177001,
   0x31604817,
   0xf7ff82c8,
   0xbd10ffa8,
   0x4815b510,
   0xf7ff7ac4,
   0x07a0ffa5,
   0x490fd00c,
   0x56082000,
   0xd0071c42,
   0x0212220f,
   0x4a0c4310,
   0x82d03260,
   0x700820ff,
   0xb510bd10,
   0x21004807,
   0x1c495641,
   0x4906d102,
   0x70017ac9,
   0x48074904,
   0x82c83160,
   0xff80f7ff,
   0x0000bd10,
   0x21000690,
   0x40086200,
   0x00000f07,
   0x21000260,
   0x00000808,
   0x4e1ab5f8,
   0x6b714605,
   0x09cc4819,
   0x2d0001e4,
   0x4918d011,
   0x29027809,
   0x7b00d00f,
   0xb6724304,
   0x4f152001,
   0x47b80240,
   0x38204811,
   0x09c18800,
   0xd00407c9,
   0x7ac0e016,
   0x7b40e7f0,
   0x490fe7ee,
   0x61cc6374,
   0x07c00a40,
   0x2001d00c,
   0x6b310380,
   0xd0012d00,
   0xe0004301,
   0x46084381,
   0x49076331,
   0x63483940,
   0x47b82000,
   0xbdf8b662,
   0x21000280,
   0x21000088,
   0x2100029b,
   0x00003f7b,
   0x40044040,
   0x4d17b538,
   0x28007f28,
   0x4816d127,
   0x09c08800,
   0xd12207c0,
   0x69604c14,
   0xd11e2800,
   0xf0004668,
   0x4668f82b,
   0x28017800,
   0x4668d117,
   0x28107840,
   0x2008d213,
   0x6a686160,
   0x01400940,
   0x4a0b6020,
   0x62d12100,
   0x21024a0a,
   0x21016011,
   0x60204308,
   0x43082103,
   0x60206268,
   0x4806bd38,
   0xbd384780,
   0x21000280,
   0x21000068,
   0x40044000,
   0x40041100,
   0xe000e280,
   0x00003bc3,
   0x4801b403,
   0xbd019001,
   0x00006fa5,
   0x00000000,
   0x00030001,
   0x001f000a,
   0x00eb0059,
   0x04ea0239,
   0x129709f9,
   0x32a11feb,
   0x660a4a78,
   0x9e8c82fa,
   0xc917b663,
   0xdeedd664,
   0xe5e0e3c1,
   0x000000ff,
};
#define _NWORD_PATCHIMAGE 158

#define _NWORD_PATCHSYS 0

#define IRQ_PATCH_0 0x210005dd


#define _SYS_PATCH_FIXED_ADDR 0x20000000

PATCH_FUN_SPEC void enterCpePatchGenfsk(void)
{
   uint32_t *pPatchVec = (uint32_t *) 0x2100041C;

#if (_NWORD_PATCHIMAGE > 0)
   memcpy(pPatchVec, patchImageGenfsk, sizeof(patchImageGenfsk));
#endif
}

PATCH_FUN_SPEC void enterSysPatchGenfsk(void)
{
}

PATCH_FUN_SPEC void configurePatchGenfsk(void)
{
   uint8_t *pPatchTab = (uint8_t *) 0x21000340;
   uint32_t *pIrqPatch = (uint32_t *) 0x210003BC;


   pPatchTab[80] = 0;
   pPatchTab[53] = 1;
   pPatchTab[57] = 2;
   pPatchTab[55] = 3;
   pPatchTab[110] = 4;
   pPatchTab[107] = 5;

   pIrqPatch[1] = IRQ_PATCH_0;
}

PATCH_FUN_SPEC void rf_patch_cpe_genfsk(void)
{
   enterSysPatchGenfsk();
   enterCpePatchGenfsk();
   configurePatchGenfsk();
}

PATCH_FUN_SPEC void refreshPatchGenfsk(void)
{
   enterCpePatchGenfsk();
   configurePatchGenfsk();
}

#undef IRQ_PATCH_0

#endif
