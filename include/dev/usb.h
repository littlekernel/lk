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
#ifndef __DEV_USB_H
#define __DEV_USB_H

#include <sys/types.h>
#include <compiler.h>

/* top level initialization for usb client, abstracts away the interfaces */
typedef struct {
	void *desc;
	size_t len;
} usb_descriptor __ALIGNED(2);

typedef struct {
	usb_descriptor string;
	uint8_t id;
} usb_string;

/* complete usb config struct, passed in to usb_setup() */
typedef struct {
	struct usb_descriptor_speed {
		usb_descriptor device;
		usb_descriptor device_qual;
		usb_descriptor config;
	} lowspeed, highspeed;
	usb_descriptor langid;
} usb_config;

void usb_init(void);

/* external code needs to set up the usb stack via the following calls */
void usb_setup(usb_config *config);

/* apped new interface descriptors to the existing config if desired */
int usb_append_interface_highspeed(const uint8_t *int_descr, size_t len);
int usb_append_interface_lowspeed(const uint8_t *int_descr, size_t len);

void usb_add_string(const char *string, uint8_t id);

void usb_start(void);
void usb_stop(void);

#endif

