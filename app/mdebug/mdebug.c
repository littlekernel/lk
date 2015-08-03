/* mdebug.c
 *
 * Copyright 2015 Brian Swetland <swetland@frotz.net>
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <app.h>
#include <debug.h>
#include <string.h>
#include <stdlib.h>
#include <printf.h>
#include <dev/udc.h>

#include <platform.h>
#include <arch/arm.h>
#include <kernel/thread.h>
#include <kernel/event.h>

#include <platform/lpc43xx-gpio.h>

#include "swd.h"

#define TX_AHEAD 1

static event_t txevt = EVENT_INITIAL_VALUE(txevt, TX_AHEAD, 0);
static event_t rxevt = EVENT_INITIAL_VALUE(rxevt, 0, 0);

static udc_request_t *txreq;
static udc_request_t *rxreq;
static udc_endpoint_t *txept;
static udc_endpoint_t *rxept;

static volatile int online;
static volatile int txstatus;
static volatile int rxstatus;
static volatile unsigned rxactual;

static void mdebug_notify(udc_gadget_t *gadget, unsigned event) {
	if (event == UDC_EVENT_ONLINE) {
		online = 1;
	} else {
		online = 0;
	}
}

static void rx_complete(udc_request_t *req, unsigned actual, int status) {
	rxactual = actual;
	rxstatus = status;
	event_signal(&rxevt, 0);
}

static void tx_complete(udc_request_t *req, unsigned actual, int status) {
	txstatus = status;
	event_signal(&txevt, 0);
}

#if TX_AHEAD
void usb_xmit(void *data, unsigned len) {
//printf(">%d>\n", len);
	event_wait(&txevt);
	event_unsignal(&txevt);
	txreq->buffer = data;
	txreq->length = len;
	txstatus = 1;
	udc_request_queue(txept, txreq);
//printf(">%d>QUEUED\n", len);
}
#else
void usb_xmit(void *data, unsigned len) {
//printf(">%d>\n", len);
	event_unsignal(&txevt);
	txreq->buffer = data;
	txreq->length = len;
	txstatus = 1;
	udc_request_queue(txept, txreq);
	event_wait(&txevt);
//printf(">%d>%s\n", len, txstatus ? "ERR" : "");
}
#endif

unsigned usb_recv(void *data, unsigned len) {
//printf("<%d<\n", len);
	event_unsignal(&rxevt);
	rxreq->buffer = data;
	rxreq->length = len;
	rxstatus = 1;
	udc_request_queue(rxept, rxreq);
	event_wait(&rxevt);
#if 0
	if (rxstatus) {
		printf("<%d<ERR\n", len);
	} else {
		printf("<%d<%d\n", len, rxactual);
	}
#endif
	return rxactual;
}
	
static udc_device_t mdebug_device = {
	.vendor_id = 0x18d1,
	.product_id = 0xdb03,
	.version_id = 0x0100,
};

static udc_endpoint_t *mdebug_endpoints[2];

static udc_gadget_t mdebug_gadget = {
	.notify = mdebug_notify,
	.ifc_class = 0xFF,
	.ifc_subclass = 0xFF,
	.ifc_protocol = 0xFF,
	.ifc_endpoints = 2,
	.ept = mdebug_endpoints,
};

static void mdebug_init(const struct app_descriptor *app)
{
	swd_init();

	udc_init(&mdebug_device);
	mdebug_endpoints[0] = txept = udc_endpoint_alloc(UDC_BULK_IN, 512);
	mdebug_endpoints[1] = rxept = udc_endpoint_alloc(UDC_BULK_OUT, 512);
	txreq = udc_request_alloc();
	rxreq = udc_request_alloc();
	rxreq->complete = rx_complete;
	txreq->complete = tx_complete;
	udc_register_gadget(&mdebug_gadget);
}

void handle_rswd(void);

static void mdebug_entry(const struct app_descriptor *app, void *args)
{
	udc_start();

	for (;;) {
		if (!online) {
			thread_yield();
			continue;
		}
		handle_rswd();
	}
}

APP_START(usbtest)
	.init = mdebug_init,
	.entry = mdebug_entry,
APP_END


