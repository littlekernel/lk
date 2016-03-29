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

#include <dev/gpio.h>

#include <driverlib/gpio.h>
#include <driverlib/ioc.h>

int gpio_config(unsigned nr, unsigned flags) {
	if (flags & GPIO_INPUT) {
		IOCPortConfigureSet(nr, IOC_PORT_GPIO, IOC_INPUT_ENABLE);
	} else {
		IOCPortConfigureSet(nr, IOC_PORT_GPIO, 0);
	}
	if (flags & GPIO_OUTPUT) {
		GPIO_setOutputEnableDio(nr, GPIO_OUTPUT_ENABLE);
	} else {
		GPIO_setOutputEnableDio(nr, GPIO_OUTPUT_DISABLE);
	}
	return 0;
}

void gpio_set(unsigned nr, unsigned on) {
	GPIO_writeDio(nr, on);
}

int gpio_get(unsigned nr) {
	return GPIO_readDio(nr);
}
