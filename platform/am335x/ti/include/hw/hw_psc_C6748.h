/**
 * \file  hw_psc_C6748.h
 *
 * \brief Hardware registers and fields for PSC module
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


#ifndef _HW_PSC_H_
#define _HW_PSC_H_

/* NOTE1
 * The actual number of MDCTL and MDSTAT register depend on number of
 * LPSC modules in a PSC. The number of MDCTL/MDSTAT registers defined
 * here would be a superset
 * e.g. PSC0 has 16 MDCTL/MDSTAT register, PSC1 has 32 MDCTL/MDSTAT
 * registers */


/* NOTE2
 * Please refer to the device specific PSC user guide to see what
 * register bit fields apply to individual registers
 * e.g. For PSC0 MERRPR0 bits 14,15 exist but for PSC1 MERRPR0
 * these bits are RESERVED */

typedef enum {
    HW_PSC_CC0              = 0,
    HW_PSC_TC0              = 1,
    HW_PSC_TC1              = 2,
    HW_PSC_EMIFA            = 3,
    HW_PSC_SPI0             = 4,
    HW_PSC_MMCSD0           = 5,
    HW_PSC_AINTC            = 6,
    HW_PSC_ARM_RAMROM       = 7,
    HW_PSC_UART0            = 9,
    HW_PSC_SCR0_SS          = 10,
    HW_PSC_SCR1_SS          = 11,
    HW_PSC_SCR2_SS          = 12,
    HW_PSC_PRU              = 13,
    HW_PSC_ARM              = 14,
    HW_PSC_DSP              = 15

} Psc0Peripheral;

typedef enum {
    HW_PSC_CC1          = 0,
    HW_PSC_USB0         = 1,
    HW_PSC_USB1         = 2,
    HW_PSC_GPIO         = 3,
    HW_PSC_UHPI         = 4,
    HW_PSC_EMAC         = 5,
    HW_PSC_DDR2_MDDR    = 6,
    HW_PSC_MCASP0       = 7,
    HW_PSC_SATA         = 8,
    HW_PSC_VPIF         = 9,
    HW_PSC_SPI1         = 10,
    HW_PSC_I2C1         = 11,
    HW_PSC_UART1        = 12,
    HW_PSC_UART2        = 13,
    HW_PSC_MCBSP0       = 14,
    HW_PSC_MCBSP1       = 15,
    HW_PSC_LCDC         = 16,
    HW_PSC_EHRPWM       = 17,
    HW_PSC_MMCSD1       = 18,
    HW_PSC_UPP          = 19,
    HW_PSC_ECAP0_1_2    = 20,
    HW_PSC_TC2          = 21,
    HW_PSC_SCRF0_SS     = 24,
    HW_PSC_SCRF1_SS     = 25,
    HW_PSC_SCRF2_SS     = 26,
    HW_PSC_SCRF6_SS     = 27,
    HW_PSC_SCRF7_SS     = 28,
    HW_PSC_SCRF8_SS     = 29,
    HW_PSC_BR_F7        = 30,
    HW_PSC_SHRAM        = 31
} Psc1Peripheral;

#define PSC_POWERDOMAIN_ALWAYS_ON    0
#define PSC_POWERDOMAIN_PD_DSP       1

#define PSC_REVID	(0x0)
#define PSC_INTEVAL	(0x18)
#define PSC_MERRPR0	(0x40)
#define PSC_MERRCR0	(0x50)
#define PSC_PERRPR	(0x60)
#define PSC_PERRCR	(0x68)
#define PSC_PTCMD	(0x120)
#define PSC_PTSTAT	(0x128)
#define PSC_PDSTAT0	(0x200)
#define PSC_PDSTAT1	(0x204)
#define PSC_PDCTL0	(0x300)
#define PSC_PDCTL1	(0x304)
#define PSC_PDCFG0	(0x400)
#define PSC_PDCFG1	(0x404)
#define PSC_MDSTAT(n)	(0x800 + (n * 4))
#define PSC_MDCTL(n)	(0xA00 + (n * 4))

/******************************************************************************
**                      FIELD DEFINITION MACROS
******************************************************************************/

/* REVID */

#define PSC_REVID_REV             (0xFFFFFFFFu)
#define PSC_REVID_REV_SHIFT            (0x00000000u)

/* INTEVAL */

#define PSC_INTEVAL_ALLEV      (0x00000001u)
#define PSC_INTEVAL_ALLEV_SHIFT     (0x00000000u)

