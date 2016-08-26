/*
 * Copyright (c) 2016 Adam Barth
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
#include <dev/gpio.h>
#include <errno.h>
#include <platform/bcm28xx.h>
#include <reg.h>

#define NUM_PINS     54
#define BITS_PER_REG 32
#define BITS_PER_PIN 3
#define PINS_PER_REG (BITS_PER_REG / BITS_PER_PIN)
#define GPIOREG(base, nr) (REG32(base) + (nr / BITS_PER_REG))

int gpio_config(unsigned nr, unsigned flags)
{
    unsigned mask = 0x7;
    if (nr >= NUM_PINS || flags & ~mask)
        return -EINVAL;
    unsigned register_number = nr / PINS_PER_REG;
    unsigned offset = (nr % PINS_PER_REG) * BITS_PER_PIN;
    unsigned shifted_mask = mask << offset;
    volatile uint32_t *reg = REG32(GPIO_GPFSEL0) + register_number;
    *reg = (*reg & ~shifted_mask) | (flags << offset);
    return 0;
}

void gpio_set(unsigned nr, unsigned on)
{
    unsigned offset = nr % BITS_PER_REG;
    *GPIOREG(on ? GPIO_GPSET0 : GPIO_GPCLR0, nr) = 1 << offset;
}

int gpio_get(unsigned nr)
{
    unsigned offset = nr % BITS_PER_REG;
    return (*GPIOREG(GPIO_GPLEV0, nr) & (1 << offset)) >> offset;
}
