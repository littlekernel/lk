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
#include <stm32f2xx_usart.h>
#include <stm32f2xx_rcc.h>
#include <stm32f2xx_gpio.h>
//#include <stm32f10x_flash.h>
//#include <stm32f10x_dbgmcu.h>
#include <platform/stm32.h>
#include <platform/gpio.h>
#include <target/gpioconfig.h>

void target_early_init(void)
{
	/* configure the usart3 pins */
	gpio_config(GPIO_USART3_TX, GPIO_STM32_AF | GPIO_STM32_AFn(GPIO_AF_USART3) | GPIO_PULLUP);
	gpio_config(GPIO_USART3_RX, GPIO_STM32_AF | GPIO_STM32_AFn(GPIO_AF_USART3) | GPIO_PULLUP);

	stm32_debug_early_init();

	/* configure some status leds */
	gpio_config(GPIO_LED0, GPIO_OUTPUT);
	gpio_config(GPIO_LED1, GPIO_OUTPUT);
	gpio_config(GPIO_LED2, GPIO_OUTPUT);
	gpio_config(GPIO_LED3, GPIO_OUTPUT);

	stm3220g_set_led_bits(1);
}

void target_init(void)
{
	TRACE_ENTRY;

	stm32_debug_init();

	TRACE_EXIT;
}

void target_set_debug_led(unsigned int led, bool on)
{
	switch (led) {
		case 0:
			gpio_set(GPIO_LED0, on);
			break;
		case 1:
			gpio_set(GPIO_LED1, on);
			break;
		case 2:
			gpio_set(GPIO_LED2, on);
			break;
		case 3:
			gpio_set(GPIO_LED3, on);
			break;
	}
}

void stm3220g_set_led_bits(unsigned int nr)
{
	gpio_set(GPIO_LED0, nr & 1);
	gpio_set(GPIO_LED1, nr & 2);
	gpio_set(GPIO_LED2, nr & 4);
	gpio_set(GPIO_LED3, nr & 8);
}
