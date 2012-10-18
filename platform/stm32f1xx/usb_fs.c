/* usb.c
 *
 * Copyright 2011 Brian Swetland <swetland@frotz.net>
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
/*
 * Portions copyright 2012 Travis Geiselbrecht <geist@foobox.com>
 */

#include <sys/types.h>
#include <debug.h>
#include <err.h>
#include <kernel/thread.h>

#include <dev/usb.h>
#include <dev/usbc.h>
#include <hw/usb.h>

#include <dev/gpio.h>
#include <platform/gpio.h>
#include <target/gpioconfig.h>

#include <stm32f10x.h>
#include <stm32f10x_rcc.h>

//#include <fw/lib.h>
//#include <fw/io.h>

//#include <arch/hardware.h>
//#include <protocol/usb.h>

/* from m3dev */
#define printx printf

#define LOCAL_TRACE 0

#define USB_SRAM_BASE (APB1PERIPH_BASE + 0x6000)
#define USB_BASE      (APB1PERIPH_BASE + 0x5C00)
#define USB_EPR(n)		(USB_BASE + (n * 4))
#define USB_CR			(USB_BASE + 0x40)
#define USB_ISR			(USB_BASE + 0x44)
#define USB_FNR			(USB_BASE + 0x48)
#define USB_DADDR		(USB_BASE + 0x4C)
#define USB_BTABLE		(USB_BASE + 0x50)

/* the *M bits apply to both CR (to enable) and ISR (to read) */
#define USB_CTRM		(1 << 15)
#define USB_PMAOVRM		(1 << 14)
#define USB_ERRM		(1 << 13)
#define USB_WKUPM		(1 << 12)
#define USB_SUSPM		(1 << 11)
#define USB_RESETM		(1 << 10)
#define USB_SOFM		(1 << 9)
#define USB_ESOFM		(1 << 8)

#define USB_CR_RESUME		(1 << 4)
#define USB_CR_FSUSP		(1 << 3)
#define USB_CR_LP_MODE		(1 << 2)
#define USB_CR_PDWN		(1 << 1)
#define USB_CR_FRES		(1 << 0)

#define USB_ISR_DIR		(1 << 4)
#define USB_ISR_EP_MASK		0xF

#define USB_DADDR_ENABLE	(1 << 7)

#define USB_EPR_CTR_RX		(1 << 15) // R+W0C
#define USB_EPR_DTOG_RX		(1 << 14) // T
#define USB_EPR_RX_DISABLE	(0 << 12) // T
#define USB_EPR_RX_STALL	(1 << 12) // T
#define USB_EPR_RX_NAK		(2 << 12) // T
#define USB_EPR_RX_VALID	(3 << 12) // T
#define USB_EPR_SETUP		(1 << 11) // RO
#define USB_EPR_TYPE_BULK	(0 << 9)  // RW
#define USB_EPR_TYPE_CONTROL	(1 << 9)  // RW
#define USB_EPR_TYPE_ISO	(2 << 9)  // RW
#define USB_EPR_TYPE_INTERRRUPT	(3 << 9)  // RW
#define USB_EPR_TYPE_MASK	(3 << 9)
#define USB_EPR_DBL_BUF		(1 << 8)  // RW (for BULK)
#define USB_EPR_STATUS_OUT	(1 << 8)  // RW (for CONTROL)
#define USB_EPR_CTR_TX		(1 << 7)  // R+W0C
#define USB_EPR_DTOG_TX		(1 << 6)  // T
#define USB_EPR_TX_DISABLED	(0 << 4)  // T
#define USB_EPR_TX_STALL	(1 << 4)  // T
#define USB_EPR_TX_NAK		(2 << 4)  // T
#define USB_EPR_TX_VALID	(3 << 4)  // T
#define USB_EPR_ADDR_MASK	(0x0F)    // RW

