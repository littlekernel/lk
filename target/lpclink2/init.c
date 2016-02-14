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
    pin_config(PIN(2,10), PIN_MODE(2) | PIN_PLAIN);
    pin_config(PIN(2,11), PIN_MODE(2) | PIN_PLAIN | PIN_INPUT);

    // SPIFI
    pin_config(PIN(3,3), PIN_MODE(3) | PIN_PLAIN); // SPIFI_SCK
    pin_config(PIN(3,4), PIN_MODE(3) | PIN_PLAIN | PIN_INPUT); // SPIFI_SIO3
    pin_config(PIN(3,5), PIN_MODE(3) | PIN_PLAIN | PIN_INPUT); // SPIFI_SIO2
    pin_config(PIN(3,6), PIN_MODE(3) | PIN_PLAIN | PIN_INPUT); // SPIFI_MISO
    pin_config(PIN(3,7), PIN_MODE(3) | PIN_PLAIN | PIN_INPUT); // SPIFI_MOSI
    pin_config(PIN(3,8), PIN_MODE(3) | PIN_PLAIN); // SPIFI_CS
}

void target_init(void)
{
}

