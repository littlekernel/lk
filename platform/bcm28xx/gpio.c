/*
 * Copyright (c) 2016 Adam Barth
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <dev/gpio.h>
#include <errno.h>
#include <platform/bcm28xx.h>
#include <lk/reg.h>

#define NUM_PINS     54
#define BITS_PER_REG 32
#define BITS_PER_PIN 3
#define PINS_PER_REG (BITS_PER_REG / BITS_PER_PIN)
#define GPIOREG(base, nr) (REG32(base) + (nr / BITS_PER_REG))

int gpio_config(unsigned nr, unsigned flags) {
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

void gpio_set(unsigned nr, unsigned on) {
    unsigned offset = nr % BITS_PER_REG;
    *GPIOREG(on ? GPIO_GPSET0 : GPIO_GPCLR0, nr) = 1 << offset;
}

int gpio_get(unsigned nr) {
    unsigned offset = nr % BITS_PER_REG;
    return (*GPIOREG(GPIO_GPLEV0, nr) & (1 << offset)) >> offset;
}