/* MERRPR0 */
#define PSC_MERRPR0_M15        (0x0000C000u)
#define PSC_MERRPR0_M15_SHIFT       (0x0000000Eu)
#define PSC_MERRPR0_M14        (0x00006000u)
#define PSC_MERRPR0_M14_SHIFT       (0x0000000Du)

/* MERRCR0 */
#define PSC_MERRCR0_M15        (0x0000C000u)
#define PSC_MERRCR0_M15_SHIFT       (0x0000000Eu)
#define PSC_MERRCR0_M14        (0x00006000u)
#define PSC_MERRCR0_M14_SHIFT       (0x0000000Du)

/* PERRPR */
#define PSC_PERRPR_P1          (0x00000002u)
#define PSC_PERRPR_P1_SHIFT         (0x00000001u)
#define PSC_PERRPR_P0          (0x00000001u)
#define PSC_PERRPR_P0_SHIFT         (0x00000000u)

/* PERRCR */
#define PSC_PERRCR_P1          (0x00000002u)
#define PSC_PERRCR_P1_SHIFT         (0x00000001u)
#define PSC_PERRCR_P0          (0x00000001u)
#define PSC_PERRCR_P0_SHIFT         (0x00000000u)

/* PTCMD */
#define PSC_PTCMD_GO1          (0x00000002u)
#define PSC_PTCMD_GO1_SHIFT         (0x00000001u)
#define PSC_PTCMD_GO0          (0x00000001u)
#define PSC_PTCMD_GO0_SHIFT         (0x00000000u)

/* PTSTAT */
#define PSC_PTSTAT_GOSTAT1     (0x00000002u)
#define PSC_PTSTAT_GOSTAT1_SHIFT    (0x00000001u)
#define PSC_PTSTAT_GOSTAT0     (0x00000001u)
#define PSC_PTSTAT_GOSTAT0_SHIFT    (0x00000000u)

/* PDSTAT0 */
#define PSC_PDSTAT0_EMUIHB     (0x00000800u)
#define PSC_PDSTAT0_EMUIHB_SHIFT    (0x0000000Bu)
#define PSC_PDSTAT0_STATE      (0x0000001Fu)
#define PSC_PDSTAT0_STATE_SHIFT     (0x00000000u)

/* PDSTAT1 */
#define PSC_PDSTAT1_EMUIHB     (0x00000800u)
#define PSC_PDSTAT1_EMUIHB_SHIFT    (0x0000000Bu)
#define PSC_PDSTAT1_STATE      (0x0000001Fu)
#define PSC_PDSTAT1_STATE_SHIFT     (0x00000000u)

/* PDCTL0 */
#define PSC_PDCTL0_WAKECNT     (0x00FF0000u)
#define PSC_PDCTL0_WAKECNT_SHIFT    (0x00000010u)
#define PSC_PDCTL0_PDMODE      (0x0000F000u)
#define PSC_PDCTL0_PDMODE_SHIFT     (0x0000000Cu)
#define PSC_PDCTL0_EMUIHBIE    (0x00000200u)
#define PSC_PDCTL0_EMUIHBIE_SHIFT   (0x00000009u)
#define PSC_PDCTL0_NEXT        (0x00000001u)
#define PSC_PDCTL0_NEXT_SHIFT       (0x00000000u)

/* PDCTL1 */
#define PSC_PDCTL1_WAKECNT     (0x00FF0000u)
#define PSC_PDCTL1_WAKECNT_SHIFT    (0x00000010u)
#define PSC_PDCTL1_PDMODE      (0x0000F000u)
#define PSC_PDCTL1_PDMODE_SHIFT     (0x0000000Cu)
/*----PDMODE Tokens----*/
#define PSC_PDCTL1_PDMODE_OFF       (0x00000000u)
#define PSC_PDCTL1_PDMODE_RAM_OFF   (0x00000008u)
#define PSC_PDCTL1_PDMODE_DEEP_SLEEP (0x00000009u)
#define PSC_PDCTL1_PDMODE_LIGHT_SLEEP (0x0000000Au)
#define PSC_PDCTL1_PDMODE_RETENTION (0x0000000Bu)
#define PSC_PDCTL1_PDMODE_ON        (0x0000000Fu)

#define PSC_PDCTL1_EMUIHBIE    (0x00000200u)
#define PSC_PDCTL1_EMUIHBIE_SHIFT   (0x00000009u)
#define PSC_PDCTL1_NEXT        (0x00000001u)
#define PSC_PDCTL1_NEXT_SHIFT       (0x00000000u)

