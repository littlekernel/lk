/*
 * hw_emac1.h
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


#ifndef _HW_EMAC_CTRL_H_
#define _HW_EMAC_CTRL_H_

#ifdef __cplusplus
extern "C" {
#endif

#define EMAC_CTRL_REVID              (0x0u)
#define EMAC_CTRL_SOFTRESET          (0x4u)
#define EMAC_CTRL_INTCONTROL         (0xCu)
#define EMAC_CTRL_C0RXTHRESHEN       (0x10u)
#define EMAC_CTRL_CnRXEN(n)          (0x14u + (n << 4))
#define EMAC_CTRL_CnTXEN(n)          (0x18u + (n << 4))
#define EMAC_CTRL_CnMISCEN(n)        (0x1Cu + (n << 4))
#define EMAC_CTRL_CnRXTHRESHEN(n)    (0x20u + (n << 4))
#define EMAC_CTRL_C0RXTHRESHSTAT     (0x40u)
#define EMAC_CTRL_C0RXSTAT           (0x44u)
#define EMAC_CTRL_C0TXSTAT           (0x48u)
#define EMAC_CTRL_C0MISCSTAT         (0x4Cu)
#define EMAC_CTRL_C1RXTHRESHSTAT     (0x50u)
#define EMAC_CTRL_C1RXSTAT           (0x54u)
#define EMAC_CTRL_C1TXSTAT           (0x58u)
#define EMAC_CTRL_C1MISCSTAT         (0x5Cu)
#define EMAC_CTRL_C2RXTHRESHSTAT     (0x60u)
#define EMAC_CTRL_C2RXSTAT           (0x64u)
#define EMAC_CTRL_C2TXSTAT           (0x68u)
#define EMAC_CTRL_C2MISCSTAT         (0x6Cu)
#define EMAC_CTRL_C0RXIMAX           (0x70u)
#define EMAC_CTRL_C0TXIMAX           (0x74u)
#define EMAC_CTRL_C1RXIMAX           (0x78u)
#define EMAC_CTRL_C1TXIMAX           (0x7Cu)
#define EMAC_CTRL_C2RXIMAX           (0x80u)
#define EMAC_CTRL_C2TXIMAX           (0x84u)

/**************************************************************************\
* Field Definition Macros
\**************************************************************************/

#ifdef __cplusplus
}
#endif


#endif