#define USB_ADDR_TX(n)		(USB_SRAM_BASE + ((n) * 16) + 0x00)
#define USB_COUNT_TX(n)		(USB_SRAM_BASE + ((n) * 16) + 0x04)
#define USB_ADDR_RX(n)		(USB_SRAM_BASE + ((n) * 16) + 0x08)
#define USB_COUNT_RX(n)		(USB_SRAM_BASE + ((n) * 16) + 0x0C)

#define USB_RX_SZ_8		((0 << 15) | (4 << 10))
#define USB_RX_SZ_16		((0 << 15) | (8 << 10))
#define USB_RX_SZ_32		((1 << 15) | (0 << 10))
#define USB_RX_SZ_64		((1 << 15) | (1 << 10))
#define USB_RX_SZ_128		((1 << 15) | (3 << 10))
#define USB_RX_SZ_256		((1 << 15) | (7 << 10))

extern void target_set_usb_active(bool on);

static void usb_handle_irq(void);

static volatile int _usb_online = 0;
static void *ep1_rx_data;
static volatile int ep1_rx_status;
static volatile int ep1_tx_busy;
static usb_callback usb_cb;

#define EP0_TX_ACK_ADDR	0 /* sending ACK, then changing address */
#define EP0_TX_ACK	1 /* sending ACK */
#define EP0_RX_ACK	2 /* receiving ACK */
#define EP0_TX		3 /* sending data */
#define EP0_RX		4 /* receiving data */
#define EP0_IDLE	5 /* waiting for SETUP */

static unsigned int current_epr;
static u8 ep0state = EP0_IDLE;
static u8 newaddr;

static unsigned int ep0rxb = USB_SRAM_BASE + 0x0040; /* 64 bytes */
static unsigned int ep0txb = USB_SRAM_BASE + 0x00c0; /* 64 bytes */
static unsigned int ep1rxb = USB_SRAM_BASE + 0x0140; /* 64 bytes */
static unsigned int ep1txb = USB_SRAM_BASE + 0x01c0; /* 64 bytes */

#define ADDR2USB(n) (((n) & 0x3FF) >> 1)

void usb_handle_reset(void) {
	_usb_online = 0;
	ep1_tx_busy = 0;
	ep1_rx_status = ERR_BUSY;

	writel(0, USB_BTABLE);
	writel(ADDR2USB(ep0txb), USB_ADDR_TX(0));
	writel(ADDR2USB(ep0rxb), USB_ADDR_RX(0));
	writel(0, USB_COUNT_TX(0));
	writel(USB_RX_SZ_64, USB_COUNT_RX(0));

	writel(ADDR2USB(ep1txb), USB_ADDR_TX(1));
	writel(ADDR2USB(ep1rxb), USB_ADDR_RX(1));
	writel(0, USB_COUNT_TX(1));
	writel(USB_RX_SZ_64, USB_COUNT_RX(1));

	writel(0x0 | USB_EPR_TYPE_CONTROL |
		USB_EPR_RX_NAK | USB_EPR_TX_NAK,
		USB_EPR(0));

	writel(0x1 | USB_EPR_TYPE_BULK |
		USB_EPR_RX_NAK | USB_EPR_TX_NAK, 
		USB_EPR(1));

	writel(0x00 | USB_DADDR_ENABLE, USB_DADDR);
}

/* exclude T and W0C bits */
#define EPMASK (USB_EPR_TYPE_MASK | USB_EPR_DBL_BUF | USB_EPR_ADDR_MASK)

static void ep0_recv_ack(void) {
	writel((current_epr & EPMASK) | USB_EPR_RX_STALL | USB_EPR_STATUS_OUT, USB_EPR(0));
}
static void ep0_send_ack(void) {
	ep0state = EP0_TX_ACK;
	writel(0, USB_COUNT_TX(0));
	writel((current_epr & EPMASK) | USB_EPR_TX_STALL, USB_EPR(0));
}

