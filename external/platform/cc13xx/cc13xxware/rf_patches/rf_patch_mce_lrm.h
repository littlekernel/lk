/******************************************************************************
*  Filename:       rf_patch_mce_lrm.h
*  Revised:        2015-11-16 13:15:59 +0100 (Mon, 11 Sep 2015)
*  Revision:       
*
*  Description:    Patch for CC13XX Long range mode
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

#ifndef _RF_PATCH_MCE_LRM_H
#define _RF_PATCH_MCE_LRM_H

#include <stdint.h>
#include <inc/hw_types.h>

#ifndef MCE_PATCH_TYPE
#define MCE_PATCH_TYPE static const uint32_t
#endif

#ifndef PATCH_FUN_SPEC
#define PATCH_FUN_SPEC static inline
#endif

#ifndef RFC_MCERAM_BASE
#define RFC_MCERAM_BASE 0x21008000
#endif

MCE_PATCH_TYPE patchLrmMce[342] = { 
   0x2fcf6068,
   0xdb3e0f9d,
   0x7f7f030c,
   0x00010000,
   0x00030002,
   0x00030000,
   0x000f000c,
   0x00c300cc,
   0x0033003c,
   0xcc33cccc,
   0x333333cc,
   0x0f0f0f00,
   0x03050404,
   0x01070206,
   0x00090008,
   0x000b000a,
   0x0b000c00,
   0x09000a00,
   0x07010800,
   0x05030602,
   0x03030404,
   0x01010202,
   0x00000000,
   0x00000000,
   0x0b0b0c0c,
   0x09090a0a,
   0x07070808,
   0x05050606,
   0x05050404,
   0x07070606,
   0x09090808,
   0x0b0b0a0a,
   0x00000000,
   0x00000000,
   0x01010000,
   0x03030202,
   0x00070003,
   0x0000001f,
   0x04000000,
   0x0000000f,
   0x00010387,
   0x004340c9,
   0x80048000,
   0x000006f0,
   0x0524091e,
   0x00070054,
   0x50140000,
   0x00000050,
   0x7f7f001f,
   0x3c33013e,
   0xcccc333c,
   0x00003cc3,
   0x72487220,
   0x73057303,
   0x73047203,
   0x72047306,
   0x72767376,
   0x8001c7c0,
   0x90010001,
   0x08019010,
   0x720c9001,
   0x720e720d,
   0x7100b0c0,
   0xa0c0b0f0,
   0x81327218,
   0x39521020,
   0x00200670,
   0x11011630,
   0x6c011401,
   0x60a0609e,
   0x613a60b4,
   0x609e609e,
   0x609f609e,
   0x60951220,
   0x72201210,
   0x7310730f,
   0x81817311,
   0x91800010,
   0x607cb070,
   0x60946094,
   0xc282c481,
   0x1820c470,
   0x6e236f13,
   0x16121611,
   0xc48268a4,
   0x1820c810,
   0x120340b1,
   0x16126e23,
   0x784068ae,
   0x60949ab0,
   0x7310730f,
   0x91c0c000,
   0x8ab0c009,
   0x44c222b0,
   0x16103980,
   0xc6d5c4f4,
   0x60c8c066,
   0x06703980,
   0xc0f41610,
   0xc036c0b5,
   0x44ce1e10,
   0xc008c067,
   0x60dfc01e,
   0x44d41e20,
   0xc018c0a7,
   0x60dfc03e,
   0x44da1e40,
   0xc038c0e7,
   0x60dfc07e,
   0x44921e80,
   0xc078c127,
   0x1062c0fe,
   0xe0301612,
   0x98789862,
   0x816065f1,
   0x81409490,
   0xb0d89290,
   0xb1087100,
   0xb200a0d8,
   0xb003b480,
   0xb002b013,
   0xb0d0b012,
   0xb1007100,
   0x22f08140,
   0xc0f044f3,
   0x84509290,
   0x71009490,
   0x8460b100,
   0x71009490,
   0x8430b100,
   0x71009490,
   0x8440b100,
   0x81af9490,
   0x81e06523,
   0x410f2210,
   0x10606109,
   0xc00f1620,
   0x69116523,
   0xc030a0d1,
   0xc0409760,
   0xb0f19780,
   0x7100b0c1,
   0xa0c1b0f1,
   0xa0037276,
   0x7220a002,
   0x061f6094,
   0x00f9306f,
   0x04411091,
   0x87da97c1,
   0x1091061a,
   0x97c10451,
   0x061b87db,
   0x14ba311b,
   0x147a3919,
   0x71006fa3,
   0x9493b100,
   0x7000929e,
   0xc1f0b0f8,
   0x78509400,
   0x721b9410,
   0x10768ab7,
   0x06773987,
   0x87d897c7,
   0x22b61618,
   0xd0404600,
   0x65f19867,
   0x7830657d,
   0x65909810,
   0xb017b007,
   0xb0d4b104,
   0xc004c205,
   0x65ea65d9,
   0x495b1c54,
   0x16149190,
   0x1e018151,
   0x16714156,
   0x4d561c41,
   0x796ea0d7,
   0x95ee797f,
   0x120495ff,
   0x161465ea,
   0x416e1c54,
   0x61689190,
   0xc050a205,
   0xa0d765f1,
   0x7206a0d4,
   0x72047202,
   0x72047203,
   0x73067305,
   0x72767376,
   0xc0616094,
   0xc0023181,
   0x6e12cff3,
   0x6e121611,
   0xc7e01611,
   0x16116e13,
   0x16116e12,
   0xcff06986,
   0x16116e12,
   0x7000698c,
   0xb016b006,
   0xb014b004,
   0xb012b002,
   0x90307810,
   0x78209050,
   0x90609040,
   0xb072b205,
   0x93b0c0b0,
   0xb0e6b116,
   0xb0737100,
   0xb116b107,
   0xb0d7a0e6,
   0x7100b88e,
   0xb041b107,
   0xc0f0b061,
   0xb88f93b0,
   0x95548964,
   0x7100a044,
   0xb04db107,
   0xb074b06d,
   0x7100b201,
   0x7000b107,
   0x120a1209,
   0x140965cf,
   0x65cf141a,
   0x140a1419,
   0x45cc1e77,
   0x140965cf,
   0x65cf141a,
   0x140a1419,
   0x3c8a3c89,
   0x71007000,
   0x7100b107,
   0x87f0b107,
   0x39803180,
   0x39818801,
   0x65bc7000,
   0x10ac109b,
   0x10bd65bc,
   0x14db318d,
   0x318d10cd,
   0x318a14dc,
   0x149b14a9,
   0x95eb149c,
   0x700095fc,
   0xb079a0d7,
   0xb1047100,
   0x8820b0d7,
   0x98507000,
   0x22008840,
   0xb83045f2,
   0x00007000,
   0x00000000,
   0x00000000,
   0x00000000,
   0x00000000,
   0x9867d060,
   0xe9b165f1,
   0x95a06f10,
   0xe9c1c100,
   0x6f13c622,
   0x16116e23,
   0x6a091612,
   0x42621e07,
   0xb04f6590,
   0xc0f5b06f,
   0x6652c004,
   0x85d0b5b0,
   0x1c543920,
   0x91904a1c,
   0x81511614,
   0x42151e01,
   0x1c411621,
   0xa0d74e15,
   0x797f796e,
   0x95ff95ee,
   0xb5b01204,
   0x392085d0,
   0x1c541614,
   0x91904231,
   0xa2056229,
   0x65f1c070,
   0xa0d4a0d7,
   0x72027206,
   0x72037204,
   0x73057204,
   0x73767306,
   0x60947276,
   0x12091070,
   0xb1077100,
   0x89b3b88d,
   0x3d833183,
   0x424b2006,
   0x14390bf3,
   0x3c896a42,
   0x3d391649,
   0x700006f9,
   0xc28a6640,
   0x6fab149a,
   0x149ac38a,
   0xc18a6fac,
   0x149a6640,
   0x14db6fad,
   0x95ec14dc,
   0x700095fb,
   0x65f1c080,
   0xb04f6590,
   0xc0f5b06f,
   0x7100c004,
   0x7100b107,
   0x87fcb107,
   0x95ec880b,
   0xb5b095fb,
   0x392085d0,
   0x4a771c54,
   0x16149190,
   0x1e018151,
   0x16214269,
   0x4e691c41,
   0x796ea0d7,
   0x95ee797f,
   0x120495ff,
   0x85d0b5b0,
   0x16143920,
   0x428c1c54,
   0x62849190,
   0xc090a205,
   0xa0d765f1,
   0x7206a0d4,
   0x72047202,
   0x72047203,
   0x73067305,
   0x72767376,
   0x00016094,
   0x00080018,
   0x001a0003,
   0x002c000a,
   0x003e0011,
   0x00080003,
   0x001a0018,
   0x002c0011,
   0x003e000a
};

PATCH_FUN_SPEC void rf_patch_mce_lrm(void)
{
#ifdef __PATCH_NO_UNROLLING
   uint32_t i;
   for (i = 0; i < 342; i++) {
      HWREG(RFC_MCERAM_BASE + 4 * i) = patchLrmMce[i];
   }
#else
   const uint32_t *pS = patchLrmMce;
   volatile unsigned long *pD = &HWREG(RFC_MCERAM_BASE);
   uint32_t t1, t2, t3, t4, t5, t6, t7, t8;
   uint32_t nIterations = 42;

   do {
      t1 = *pS++;
      t2 = *pS++;
      t3 = *pS++;
      t4 = *pS++;
      t5 = *pS++;
      t6 = *pS++;
      t7 = *pS++;
      t8 = *pS++;
      *pD++ = t1;
      *pD++ = t2;
      *pD++ = t3;
      *pD++ = t4;
      *pD++ = t5;
      *pD++ = t6;
      *pD++ = t7;
      *pD++ = t8;
   } while (--nIterations);

   t1 = *pS++;
   t2 = *pS++;
   t3 = *pS++;
   t4 = *pS++;
   t5 = *pS++;
   t6 = *pS++;
   *pD++ = t1;
   *pD++ = t2;
   *pD++ = t3;
   *pD++ = t4;
   *pD++ = t5;
   *pD++ = t6;
#endif
}

#endif
