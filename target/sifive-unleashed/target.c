/*
 * Copyright (c) 2019 Elliot Berman
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <target.h>
#include <arch/arch_ops.h>
#include <platform/sifive.h>

// NOTE: set to 0 if trying to boot on qemu
#define ENABLE_DEBUG_LED 1

static volatile struct {
    volatile uint32_t pwmcfg;
    volatile uint32_t res0;
    volatile uint32_t pwmcount;
    volatile uint32_t res1;
    volatile uint32_t pwms;
    volatile uint32_t res2[3];
    volatile uint32_t pwmcmp[4];
} *const pwm0_base = (void*)PWM0_BASE;

void target_early_init(void) {
    if (ENABLE_DEBUG_LED) {
        pwm0_base->pwmcfg = 0x100f; // enable always and max scaling
        target_set_debug_led(0, false);
        target_set_debug_led(1, false);
        target_set_debug_led(2, false);
        target_set_debug_led(3, false);
    }
}

void target_init(void) {
}

void target_set_debug_led(unsigned int led, bool on) {
    if (ENABLE_DEBUG_LED) {
        if(led > 3)
            return;
        pwm0_base->pwmcmp[led] = (0xffff + on) & 0xffff;
    }
}
