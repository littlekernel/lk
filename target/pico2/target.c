// Copyright (c) 2020 Brian Swetland
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#include <target.h>
#include <target/debugconfig.h>
#include <hardware/gpio.h>
#include "target_p.h"

static void raw_led_delay(volatile unsigned int count) {
	while (count-- > 0)
		__asm__ volatile("nop");
}

static void raw_led_pulse(unsigned int count) {
	for (unsigned int i = 0; i < count; ++i) {
		gpio_put(DEBUG_LED_GPIO, 1);
		raw_led_delay(800000);
		gpio_put(DEBUG_LED_GPIO, 0);
		raw_led_delay(800000);
	}
}

void target_early_init(void) {
	gpio_init(DEBUG_LED_GPIO);
	gpio_set_dir(DEBUG_LED_GPIO, GPIO_OUT);

	/* Emit a raw pulse train before the rest of LK starts up. */
	raw_led_pulse(3);
	gpio_put(DEBUG_LED_GPIO, 1);
}

void target_set_debug_led(unsigned int led, bool on) {
	if (led != 0)
		return;

	gpio_put(DEBUG_LED_GPIO, on);
}

void target_init(void) {
	target_usb_setup();
}

