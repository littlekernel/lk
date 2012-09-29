/*
 * hw_hpi.h
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



#ifndef _HW_HPI_H_
#define _HW_HPI_H_

#ifdef __cplusplus
extern "C" {
#endif

#define HPI_REVID		(0x0)
#define HPI_PWREMU_MGMT		(0x4)
#define HPI_GPIOEN		(0xC)
#define HPI_GPIODIR1		(0x10)
#define HPI_GPIODAT1		(0x14)
#define HPI_GPIODIR2		(0x18)
#define HPI_GPIODAT2		(0x1C)
#define HPI_HPIC		(0x30)
#define HPI_HPIAW		(0x34)
#define HPI_HPIAR		(0x38)

/**************************************************************************\
* Field Definition Macros
\**************************************************************************/

/* REVID */


#define HPI_REVID_REV         (0xFFFFFFFFu)
#define HPI_REVID_REV_SHIFT        (0x00000000u)


/* PWREMU_MGMT */


#define HPI_PWREMU_MGMT_SOFT (0x00000002u)
#define HPI_PWREMU_MGMT_SOFT_SHIFT (0x00000001u)

#define HPI_PWREMU_MGMT_FREE (0x00000001u)
#define HPI_PWREMU_MGMT_FREE_SHIFT (0x00000000u)


/* HPIC */


#define HPI_HPIC_HPIASEL (0x00000800u)
#define HPI_HPIC_HPIASEL_SHIFT (0x0000000Bu)


#define HPI_HPIC_DUALHPIA (0x00000200u)
#define HPI_HPIC_DUALHPIA_SHIFT (0x00000009u)

#define HPI_HPIC_HWOBSTAT (0x00000100u)
#define HPI_HPIC_HWOBSTAT_SHIFT (0x00000008u)



#define HPI_HPIC_FETCH (0x00000010u)
#define HPI_HPIC_FETCH_SHIFT (0x00000004u)


#define HPI_HPIC_HINT (0x00000004u)
#define HPI_HPIC_HINT_SHIFT (0x00000002u)

#define HPI_HPIC_DSPINT (0x00000002u)
#define HPI_HPIC_DSPINT_SHIFT (0x00000001u)

#define HPI_HPIC_HWOB (0x00000001u)
#define HPI_HPIC_HWOB_SHIFT (0x00000000u)


/* HPIAW */

#define HPI_HPIAW_HPIAW (0xFFFFFFFFu)
#define HPI_HPIAW_HPIAW_SHIFT (0x00000000u)


/* HPIAR */

#define HPI_HPIAR_HPIAR (0xFFFFFFFFu)
#define HPI_HPIAR_HPIAR_SHIFT (0x00000000u)


#ifdef __cplusplus
}
#endif

#endif

