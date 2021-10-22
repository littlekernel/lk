/*
 * Copyright (c) 2020 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/err.h>
#include <lk/reg.h>
#include <lk/trace.h>
#include <kernel/thread.h>
#include <platform.h>
#include <platform/interrupts.h>
#include <platform/debug.h>
#include <platform/sifive.h>
#include <sys/types.h>
#include <dev/gpio.h>

#include "platform_p.h"

static volatile unsigned int *const gpio_base = (unsigned int *)GPIO_BASE;

#define GPIO_REG_VALUE      0
#define GPIO_REG_INPUT_EN   1
#define GPIO_REG_OUTPUT_EN  2
#define GPIO_REG_PORT       3
#define GPIO_REG_PUE        4
#define GPIO_REG_DS         5
#define GPIO_REG_RISE_IE    6
#define GPIO_REG_RISE_IP    7
#define GPIO_REG_FALL_IE    8
#define GPIO_REG_FALL_IP    9
#define GPIO_REG_HIGH_IE    10
#define GPIO_REG_HIGH_IP    11
#define GPIO_REG_LOW_IE     12
#define GPIO_REG_LOW_IP     13
#define GPIO_REG_IOF_EN     14
#define GPIO_REG_IOF_SEL    15

void gpio_early_init(void) {}
void gpio_init(void) {}

__ALWAYS_INLINE
static inline void gpio_reg_bit_set(unsigned int reg, unsigned nr, bool set) {
    if (set) {
        __atomic_fetch_or(&gpio_base[reg], (1U << nr), __ATOMIC_RELAXED);
    } else {
        __atomic_fetch_and(&gpio_base[reg], ~(1U << nr), __ATOMIC_RELAXED);
    }
}

int gpio_config(unsigned nr, unsigned flags) {
    if (nr >= 32) {
        return ERR_INVALID_ARGS;
    }

#if PLATFORM_SIFIVE_E
    // the alternate function feature only exists on the embedded variant
    if (flags & GPIO_AF0) { // alternate function 0
        gpio_reg_bit_set(GPIO_REG_IOF_SEL, nr, 0);
        gpio_reg_bit_set(GPIO_REG_IOF_EN, nr, 1);
    } else if (flags & GPIO_AF0) { // alternate function 1
        gpio_reg_bit_set(GPIO_REG_IOF_SEL, nr, 1);
        gpio_reg_bit_set(GPIO_REG_IOF_EN, nr, 1);
    } else
#endif // PLATFORM_SIFIVE_E
    {
        if (flags & GPIO_INPUT) {
            gpio_reg_bit_set(GPIO_REG_INPUT_EN, nr, 1);
        }
        if (flags & GPIO_OUTPUT) {
            gpio_reg_bit_set(GPIO_REG_OUTPUT_EN, nr, 1);
        }
        if (flags & GPIO_PULLUP) {
            gpio_reg_bit_set(GPIO_REG_PUE, nr, 1);
        } else {
            gpio_reg_bit_set(GPIO_REG_PUE, nr, 0);
        }

#if PLATFORM_SIFIVE_E
        // clear the alternate function
        gpio_reg_bit_set(GPIO_REG_IOF_EN, nr, 0);
#endif // PLATFORM_SIFIVE_E
    }

    return 0;
}

void gpio_set(unsigned nr, unsigned on) {
    if (nr >= 32) {
        return;
    }

    // set and clear the LED gpios using atomic instructions
    // polarity is inverted
    if (on) {
        gpio_reg_bit_set(GPIO_REG_PORT, nr, 0);
    } else {
        gpio_reg_bit_set(GPIO_REG_PORT, nr, 1);
    }
}

int gpio_get(unsigned nr) {
    if (nr >= 32) {
        return ERR_INVALID_ARGS;
    }

    unsigned int val = gpio_base[GPIO_REG_VALUE] & (1U << nr);
    return val ? 1 : 0;
}

