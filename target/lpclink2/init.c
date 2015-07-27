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

#include <reg.h>
#include <debug.h>
#include <printf.h>
#include <kernel/thread.h>

#include <platform/lpc43xx-gpio.h>

void target_early_init(void)
{
	// UART2 on P2.10 (TX) and P2.11 (RX)
	writel(PIN_MODE(2) | PIN_PLAIN, PIN_CFG(2, 10));
	writel(PIN_MODE(2) | PIN_PLAIN | PIN_INPUT, PIN_CFG(2, 11));

	// SPIFI
	writel(PIN_MODE(3) | PIN_PLAIN, PIN_CFG(3,3)); // SPIFI_SCK
	writel(PIN_MODE(3) | PIN_PLAIN | PIN_INPUT, PIN_CFG(3, 4)); // SPIFI_SIO3
	writel(PIN_MODE(3) | PIN_PLAIN | PIN_INPUT, PIN_CFG(3, 5)); // SPIFI_SIO2
	writel(PIN_MODE(3) | PIN_PLAIN | PIN_INPUT, PIN_CFG(3, 6)); // SPIFI_MISO
	writel(PIN_MODE(3) | PIN_PLAIN | PIN_INPUT, PIN_CFG(3, 7)); // SPIFI_MOSI
	writel(PIN_MODE(3) | PIN_PLAIN, PIN_CFG(3, 8)); // SPIFI_CS
}

void target_init(void)
{
}

