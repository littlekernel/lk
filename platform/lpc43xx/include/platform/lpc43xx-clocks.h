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

#define BASE_X_PD		(1 << 0) // power-down
#define BASE_X_AUTOBLOCK	(1 << 11)
#define BASE_X_SEL(n)		((n) << 24)

#define CLK_32K		0x00
#define CLK_IRC		0x01 // 12MHz internal RC OSC
#define CLK_ENET_RX	0x02
#define CLK_ENET_TX	0x03
#define CLK_GP_CLKIN	0x04
#define CLK_XTAL_OSC	0x06
#define CLK_PLL0USB	0x07 // only allowed for BASE_{USB0,USB1,OUT}_CLK
#define CLK_PLL0AUDIO	0x08
#define CLK_PLL1	0x09
#define CLK_IDIVA	0x0C
#define CLK_IDIVB	0x0D
#define CLK_IDIVC	0x0E
#define CLK_IDIVD	0x0F
#define CLK_IDIVE	0x10

