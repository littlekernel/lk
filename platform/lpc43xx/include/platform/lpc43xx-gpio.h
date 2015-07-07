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

// pinmux

#define PIN_CFG(m,n)	(0x40086000 + ((m) * 0x80) + ((n) * 4))
#define PIN_MODE(n)	((n) & 3)
#define PIN_PULLUP	(0 << 3) // pull-up, no pull-down
#define PIN_REPEATER	(1 << 3) // repeater mode
#define PIN_PLAIN	(2 << 3) // no pull-up, no pull-down
#define PIN_PULLDOWN	(3 << 3) // pull-down, no pull-up
#define PIN_SLOW	(0 << 5) // slow slew rate (low noise, medium speed)
#define PIN_FAST	(1 << 5) // fast slew rate (medium noise, fast speed)
#define PIN_INPUT	(1 << 6) // enable input buffer, required for inputs
#define PIN_FILTER	(1 << 7) // enable glitch filter, not for >30MHz signals

// gpio

// each GPIO as a single byte or word register
// write zero to clear
// write non-zero to set
// reads as zero if input is low
// reads as FF (byte) or FFFFFFFF (word) if input is high
#define GPIO_BYTE(m,n)	(0x400F4000 + ((m) * 0x20) + (n))
#define GPIO_WORD(m,n)	(0x400F5000 + ((m) * 0x80) + ((n) * 4))

// GPIOs grouped by port, with one bit per pin
#define GPIO_DIR(m)	(0x400F6000 + ((m) * 4)) // 1 = output, 0 = input
#define GPIO_MASK(m)	(0x400F6080 + ((m) * 4)) // 1s disable MPIN() bits
#define GPIO_PIN(m)	(0x400F6100 + ((m) * 4)) // r/w value at pins
#define GPIO_MPIN(m)	(0x400F6180 + ((m) * 4)) // r value at pins & ~MASK
						 // w only MASK bits
#define GPIO_SET(m)	(0x400F6200 + ((m) * 4)) // write 1s to set
#define GPIO_CLR(m)	(0x400F6280 + ((m) * 4)) // write 1s to clear
#define GPIO_NOT(m)	(0x400F6300 + ((m) * 4)) // write 1s to invert