static void usb_handle_ep0_tx(void)
{
	LTRACEF("ep0state %d\n", ep0state);
	switch (ep0state) {
	case EP0_TX_ACK_ADDR:
		writel(newaddr | USB_DADDR_ENABLE, USB_DADDR);
	case EP0_TX_ACK:
		ep0state = EP0_IDLE;
		writel((current_epr & EPMASK), USB_EPR(0));
		break;
	case EP0_TX:
		ep0state = EP0_RX_ACK;
		ep0_recv_ack();
		break;
	}
}

static void usb_handle_ep0_rx(void)
{
	LTRACEF("ep0state %d\n", ep0state);
	switch (ep0state) {
	case EP0_RX_ACK:
		/* ack txn and make sure STATUS_OUT is cleared */
		writel(((current_epr & EPMASK) & (~USB_EPR_STATUS_OUT)) |
			USB_EPR_CTR_TX, USB_EPR(0));
		ep0state = EP0_IDLE;
		break;
	case EP0_RX:
		;
	}
}

void usb_handle_ep0_setup(void)
{
	LTRACE;
	u16 req, val, idx, len, x;

	req = readl(ep0rxb + 0x00);
	val = readl(ep0rxb + 0x04);
	idx = readl(ep0rxb + 0x08);
	len = readl(ep0rxb + 0x0C);
	x = readl(USB_COUNT_RX(0)) & 0x3ff;

	struct usb_setup setup;
	setup.request_type = req & 0xff;
	setup.request = (req >> 8) & 0xff;
	setup.value = val;
	setup.index = idx;
	setup.length = len;

	/* release SETUP latch by acking RX */
	writel((current_epr & EPMASK), USB_EPR(0));

	if (usb_cb) {
		union usb_callback_args args;

		args.setup = &setup;

		usb_cb(CB_SETUP_MSG, &args);
	}
}

void usbc_set_address(uint8_t addr)
{
	LTRACEF("addr %d\n", addr);
	ep0state = EP0_TX_ACK_ADDR;
	newaddr = addr & 0x7F;
}

void usbc_ep0_ack(void)
{
	LTRACE;
	ep0_send_ack();
}

void usbc_ep0_stall(void)
{
	LTRACE;
	ep0state = EP0_IDLE;
	writel((current_epr & EPMASK) | USB_EPR_TX_NAK | USB_EPR_TX_STALL, USB_EPR(0));
}

void usbc_ep0_send(const void *buf, size_t len, size_t maxlen)
{
	LTRACEF("buf %p, len %u, maxlen %u\n", buf, len, maxlen);

	ep0state = EP0_TX;

	u16 *src = (u16*)buf;
	u32 *dst = (void*)ep0txb;

	if (len > maxlen)
		len = maxlen;

	size_t n = (len & 1) + (len >> 1);
	while (n--)
		*dst++ = *src++;

	writel(len, USB_COUNT_TX(0));
	writel((current_epr & EPMASK) | USB_EPR_TX_STALL, USB_EPR(0));
}

void usbc_ep0_recv(void *buf, size_t len, ep_callback cb)
{
	PANIC_UNIMPLEMENTED;
}

bool usbc_is_highspeed(void)
{
	return false;
}

int usbc_set_callback(usb_callback cb)
{
	LTRACEF("cb %p\n", cb);

	usb_cb = cb;

	return 0;
}

int usbc_set_active(bool active)
{
	LTRACEF("active %d\n", active);

	target_set_usb_active(active);

	return 0;
}

static void usb_handle_ep0(void) {
	current_epr = readl(USB_EPR(0));
	if (current_epr & USB_EPR_SETUP) {
		usb_handle_ep0_setup();
	} else if (current_epr & USB_EPR_CTR_TX) {
		usb_handle_ep0_tx();
	} else if (current_epr & USB_EPR_CTR_RX) {
		usb_handle_ep0_rx();
	}
}

