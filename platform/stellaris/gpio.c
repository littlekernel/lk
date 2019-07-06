/*
 * Copyright (c) 2012 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/debug.h>
#include <assert.h>
#include <dev/gpio.h>
#include <platform/gpio.h>
#include "ti_driverlib.h"

static void *port_to_pointer(unsigned int port) {
    switch (port) {
        default:
        case GPIO_PORT_A:
            return (void *)GPIO_PORTA_AHB_BASE;
        case GPIO_PORT_B:
            return (void *)GPIO_PORTB_AHB_BASE;
        case GPIO_PORT_C:
            return (void *)GPIO_PORTC_AHB_BASE;
        case GPIO_PORT_D:
            return (void *)GPIO_PORTD_AHB_BASE;
        case GPIO_PORT_E:
            return (void *)GPIO_PORTE_AHB_BASE;
        case GPIO_PORT_F:
            return (void *)GPIO_PORTF_AHB_BASE;
        case GPIO_PORT_G:
            return (void *)GPIO_PORTG_AHB_BASE;
        case GPIO_PORT_H:
            return (void *)GPIO_PORTH_AHB_BASE;
        case GPIO_PORT_J:
            return (void *)GPIO_PORTJ_BASE;
        case GPIO_PORT_K:
            return (void *)GPIO_PORTK_BASE;
        case GPIO_PORT_L:
            return (void *)GPIO_PORTL_BASE;
        case GPIO_PORT_M:
            return (void *)GPIO_PORTM_BASE;
        case GPIO_PORT_N:
            return (void *)GPIO_PORTN_BASE;
        case GPIO_PORT_P:
            return (void *)GPIO_PORTP_BASE;
        case GPIO_PORT_Q:
            return (void *)GPIO_PORTQ_BASE;
    }
}

void stellaris_gpio_early_init(void) {
    /* Disable hitting the AHB bits on this target, which
     * is probably qemu emulated. QEMU does not implement
     * these registers and will crash.
     */
#if !TARGET_LM3S6965EVB
    SysCtlGPIOAHBEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlGPIOAHBEnable(SYSCTL_PERIPH_GPIOB);
    SysCtlGPIOAHBEnable(SYSCTL_PERIPH_GPIOC);
    SysCtlGPIOAHBEnable(SYSCTL_PERIPH_GPIOD);
    SysCtlGPIOAHBEnable(SYSCTL_PERIPH_GPIOE);
    SysCtlGPIOAHBEnable(SYSCTL_PERIPH_GPIOF);
    SysCtlGPIOAHBEnable(SYSCTL_PERIPH_GPIOG);
    SysCtlGPIOAHBEnable(SYSCTL_PERIPH_GPIOH);
#endif

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOH);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ);
}

void stellaris_gpio_init(void) {
}

#if 0
int gpio_config(unsigned nr, unsigned flags) {
    uint port = GPIO_PORT(nr);
    uint pin = GPIO_PIN(nr);

    enable_port(port);

    GPIO_InitTypeDef init;
    init.GPIO_Speed = GPIO_Speed_50MHz;

    init.GPIO_Pin = (1 << pin);

    if (flags & GPIO_STM32_AF) {
        if (flags & GPIO_STM32_OD)
            init.GPIO_Mode = GPIO_Mode_Out_OD;
        else
            init.GPIO_Mode = GPIO_Mode_AF_PP;
    } else if (flags & GPIO_OUTPUT) {
        if (flags & GPIO_STM32_OD)
            init.GPIO_Mode = GPIO_Mode_Out_OD;
        else
            init.GPIO_Mode = GPIO_Mode_Out_PP;
    } else { // GPIO_INPUT
        if (flags & GPIO_PULLUP) {
            init.GPIO_Mode = GPIO_Mode_IPU;
        } else if (flags & GPIO_PULLDOWN) {
            init.GPIO_Mode = GPIO_Mode_IPD;
        } else {
            init.GPIO_Mode = GPIO_Mode_IN_FLOATING;
        }
    }

    GPIO_Init(port_to_pointer(port), &init);

    return 0;
}
#endif

void gpio_set(unsigned nr, unsigned on) {
    GPIOPinWrite((unsigned int)port_to_pointer(GPIO_PORT(nr)), 1 << GPIO_PIN(nr), on ? (1 << GPIO_PIN(nr)) : 0);
}

int gpio_get(unsigned nr) {
    return GPIOPinRead((unsigned int)port_to_pointer(GPIO_PORT(nr)), 1 << GPIO_PIN(nr));
}


