/*
 * Copyright (c) 2012 Travis Geiselbrecht
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
#include <target.h>
#include <compiler.h>
#include <dev/gpio.h>
#include <dev/usb.h>
#include <dev/usbc.h>
#include <hw/usb.h>
#include <platform/gpio.h>
#include <stm32f10x_usart.h>
#include <stm32f10x_rcc.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_flash.h>
#include <stm32f10x_dbgmcu.h>
#include <platform/stm32.h>
#include <target/gpioconfig.h>

static void target_init_usb(void);

void target_early_init(void)
{
	/* configure the usart1 pins */
	gpio_config(GPIO(GPIO_PORT_A, 9), GPIO_STM32_AF);
	gpio_config(GPIO(GPIO_PORT_A, 10), GPIO_INPUT);

	stm32_debug_early_init();

	/* configure some status leds */
	gpio_set(GPIO_LED0, 1);

	gpio_config(GPIO_LED0, GPIO_OUTPUT);

	/* usb pins */
	gpio_config(GPIO_USB_PR, GPIO_INPUT);
	gpio_set(GPIO_USB_DISC, 1);
	gpio_config(GPIO_USB_DISC, GPIO_OUTPUT);
}

void target_init(void)
{
	stm32_debug_init();

	target_init_usb();
}

void target_set_debug_led(unsigned int led, bool on)
{
	switch (led) {
		case 0:
			gpio_set(GPIO_LED0, !on);
			break;
	}
}

void target_set_usb_active(bool on)
{
	gpio_set(GPIO_USB_DISC, !on);
}

static uint8_t _dev00[] = {
	18,		/* size */
	DEVICE,
	0x00, 0x02,	/* version */
	0x00,		/* class */
	0x00,		/* subclass */
	0x00,		/* protocol */
	0x40,		/* maxpacket0 */
	0xd1, 0x18,	/* VID */
	0x02, 0x65,	/* PID */
	0x00, 0x01,	/* version */
	0x00,		/* manufacturer string */
	0x00,		/* product string */
	0x00,		/* serialno string */
	0x01,		/* configurations */
};

static uint8_t _cfg00[] = {
	9,
	CONFIGURATION,
	0x20, 0x00,	/* total length */
	0x01,		/* ifc count */
	0x01,		/* configuration value */
	0x00,		/* configuration string */
	0x80,		/* attributes */
	50,		/* mA/2 */

	9,
	INTERFACE,
	0x00,		/* interface number */
	0x00,		/* alt setting */
	0x02,		/* ept count */
	0xFF,		/* class */
	0x00,		/* subclass */
	0x00,		/* protocol */
	0x00,		/* interface string */

	7,
	ENDPOINT,
	0x81,		/* address */
	0x02,		/* bulk */
	0x40, 0x00,	/* max packet size */
	0x00,		/* interval */

	7,
	ENDPOINT,
	0x01,		/* address */
	0x02,		/* bulk */
	0x40, 0x00,	/* max packet size */
	0x00,		/* interval */

};

static uint8_t devqual[] = {
	10,		/* size */
	DEVICE_QUALIFIER,
	0x00, 0x02,	/* version */
	0x00,		/* class */
	0x00,		/* subclass */
	0x00,		/* protocol */
	0x40,		/* maxpacket0 */
	0x01,		/* configurations */
	0x00,		/* reserved */
};

static usb_config uconfig = {
	.lowspeed.device = { _dev00, sizeof(_dev00) },
	.lowspeed.config = { _cfg00, sizeof(_cfg00) },
	.lowspeed.device_qual = { devqual, sizeof(devqual) },
};

static void target_init_usb(void)
{
	usbc_init();

	usb_init();
	usb_setup(&uconfig);
	usb_start();
}