static void usb_handle_ep1(void) {
	unsigned int n;
	int len;

	n = readl(USB_EPR(1));
	if (n & USB_EPR_CTR_RX) {
		/* first, clear RX CTR */
		writel((n & EPMASK) | USB_EPR_CTR_TX, USB_EPR(1));

		u32 *src = (void*) ep1rxb;
		u16 *dst = (void*) ep1_rx_data;
		len = readl(USB_COUNT_RX(1)) & 0x3FF;
		ep1_rx_status = len;
		while (len > 0) {
			*dst++ = *src++;
			len -= 2;
		}
	}
	if (n & USB_EPR_CTR_TX) {
		/* first, clear TX CTR */
		writel((n & EPMASK) | USB_EPR_CTR_RX, USB_EPR(1));
		ep1_tx_busy = 0;
	}
}

#if 0
int usb_recv(void *_data, int count) {
	int r, rx = 0;
	unsigned int n;
	u8 *data = _data;

	while (!_usb_online)
		usb_handle_irq();

	while (count > 0) {
		if (!_usb_online)
			return ERR_NOT_READY;

		ep1_rx_data = data;
		ep1_rx_status = ERR_BUSY;

		/* move from NAK to VALID, don't touch any other bits */
		n = readl(USB_EPR(1)) & EPMASK;
		writel(n | USB_EPR_CTR_RX | USB_EPR_CTR_TX | USB_EPR_RX_STALL, USB_EPR(1));

		while (ep1_rx_status == ERR_BUSY)
			usb_handle_irq();

		r = ep1_rx_status;

		if (r < 0)
			return r;
		if (r > count)
			r = count;
		data += r;
		rx += r;
		count -= r;	

		/* terminate on short packet */
		if (r != 64)
			break;
	}

	return rx;
}

int usb_xmit(void *data, int len) {
	int tx = 0;
	int n;
	u16 *src = data;

	while (len > 0) {
		u32 *dst = (void*) ep1txb;
		int xfer = (len > 64) ? 64 : len;

		if (!_usb_online)
			return ERR_NOT_READY;

		while (ep1_tx_busy)
			usb_handle_irq();

		writel(xfer, USB_COUNT_TX(1));
		//printx("%x <- %x (%x)\n",dst, src, xfer);
		len -= xfer;
		tx += xfer;

		while (xfer > 0) {
			*dst++ = *src++;
			xfer -= 2;
		}

		/* move from NAK to VALID, don't touch any other bits */
		n = readl(USB_EPR(1)) & EPMASK;
		writel(n | USB_EPR_CTR_RX | USB_EPR_CTR_TX | USB_EPR_TX_STALL, USB_EPR(1));

		ep1_tx_busy = 1;

	}

	return tx;
}
#endif 

void usbc_init(void)
{
	TRACE_ENTRY;

	printx("usb_init()\n");

	/* enable USB clock */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USB, ENABLE);

	/* reset */
	writel(USB_CR_PDWN | USB_CR_FRES, USB_CR);
	spin(1000);
	writel(~USB_CR_PDWN, USB_CR); /* power up analog block */
	spin(1000);
	writel(0, USB_CR);
	writel(0, USB_ISR);

	usb_handle_reset();

	NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn);

	/* unmask interrupts */
	writel(USB_RESETM, USB_CR);

	TRACE_EXIT;
}

static void usb_handle_irq(void)
{
	inc_critical_section();

	unsigned int n;
	for (;;) {
		n = readl(USB_ISR);
		if (n & USB_RESETM) {
			usb_handle_reset();
			writel(~USB_RESETM, USB_ISR);
			continue;
		}
		if (n & USB_CTRM) {
			if ((n & 0x0F) == 0)
				usb_handle_ep0();
			if ((n & 0x0F) == 1)
				usb_handle_ep1();
			writel(~USB_CTRM, USB_ISR);
			continue;
		}
		break;
	}

	dec_critical_section();
}

void stm32_USB_HP_CAN1_TX_IRQ(void)
{
	panic("usb_hp\n");
}

void stm32_USB_LP_CAN1_RX0_IRQ(void)
{
	usb_handle_irq();
}

