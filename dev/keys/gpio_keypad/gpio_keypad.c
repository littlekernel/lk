/*
 * Copyright (c) 2009, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Google, Inc. nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <assert.h>
#include <bits.h>
#include <stdlib.h>
#include <string.h>
#include <dev/keys.h>
#include <dev/gpio.h>
#include <dev/gpio_keypad.h>
#include <kernel/event.h>
#include <kernel/timer.h>

struct gpio_kp {
	struct gpio_keypad_info *keypad_info;
	struct timer timer;
	event_t full_scan;
	int current_output;
	unsigned int some_keys_pressed:2;
	unsigned long keys_pressed[0];
};

/* TODO: Support multiple keypads? */
static struct gpio_kp *keypad;

static void check_output(struct gpio_kp *kp, int out, int polarity)
{
	struct gpio_keypad_info *kpinfo = kp->keypad_info;
	int key_index;
	int in;
	int gpio;
	int changed = 0;

	key_index = out * kpinfo->ninputs;
	for (in = 0; in < kpinfo->ninputs; in++, key_index++) {
		gpio = kpinfo->input_gpios[in];
		changed = 0;
		if (gpio_get(gpio) ^ !polarity) {
			if (kp->some_keys_pressed < 3)
				kp->some_keys_pressed++;
			changed = !bitmap_set(kp->keys_pressed, key_index);
		} else {
			changed = bitmap_clear(kp->keys_pressed, key_index);
		}
		if (changed) {
			int state = bitmap_test(kp->keys_pressed, key_index);
			keys_post_event(kpinfo->keymap[key_index], state);
		}
	}

	/* sets up the right state for the next poll cycle */
	gpio = kpinfo->output_gpios[out];
	if (kpinfo->flags & GPIOKPF_DRIVE_INACTIVE)
		gpio_set(gpio, !polarity);
	else
		gpio_config(gpio, GPIO_INPUT);
}

static enum handler_return
gpio_keypad_timer_func(struct timer *timer, time_t now, void *arg)
{
	struct gpio_kp *kp = keypad;
	struct gpio_keypad_info *kpinfo = kp->keypad_info;
	int polarity = !!(kpinfo->flags & GPIOKPF_ACTIVE_HIGH);
	int out;
	int gpio;

	out = kp->current_output;
	if (out == kpinfo->noutputs) {
		out = 0;
		kp->some_keys_pressed = 0;
	} else {
		check_output(kp, out, polarity);
		out++;
	}

	kp->current_output = out;
	if (out < kpinfo->noutputs) {
		gpio = kpinfo->output_gpios[out];
		if (kpinfo->flags & GPIOKPF_DRIVE_INACTIVE)
			gpio_set(gpio, polarity);
		else
			gpio_config(gpio, polarity ? GPIO_OUTPUT : 0);
		timer_set_oneshot(timer, kpinfo->settle_time,
		                  gpio_keypad_timer_func, NULL);
		goto done;
	}

	if (/*!kp->use_irq*/ 1 || kp->some_keys_pressed) {
		event_signal(&kp->full_scan, false);
		timer_set_oneshot(timer, kpinfo->poll_time,
		                  gpio_keypad_timer_func, NULL);
		goto done;
	}

#if 0
	/* No keys are pressed, reenable interrupt */
	for (out = 0; out < kpinfo->noutputs; out++) {
		if (gpio_keypad_flags & GPIOKPF_DRIVE_INACTIVE)
			gpio_set(kpinfo->output_gpios[out], polarity);
		else
			gpio_config(kpinfo->output_gpios[out], polarity ? GPIO_OUTPUT : 0);
	}
	for (in = 0; in < kpinfo->ninputs; in++)
		enable_irq(gpio_to_irq(kpinfo->input_gpios[in]));
	return INT_RESCHEDULE;
#endif

done:
	return INT_RESCHEDULE;
}

void gpio_keypad_init(struct gpio_keypad_info *kpinfo)
{
	int key_count;
	int output_val;
	int output_cfg;
	int i;
	int len;

	ASSERT(kpinfo->keymap && kpinfo->input_gpios && kpinfo->output_gpios);
	key_count = kpinfo->ninputs * kpinfo->noutputs;

	len = sizeof(struct gpio_kp) + (sizeof(unsigned long) *
	                                BITMAP_NUM_WORDS(key_count));
	keypad = malloc(len);
	ASSERT(keypad);

	memset(keypad, 0, len);
	keypad->keypad_info = kpinfo;

	output_val = (!!(kpinfo->flags & GPIOKPF_ACTIVE_HIGH)) ^
	             (!!(kpinfo->flags & GPIOKPF_DRIVE_INACTIVE));
	output_cfg = kpinfo->flags & GPIOKPF_DRIVE_INACTIVE ? GPIO_OUTPUT : 0;
	for (i = 0; i < kpinfo->noutputs; i++) {
		gpio_set(kpinfo->output_gpios[i], output_val);
		gpio_config(kpinfo->output_gpios[i], output_cfg);
	}
	for (i = 0; i < kpinfo->ninputs; i++)
		gpio_config(kpinfo->input_gpios[i], GPIO_INPUT);

	keypad->current_output = kpinfo->noutputs;

	event_init(&keypad->full_scan, false, EVENT_FLAG_AUTOUNSIGNAL);
	timer_initialize(&keypad->timer);
	timer_set_oneshot(&keypad->timer, 0, gpio_keypad_timer_func, NULL);

	/* wait for the keypad to complete one full scan */
	event_wait(&keypad->full_scan);
}
