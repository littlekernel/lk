/*
 * Copyright (c) 2015 Brian Swetland
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once

#define FREQ_MON		0x40050014
#define XTAL_OSC_CTRL		0x40050018

#define PLL0USB_STAT		0x4005001C
#define PLL0USB_CTRL		0x40050020
#define PLL0USB_MDIV		0x40050024
#define PLL0USB_NP_DIV		0x40050028

#define PLL0AUDIO_STAT		0x4005002C
#define PLL0AUDIO_CTRL		0x40050030
#define PLL0AUDIO_MDIV		0x40050034
#define PLL0AUDIO_NP_DIV	0x40050038
#define PLL0AUDIO_FRAC		0x4005003C

#define PLL1_STAT		0x40050040
#define PLL1_CTRL		0x40050044

#define IDIVA_CTRL		0x40050048 // /(1..4)   IRC, CLKIN, XTAL, PLLs
#define IDIVB_CTRL		0x4005004C // /(1..16)  IRC, CLKIN, XTAL, PLL0AUDIO, PLL1, IDIVA
#define IDIVC_CTRL		0x40050050 // /(1..16)  "
#define IDIVD_CTRL		0x40050054 // /(1..16)  "
#define IDIVE_CTRL		0x40050058 // /(1..256) IRC, CLKIN, XTAL, PLL0AUDIO, PLL1, IDIVA

#define BASE_SAFE_CLK		0x4005005C // only CLK_IRC allowed
#define BASE_USB0_CLK		0x40050060 // only CLK_PLL0USB allowed
#define BASE_PERIPH_CLK		0x40050064
#define BASE_USB1_CLK		0x40050068
#define BASE_M4_CLK		0x4005006C
#define BASE_SPIFI_CLK		0x40050070
#define BASE_SPI_CLK		0x40050074
#define BASE_PHY_RX_CLK		0x40050078
#define BASE_PHY_TX_CLK		0x4005008C
#define BASE_APB1_CLK		0x40050080
#define BASE_APB3_CLK		0x40050084
#define BASE_LCD_CLK		0x40050088
#define BASE_ADCHS_CLK		0x4005008C
#define BASE_SDIO_CLK		0x40050090
#define BASE_SSP0_CLK		0x40050094
#define BASE_SSP1_CLK		0x40050098
#define BASE_UART0_CLK		0x4005009C
#define BASE_UART1_CLK		0x400500A0
#define BASE_UART2_CLK		0x400500A4
#define BASE_UART3_CLK		0x400500A8
#define BASE_OUT_CLK		0x400500AC
#define BASE_AUDIO_CLK		0x400500C0
#define BASE_CGU_OUT0_CLK	0x400500C4
#define BASE_CGU_OUT1_CLK	0x400500C8

#define BASE_PD			(1 << 0) // power-down
#define BASE_AUTOBLOCK		(1 << 11)
#define BASE_CLK_SEL(n)		((n) << 24)

#define PLL0_STAT_LOCK		(1 << 0)
#define PLL0_STAT_FR		(1 << 1)

#define PLL0_CTRL_PD		(1 << 0) // power down
#define PLL0_CTRL_BYPASS	(1 << 1) // input sent to post-div
#define PLL0_CTRL_DIRECTI	(1 << 2)
#define PLL0_CTRL_DIRECTO	(1 << 3)
#define PLL0_CTRL_CLKEN		(1 << 4)
#define PLL0_CTRL_FRM		(1 << 6) // free running mode
#define PLL0_CTRL_AUTOBLOCK	(1 << 11)
#define PLL0_CTRL_CLK_SEL(n)	((n) << 24) // input clock select
// PLL0AUDIO only:
#define PLL0_CTRL_PLLFRACT_REQ	(1 << 12)
#define PLL0_CTRL_SEL_EXT	(1 << 13)
#define PLL0_CTRL_MOD_PD	(1 << 14)

#define PLL1_STAT_LOCK		(1 << 0)

#define PLL1_CTRL_PD		(1 << 0)
#define PLL1_CTRL_BYPASS	(1 << 1)
#define PLL1_CTRL_FBSEL		(1 << 6)
#define PLL1_CTRL_DIRECT	(1 << 7)
#define PLL1_CTRL_PSEL_1	(0 << 8)
#define PLL1_CTRL_PSEL_2	(1 << 8)
#define PLL1_CTRL_PSEL_4	(2 << 8)
#define PLL1_CTRL_PSEL_8	(3 << 8)
#define PLL1_CTRL_AUTOBLOCK	(1 << 11)
#define PLL1_CTRL_NSEL_1	(0 << 12)
#define PLL1_CTRL_NSEL_2	(1 << 12)
#define PLL1_CTRL_NSEL_3	(2 << 12)
#define PLL1_CTRL_NSEL_4	(3 << 12)
#define PLL1_CTRL_MSEL(m)	(((m) - 1) << 16)
#define PLL1_CTRL_CLK_SEL(c)	((c) << 24)

#define IDIV_PD			(1 << 0)
#define IDIV_N(n)		(((n) - 1) << 2)
#define IDIV_AUTOBLOCK		(1 << 11)
#define IDIV_CLK_SEL(c)		((c) << 24)

#define CLK_32K			0x00
#define CLK_IRC			0x01 // 12MHz internal RC OSC
#define CLK_ENET_RX		0x02
#define CLK_ENET_TX		0x03
#define CLK_GP_CLKIN		0x04
#define CLK_XTAL		0x06 // crystal oscillator
#define CLK_PLL0USB		0x07 // only for BASE_{USB0,USB1,OUT}_CLK
#define CLK_PLL0AUDIO		0x08
#define CLK_PLL1		0x09
#define CLK_IDIVA		0x0C
#define CLK_IDIVB		0x0D
#define CLK_IDIVC		0x0E
#define CLK_IDIVD		0x0F
#define CLK_IDIVE		0x10

