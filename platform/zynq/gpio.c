/*
 * Copyright (c) 2012-2015 Travis Geiselbrecht
 * Copyright (c) 2015 Christopher Anderson
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
#include <assert.h>
#include <debug.h>
#include <reg.h>
#include <stdio.h>
#include <string.h>
#include <dev/gpio.h>
#include <platform/gpio.h>

#define MAX_GPIO 128

void zynq_gpio_early_init(void)
{
}

void zynq_gpio_init(void)
{
}

int gpio_config(unsigned gpio, unsigned flags)
{
    DEBUG_ASSERT(gpio < MAX_GPIO);

    uint16_t bank = gpio / 31;
    uint16_t bit = gpio % 32;
    uint32_t mio_cfg = MIO_GPIO;

    /* MIO region, exclude EMIO. MIO needs to be configured before the GPIO block. */
    if (bank < 2) {
        if (flags & GPIO_PULLUP) {
            mio_cfg |= MIO_PULLUP;
        }

        SLCR_REG(MIO_PIN_00 + (gpio * 4)) = mio_cfg;
    }

    if (flags & GPIO_OUTPUT || flags & GPIO_INPUT) {
        RMWREG32(GPIO_DIRM(bank), bit, 1, ((flags & GPIO_OUTPUT) > 0));
        RMWREG32(GPIO_OEN(bank), bit, 1, ((flags & GPIO_OUTPUT) > 0));
    }

	return 0;
}

void gpio_set(unsigned gpio, unsigned on)
{
    DEBUG_ASSERT(gpio < MAX_GPIO);

    uint16_t bank = gpio / 32;
    uint16_t bit = gpio % 32;
    uintptr_t reg = (bit < 16) ? GPIO_MASK_DATA_LSW(bank) : GPIO_MASK_DATA_MSW(bank);
    *REG32(reg) = (~(1 << bit) << 16) | (!!on << bit);
}

int gpio_get(unsigned gpio)
{
    DEBUG_ASSERT(gpio < MAX_GPIO);

    uint16_t bank = gpio / 32;
    uint16_t bit = gpio % 32;

    return ((*REG32(GPIO_DATA_RO(bank)) & (1 << bit)) > 0);
}
