/*
 * Copyright (c) 2008 Travis Geiselbrecht
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
#include <arch/arm.h>
#include <lib/console.h>
#include <app/tests.h>
#include <dev/usb.h>

#include "usb_descriptors.h"

static usb_config config = { 
	.lowspeed = {
		.device = { dev_descr, sizeof(dev_descr) },
		.device_qual = { devqual_descr, sizeof(devqual_descr) },
		.config = { cfg_descr_lowspeed, sizeof(cfg_descr_lowspeed) },
	},
	.highspeed = {
		.device = { dev_descr, sizeof(dev_descr) },
		.device_qual = { devqual_descr, sizeof(devqual_descr) },
		.config = { cfg_descr_highspeed, sizeof(cfg_descr_highspeed) },
	},
	.langid = { langid, sizeof(langid) }
};

extern int string_tests(void);
extern int thread_tests(void);

void project_init(void)
{
	console_init();
	tests_init();
	
	usb_init();
	usb_setup(&config);
	usb_add_string("lk, Inc.", 0x1); // manufacturer id
	usb_add_string("lk", 0x2); // device string
	usb_start();

	console_start();
}

