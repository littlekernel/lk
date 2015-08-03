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

#define SGPIO_OUT_CFG(n)	(0x40101000 + ((n) * 4))
#define CFG_OUT_M1		0x00
#define CFG_OUT_M2A		0x01
#define CFG_OUT_M2B		0x02
#define CFG_OUT_M2C		0x03
#define CFG_OUT_GPIO		0x04
#define CFG_OUT_M4A		0x05
#define CFG_OUT_M4B		0x06
#define CFG_OUT_M4C		0x07
#define CFG_OUT_CLK		0x08
#define CFG_OUT_M8A		0x09
#define CFG_OUT_M8B		0x0A
#define CFG_OUT_M8C		0x0B
#define CFG_OE_GPIO		0x00
#define CFG_OE_M1		0x40
#define CFG_OE_M2		0x50
#define CFG_OE_M4		0x60
#define CFG_OE_M8		0x70

#define SLICE_CFG1(n) 		(0x40101040 + ((n) * 4))
#define CLK_USE_SLICE		(0 << 0)
#define CLK_USE_PIN		(1 << 0)
#define CLK_PIN_SGPIO8		(0 << 1)
#define CLK_PIN_SGPIO9		(1 << 1)
#define CLK_PIN_SGPIO10		(2 << 1)
#define CLK_PIN_SGPIO11		(3 << 1)
#define CLK_SLICE_D		(0 << 3)
#define CLK_SLICE_H		(1 << 3)
#define CLK_SLICE_O		(2 << 3)
#define CLK_SLICE_P		(3 << 3)
#define QUAL_ENABLE		(0 << 5)
#define QUAL_DISABLE		(1 << 5)
#define QUAL_USE_SLICE		(2 << 5)
#define QUAL_USE_PIN		(3 << 5)
#define QUAL_PIN_SGPIO8		(0 << 7)
#define QUAL_PIN_SGPIO9		(1 << 7)
#define QUAL_PIN_SGPIO10	(2 << 7)
#define QUAL_PIN_SGPIO11	(3 << 7)
#define QUAL_SLICE_A		(0 << 9) // D for SLICE A
#define QUAL_SLICE_H		(1 << 9) // O for SLICE H
#define QUAL_SLICE_I		(2 << 9) // D for SLICE I
#define QUAL_SLICE_P		(3 << 9) // O for SLICE P
#define CONCAT_PIN		(0 << 11)
#define CONCAT_SLICE		(1 << 11)
#define CONCAT_LOOP		(0 << 12)
#define CONCAT_2_SLICE		(1 << 12)
#define CONCAT_4_SLICE		(2 << 12)
#define CONCAT_8_SLICE		(3 << 12)

#define SLICE_CFG2(n)		(0x40101080 + ((n) * 4))
#define MATCH_MODE		(1 << 0)
#define CLK_GEN_INTERNAL	(0 << 2) // from COUNTER
#define CLK_GEN_EXTERNAL	(1 << 2) // from PIN or SLICE
#define INV_CLK_OUT		(1 << 3)
#define SHIFT_1BPC		(0 << 6)
#define SHIFT_2BPC		(1 << 6)
#define SHIFT_4BPC		(2 << 6)
#define SHIFT_8BPC		(3 << 6)
#define INVERT_QUALIFIER	(1 << 8)

#define SLICE_REG(n)		(0x401010C0 + ((n) * 4)) // main shift reg
#define SLICE_SHADOW(n)		(0x40101100 + ((n) * 4)) // swapped @ POS underflow
#define SLICE_PRESET(n)		(0x40101140 + ((n) * 4)) // 12bit -> COUNT @ 0
#define SLICE_COUNT(n)		(0x40101180 + ((n) * 4)) // 12 bit downcount
#define SLICE_POS(n)		(0x401011C0 + ((n) * 4))
#define POS_POS(n)		((n) << 0) // value at start
#define POS_RESET(n)		((n) << 8) // load at underflow

#define SGPIO_IN		(0x40101210)
#define SGPIO_OUT		(0x40101214)
#define SGPIO_OEN		(0x40101218)
#define SLICE_CTRL_ENABLE	(0x4010121C)
#define SLICE_CTRL_DISABLE	(0x40101220)
#define SLICE_XHG_STS		(0x40101F2C)
#define SLICE_XHG_STS_CLR	(0x40101F30)
#define SLICE_XHG_STS_SET	(0x40101F34)

#define SLC_A			0
#define SLC_B			1
#define SLC_C			2
#define SLC_D			3
#define SLC_E			4
#define SLC_F			5
#define SLC_G			6
#define SLC_H			7
#define SLC_I			8
#define SLC_J			9
#define SLC_K			10
#define SLC_L			11
#define SLC_M			12
#define SLC_N			13
#define SLC_O			14
#define SLC_P			15

