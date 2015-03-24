/*
 * Copyright (c) 2012-2015 Travis Geiselbrecht
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
#include <err.h>
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include <arch/arm/mmu.h>
#include <kernel/vm.h>
#include <dev/uart.h>
#include <dev/gpio.h>
#include <lib/console.h>
#include <platform.h>
#include <platform/zynq.h>
#include "platform_p.h"

#define MAX_GPIO (64*2) // MIO + EMIO block

// registers with 4 adjacent copies
#define GPIO_REG_MASK_DATA_0_LSW (0x0)
#define GPIO_REG_DATA_0          (0x40)
#define GPIO_REG_DATA_0_RO       (0x60)

// next block of registers repeated every 0x40
#define GPIO_REG_DIRM_0          (0x204)
#define GPIO_REG_OEN_0           (0x208)

void gpio_set(uint gpio, unsigned set)
{
    DEBUG_ASSERT(gpio < MAX_GPIO);

    uint reg = gpio / 16;

    gpio %= 16;

    uint32_t mask = (~(1 << gpio) & 0xffff) << 16;
    *REG32(GPIO_BASE + GPIO_REG_MASK_DATA_0_LSW + reg * 4) = mask | (set ? 0xffff : 0);
}

int gpio_get(uint gpio)
{
    DEBUG_ASSERT(gpio < MAX_GPIO);

    uint bank = gpio / 32;

    uint32_t val = *REG32(GPIO_BASE + GPIO_REG_DATA_0_RO + bank * 4);

    return (val & (1 << (gpio % 32)));
}

int gpio_config(unsigned gpio, unsigned flags)
{
    DEBUG_ASSERT(gpio < MAX_GPIO);

    if (gpio >= MAX_GPIO)
        return ERR_INVALID_ARGS;

    uint bank = gpio / 32;

    if (flags & GPIO_OUTPUT) {
        /* configure this mio as an output */
        *REG32(GPIO_BASE + GPIO_REG_DIRM_0 + bank * 0x40) |= (1 << (gpio % 32));
        *REG32(GPIO_BASE + GPIO_REG_OEN_0 + bank * 0x40) |= (1 << (gpio % 32));
    } else {
        /* configure this mio as an input */
        *REG32(GPIO_BASE + GPIO_REG_DIRM_0 + bank * 0x40) &= ~(1 << (gpio % 32));

        if (flags & GPIO_PULLUP && gpio < 54) {
            *REG32(&SLCR->MIO_PIN_00 + gpio) |= MIO_PULLUP;
        } else {
            *REG32(&SLCR->MIO_PIN_00 + gpio) &= ~MIO_PULLUP;
        }
    }

    return NO_ERROR;
}

void zynq_gpio_init(void)
{
    /* Enable VREF from GPIOB */
    SLCR->GPIOB_CTRL = 0x1;
}


