/*
 * Copyright (c) 2016 Brian Swetland
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <lk/debug.h>
#include <platform.h>
#include <arch/arm/cm.h>

#include <driverlib/prcm.h>
#include <driverlib/ioc.h>
#include <driverlib/uart.h>
#include <driverlib/osc.h>

// if GPIO_UART_?R are defined, route UART there
#include <target/gpioconfig.h>

void trimDevice(void);

#define UART0_BASE 0x40001000

void platform_early_init(void) {
    trimDevice();

    PRCMPowerDomainOn(PRCM_DOMAIN_SERIAL);
    while (PRCMPowerDomainStatus(PRCM_DOMAIN_SERIAL) != PRCM_DOMAIN_POWER_ON) ;

    PRCMPowerDomainOn(PRCM_DOMAIN_PERIPH);
    while (PRCMPowerDomainStatus(PRCM_DOMAIN_PERIPH) != PRCM_DOMAIN_POWER_ON) ;

    PRCMPeripheralRunEnable(PRCM_PERIPH_UART0);
    PRCMPeripheralRunEnable(PRCM_PERIPH_GPIO);
    PRCMLoadSet();

#ifdef GPIO_UART_TX
    IOCPortConfigureSet(GPIO_UART_TX, IOC_PORT_MCU_UART0_TX, 0);
#endif
#ifdef GPIO_UART_RX
    IOCPortConfigureSet(GPIO_UART_RX, IOC_PORT_MCU_UART0_RX, IOC_INPUT_ENABLE);
#endif

    UARTConfigSetExpClk(UART0_BASE, 48000000, 115200,
                        UART_CONFIG_WLEN_8 | UART_CONFIG_PAR_NONE | UART_CONFIG_STOP_ONE);
    UARTEnable(UART0_BASE);

    arm_cm_systick_init(48000000);

    // switch to 24MHz XOSC
    OSCInterfaceEnable();
    OSCClockSourceSet(OSC_SRC_CLK_HF, OSC_XOSC_HF);
    OSCHfSourceSwitch();

#if 0
    dprintf(INFO, "hf clk src %d\n", OSCClockSourceGet(OSC_SRC_CLK_HF));
    dprintf(INFO, "mf clk src %d\n", OSCClockSourceGet(OSC_SRC_CLK_MF));
    dprintf(INFO, "lf clk src %d\n", OSCClockSourceGet(OSC_SRC_CLK_LF));
#endif
}

void platform_init(void) {
}

