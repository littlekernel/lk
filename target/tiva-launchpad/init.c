/*
 * Copyright (c) 2012 Ian McKellar
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/err.h>
#include <lk/debug.h>
#include <target.h>
#include <lk/compiler.h>
#include <dev/gpio.h>
#include <platform/gpio.h>

#include "ti_driverlib.h"

extern void target_usb_setup(void);

void target_early_init(void) {
    GPIOPinWrite(GPIO_PORTF_AHB_BASE, GPIO_PIN_1, 0);
    GPIOPinWrite(GPIO_PORTF_AHB_BASE, GPIO_PIN_2, 0);
    GPIOPinWrite(GPIO_PORTF_AHB_BASE, GPIO_PIN_3, 0);

    GPIOPadConfigSet(GPIO_PORTF_AHB_BASE, GPIO_PIN_1, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD);
    GPIOPadConfigSet(GPIO_PORTF_AHB_BASE, GPIO_PIN_2, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD);
    GPIOPadConfigSet(GPIO_PORTF_AHB_BASE, GPIO_PIN_3, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD);
    GPIODirModeSet(GPIO_PORTF_AHB_BASE, GPIO_PIN_1, GPIO_DIR_MODE_OUT);
    GPIODirModeSet(GPIO_PORTF_AHB_BASE, GPIO_PIN_2, GPIO_DIR_MODE_OUT);
    GPIODirModeSet(GPIO_PORTF_AHB_BASE, GPIO_PIN_3, GPIO_DIR_MODE_OUT);
}

void target_init(void) {
    target_usb_setup();
}

void target_set_debug_led(unsigned int led, bool on) {
    switch (led) {
        case 0:
            gpio_set(GPIO(GPIO_PORT_F, 1), on);
            break;
        case 1:
            gpio_set(GPIO(GPIO_PORT_F, 2), on);
            break;
        case 2:
            gpio_set(GPIO(GPIO_PORT_F, 3), on);
            break;
    }
}
