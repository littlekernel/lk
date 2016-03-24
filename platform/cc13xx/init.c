/*
 * Copyright (c) 2016 Brian Swetland
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

#include <debug.h>
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

