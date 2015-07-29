/*
 * Copyright (c) 2015 Brian Swetland
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

#include <app.h>
#include <debug.h>
#include <string.h>
#include <stdlib.h>
#include <printf.h>
#include <dev/udc.h>

#include <platform.h>
#include <arch/arm.h>
#include <kernel/thread.h>

void spifi_init(void);
void spifi_page_program(u32 addr, u32 *ptr, u32 count);
void spifi_sector_erase(u32 addr);
int spifi_verify_erased(u32 addr, u32 count);
int spifi_verify_page(u32 addr, u32 *ptr);

static udc_request_t *txreq;
static udc_request_t *rxreq;
static udc_endpoint_t *txept;
static udc_endpoint_t *rxept;

static volatile int online;
static volatile int txstatus;
static volatile int rxstatus;
static volatile unsigned rxactual;

static void lpcboot_notify(udc_gadget_t *gadget, unsigned event) {
	if (event == UDC_EVENT_ONLINE) {
		online = 1;
	} else {
		online = 0;
	}
}

static void rx_complete(udc_request_t *req, unsigned actual, int status) {
	rxactual = actual;
	rxstatus = status;
}

static void tx_complete(udc_request_t *req, unsigned actual, int status) {
	txstatus = status;	
}

void usb_xmit(void *data, unsigned len) {
	txreq->buffer = data;
	txreq->length = len;
	txstatus = 1;
	udc_request_queue(txept, txreq);
	while (txstatus == 1) thread_yield();
}

unsigned usb_recv(void *data, unsigned len) {
	rxreq->buffer = data;
	rxreq->length = len;
	rxstatus = 1;
	udc_request_queue(rxept, rxreq);
	while (rxstatus == 1) thread_yield();
	return rxactual;
}
	
static udc_device_t lpcboot_device = {
	.vendor_id = 0x18d1,
	.product_id = 0xdb00,
	.version_id = 0x0100,
};

static udc_endpoint_t *lpcboot_endpoints[2];

static udc_gadget_t lpcboot_gadget = {
	.notify = lpcboot_notify,
	.ifc_class = 0xFF,
	.ifc_subclass = 0xFF,
	.ifc_protocol = 0xFF,
	.ifc_endpoints = 2,
	.ept = lpcboot_endpoints,
};
	
static void lpcboot_init(const struct app_descriptor *app)
{
	udc_init(&lpcboot_device);
	lpcboot_endpoints[0] = txept = udc_endpoint_alloc(UDC_BULK_IN, 512);
	lpcboot_endpoints[1] = rxept = udc_endpoint_alloc(UDC_BULK_OUT, 512);
	txreq = udc_request_alloc();
	rxreq = udc_request_alloc();
	rxreq->complete = rx_complete;
	txreq->complete = tx_complete;
	udc_register_gadget(&lpcboot_gadget);
}

#define RAM_BASE	0x10000000
#define RAM_SIZE	(32 * 1024)

#define ROM_BASE	0x00000000
#define ROM_SIZE	(1024 * 1024)

struct device_info {
	u8 part[16];
	u8 board[16];
	u32 version;
	u32 ram_base;
	u32 ram_size;
	u32 rom_base;
	u32 rom_size;
	u32 unused0;
	u32 unused1;
	u32 unused2;
};

struct device_info DEVICE = {
	.part = "LPC43xx",
	.board = TARGET,
	.version = 0x0001000,
	.ram_base = RAM_BASE,
	.ram_size = RAM_SIZE,
	.rom_base = ROM_BASE,
	.rom_size = ROM_SIZE,
};


int erase_page(u32 addr) {
	spifi_sector_erase(addr);
	return spifi_verify_erased(addr, 0x1000/4);
}

int write_page(u32 addr, void *ptr) {
	unsigned n;
	u32 *x = ptr;
	for (n = 0; n < 16; n++) {
		spifi_page_program(addr, x, 256 / 4);
		if (spifi_verify_page(addr, x)) return -1;
		addr += 256;
		x += (256 / 4);
	}
	return 0;
} 

static uint32_t ram[4096/4];

void handle(u32 magic, u32 cmd, u32 arg) {
	u32 reply[2];
	u32 addr, xfer;
	int err = 0;

	if (magic != 0xDB00A5A5)
		return;

	reply[0] = magic;
	reply[1] = -1;

	switch (cmd) {
	case 'E':
		reply[1] = erase_page(0);
		break;
	case 'W':
		if (arg > ROM_SIZE)
			break;
		reply[1] = 0;
		usb_xmit(reply, 8);
		addr = ROM_BASE;
		while (arg > 0) {
			xfer = (arg > 4096) ? 4096 : arg;
			usb_recv(ram, xfer);
			if (!err) err = erase_page(addr);
			if (!err) err = write_page(addr, ram);
			addr += 4096;
			arg -= xfer;
		}
		printf("flash %s\n", err ? "ERROR" : "OK");
		reply[1] = err;
		break;
#if WITH_BOOT_TO_RAM 
	case 'X':
		if (arg > RAM_SIZE)
			break;
		reply[1] = 0;
		usb_xmit(reply, 8);
		usb_recv(ram, arg);
		usb_xmit(reply, 8);

		/* let last txn clear */
		usb_recv_timeout(buf, 64, 10);

		boot_image(ram);
		break;
#endif
	case 'Q':
		reply[1] = 0;
		usb_xmit(reply, 8);
		usb_xmit(&DEVICE, sizeof(DEVICE));
		return;
#if WITH_BOOT_TO_APP
	case 'A':
		// reboot-into-app
#endif
	case 'R':
		/* reboot "normally" */
		reply[1] = 0;
		usb_xmit(reply, 8);
		udc_stop();
		platform_halt(HALT_ACTION_REBOOT, HALT_REASON_SW_RESET);
	default:
		break;
	}
	usb_xmit(reply, 8);
}


static void lpcboot_entry(const struct app_descriptor *app, void *args)
{
	u32 buf[64/4];
	udc_start();
	spifi_init();
	for (;;) {
		if (!online) {
			thread_yield();
			continue;
		}
		if (usb_recv(buf, 64) == 12) {
			handle(buf[0], buf[1], buf[2]);
		}
	}
}

APP_START(usbtest)
	.init = lpcboot_init,
	.entry = lpcboot_entry,
APP_END