/* PDCFG0 */
#define PSC_PDCFG0_PDLOCK      (0x00000008u)
#define PSC_PDCFG0_PDLOCK_SHIFT     (0x00000003u)
#define PSC_PDCFG0_ICEPICK     (0x00000004u)
#define PSC_PDCFG0_ICEPICK_SHIFT    (0x00000002u)
#define PSC_PDCFG0_RAM_PSM     (0x00000002u)
#define PSC_PDCFG0_RAM_PSM_SHIFT    (0x00000001u)
#define PSC_PDCFG0_ALWAYSON    (0x00000001u)
#define PSC_PDCFG0_ALWAYSON_SHIFT   (0x00000000u)

/* PDCFG1 */
#define PSC_PDCFG1_PDLOCK      (0x00000008u)
#define PSC_PDCFG1_PDLOCK_SHIFT     (0x00000003u)
#define PSC_PDCFG1_ICEPICK     (0x00000004u)
#define PSC_PDCFG1_ICEPICK_SHIFT    (0x00000002u)
#define PSC_PDCFG1_RAM_PSM     (0x00000002u)
#define PSC_PDCFG1_RAM_PSM_SHIFT    (0x00000001u)
#define PSC_PDCFG1_ALWAYSON    (0x00000001u)
#define PSC_PDCFG1_ALWAYSON_SHIFT   (0x00000000u)

/* MDSTAT */
#define PSC_MDSTAT_EMUIHB      (0x00020000u)
#define PSC_MDSTAT_EMUIHB_SHIFT     (0x00000011u)
#define PSC_MDSTAT_EMURST      (0x00010000u)
#define PSC_MDSTAT_EMURST_SHIFT     (0x00000010u)
#define PSC_MDSTAT_MCKOUT      (0x00001000u)
#define PSC_MDSTAT_MCKOUT_SHIFT     (0x0000000Cu)
#define PSC_MDSTAT_MRSTDONE    (0x00000800u)
#define PSC_MDSTAT_MRSTDONE_SHIFT   (0x0000000Bu)
#define PSC_MDSTAT_MRST        (0x00000400u)
#define PSC_MDSTAT_MRST_SHIFT       (0x0000000Au)
#define PSC_MDSTAT_LRSTDONE    (0x00000200u)
#define PSC_MDSTAT_LRSTDONE_SHIFT   (0x00000009u)
#define PSC_MDSTAT_LRST        (0x00000100u)
#define PSC_MDSTAT_LRST_SHIFT       (0x00000008u)
#define PSC_MDSTAT_STATE       (0x0000003Fu)
#define PSC_MDSTAT_STATE_SHIFT      (0x00000000u)
/*----STATE Tokens----*/
#define PSC_MDSTAT_STATE_SWRSTDISABLE (0x00000000u)
#define PSC_MDSTAT_STATE_SYNCRST    (0x00000001u)
#define PSC_MDSTAT_STATE_AUTOSLEEP  (0x00000004u)
#define PSC_MDSTAT_STATE_AUTOWAKE   (0x00000005u)

/* MDCTL */
#define PSC_MDCTL_FORCE        (0x80000000u)
#define PSC_MDCTL_FORCE_SHIFT       (0x0000001Fu)

#define PSC_MDCTL_EMUIHBIE     (0x00000400u)
#define PSC_MDCTL_EMUIHBIE_SHIFT    (0x0000000Au)

#define PSC_MDCTL_EMURSTIE     (0x00000200u)
#define PSC_MDCTL_EMURSTIE_SHIFT    (0x00000009u)

#define PSC_MDCTL_LRST         (0x00000100u)
#define PSC_MDCTL_LRST_SHIFT        (0x00000008u)
#define PSC_MDCTL_NEXT         (0x0000001Fu)
#define PSC_MDCTL_NEXT_SHIFT        (0x00000000u)
/*----NEXT Tokens----*/
#define PSC_MDCTL_NEXT_SWRSTDISABLE (0x00000000u)
#define PSC_MDCTL_NEXT_SYNCRST      (0x00000001u)
#define PSC_MDCTL_NEXT_DISABLE      (0x00000002u)
#define PSC_MDCTL_NEXT_ENABLE       (0x00000003u)
#define PSC_MDCTL_NEXT_AUTOWAKE     (0x00000005u)

#endif
