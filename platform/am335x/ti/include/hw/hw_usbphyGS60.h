/** ============================================================================
 *   \file  hw_usbphyGS60.h
 *
 *   \brief This file contains the bit field values to use with the USB phy register
 *
 *  ============================================================================
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


#ifndef __HW_USBPHYGS60_H__
#define __HW_USBPHYGS60_H__

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************************************
**                      PHY REGISTER ADDRESS
*******************************************************************************/
#define CFGCHIP2_USBPHYCTRL 	SOC_USB_0_PHY_REGS	


/******************************************************************************
**                      BIT FIELDS TO USE WITH PHY REGISTER
*******************************************************************************/
#define CFGCHIP2_PHYCLKGD       (1 << 17)

#define CFGCHIP2_VBUSSENSE      (1 << 16)

#define CFGCHIP2_RESET          (1 << 15)

#define CFGCHIP2_OTGMODE        (3 << 13)

#define CFGCHIP2_NO_OVERRIDE    (0 << 13)

#define CFGCHIP2_FORCE_HOST     (1 << 13)

#define CFGCHIP2_FORCE_DEVICE   (2 << 13)

#define CFGCHIP2_FORCE_HOST_VBUS_LOW (3 << 13)

#define CFGCHIP2_USB1PHYCLKMUX  (1 << 12)

#define CFGCHIP2_USB2PHYCLKMUX  (1 << 11)

#define CFGCHIP2_PHYPWRDN       (1 << 10)

#define CFGCHIP2_OTGPWRDN       (1 << 9)

#define CFGCHIP2_DATPOL         (1 << 8)

#define CFGCHIP2_USB1SUSPENDM   (1 << 7)

#define CFGCHIP2_PHY_PLLON      (1 << 6)        /* override PLL suspend */

#define CFGCHIP2_SESENDEN       (1 << 5)        /* Vsess_end comparator */

#define CFGCHIP2_VBDTCTEN       (1 << 4)        /* Vbus comparator */

#define CFGCHIP2_REFFREQ        (0xf << 0)

#define CFGCHIP2_REFFREQ_12MHZ  (1 << 0)

#define CFGCHIP2_REFFREQ_24MHZ  (2 << 0)

#define CFGCHIP2_REFFREQ_48MHZ  (3 << 0)

void UsbPhyOn(unsigned int ulIndex);
void UsbPhyOff(unsigned int ulIndex);

#ifdef __cplusplus
}
#endif

#endif // __HW_USBPHY_AM1808_H__



