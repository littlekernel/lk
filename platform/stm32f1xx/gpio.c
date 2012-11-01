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
#include <debug.h>
#include <assert.h>
#include <dev/gpio.h>
#include <platform/stm32.h>
#include <platform/gpio.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_rcc.h>

static GPIO_TypeDef *port_to_pointer(unsigned int port)
{
	switch (port) {
		default:
		case GPIO_PORT_A:
			return GPIOA;
		case GPIO_PORT_B:
			return GPIOB;
		case GPIO_PORT_C:
			return GPIOC;
		case GPIO_PORT_D:
			return GPIOD;
		case GPIO_PORT_E:
			return GPIOE;
		case GPIO_PORT_F:
			return GPIOF;
		case GPIO_PORT_G:
			return GPIOG;
	}
}

static void enable_port(unsigned int port)
{
	DEBUG_ASSERT(port <= GPIO_PORT_G);

	/* happens to be the RCC ids are sequential bits, so we can start from A and shift */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA << port, ENABLE);
}

void stm32_gpio_early_init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
}

int gpio_config(unsigned nr, unsigned flags)
{
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

void gpio_set(unsigned nr, unsigned on)
{
	GPIO_WriteBit(port_to_pointer(GPIO_PORT(nr)), 1 << GPIO_PIN(nr), on);
}

int gpio_get(unsigned nr)
{
	return GPIO_ReadInputDataBit(port_to_pointer(GPIO_PORT(nr)), 1 << GPIO_PIN(nr));
}

