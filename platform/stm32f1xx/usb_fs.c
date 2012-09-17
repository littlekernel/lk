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

#include <fw/types.h>
#include <fw/lib.h>
#include <fw/io.h>

#include <arch/hardware.h>
#include <protocol/usb.h>

void usb_handle_irq(void);

void irq_usb_lp(void) {
	printx("IRQ USB LP\n");
	for (;;) ;
}
void irq_usb_hp(void) {
	printx("IRQ USB HP\n");
	for (;;) ;
}

static volatile int _usb_online = 0;
static void *ep1_rx_data;
static volatile int ep1_rx_status;
static volatile int ep1_tx_busy;

static unsigned ep0rxb = USB_SRAM_BASE + 0x0040; /* 64 bytes */
static unsigned ep0txb = USB_SRAM_BASE + 0x00c0; /* 64 bytes */
static unsigned ep1rxb = USB_SRAM_BASE + 0x0140; /* 64 bytes */
static unsigned ep1txb = USB_SRAM_BASE + 0x01c0; /* 64 bytes */

#define ADDR2USB(n) (((n) & 0x3FF) >> 1)

void usb_handle_reset(void) {
	_usb_online = 0;
	ep1_tx_busy = 0;
	ep1_rx_status = -ENODEV;

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

static u8 _dev00[] = {
	18,		/* size */
	DSC_DEVICE,
	0x00, 0x01,	/* version */
	0xFF,		/* class */
	0x00,		/* subclass */
	0x00,		/* protocol */
	0x40,		/* maxpacket0 */
	0xd1, 0x18,	/* VID */
	0x02, 0x65,	/* PID */
	0x00, 0x01,	/* version */
	0x00,		/* manufacturer string */
	0x00,		/* product string */
	0x00,		/* serialno string */
	0x01,		/* configurations */
};

static u8 _cfg00[] = {
	9,
	DSC_CONFIG,
	0x20, 0x00,	/* total length */
	0x01,		/* ifc count */
	0x01,		/* configuration value */
	0x00,		/* configuration string */
	0x80,		/* attributes */
	50,		/* mA/2 */

	9,
	DSC_INTERFACE,
	0x00,		/* interface number */
	0x00,		/* alt setting */
	0x02,		/* ept count */
	0xFF,		/* class */
	0x00,		/* subclass */
	0x00,		/* protocol */
	0x00,		/* interface string */

	7,
	DSC_ENDPOINT,
	0x81,		/* address */
	0x02,		/* bulk */
	0x40, 0x00,	/* max packet size */
	0x00,		/* interval */

	7,
	DSC_ENDPOINT,
	0x01,		/* address */
	0x02,		/* bulk */
	0x40, 0x00,	/* max packet size */
	0x00,		/* interval */

};

static struct {
	u16 id;
	u16 len;
	u8 *desc;
} dtable[] = {
	{ 0x0100, sizeof(_dev00), _dev00 },
	{ 0x0200, sizeof(_cfg00), _cfg00 },
};

unsigned load_desc(unsigned id) {
	unsigned n, len;
	for (n = 0; n < (sizeof(dtable)/sizeof(dtable[0])); n++) {
		if (id == dtable[n].id) {
			u16 *src = (u16*) dtable[n].desc;
			u32 *dst = (void*) ep0txb;
			len = dtable[n].len;
			n = (len & 1) + (len >> 1);
			while (n--)
				*dst++ = *src++;
			return len;
		}
	}
	printx("? %h\n", id);
	return 0;
}


/* exclude T and W0C bits */
#define EPMASK (USB_EPR_TYPE_MASK | USB_EPR_DBL_BUF | USB_EPR_ADDR_MASK)

#define EP0_TX_ACK_ADDR	0 /* sending ACK, then changing address */
#define EP0_TX_ACK	1 /* sending ACK */
#define EP0_RX_ACK	2 /* receiving ACK */
#define EP0_TX		3 /* sending data */
#define EP0_RX		4 /* receiving data */
#define EP0_IDLE	5 /* waiting for SETUP */

static void ep0_recv_ack(unsigned n) {
	writel((n & EPMASK) | USB_EPR_RX_STALL | USB_EPR_STATUS_OUT, USB_EPR(0));
}
static void ep0_send_ack(unsigned n) {
	writel(0, USB_COUNT_TX(0));
	writel((n & EPMASK) | USB_EPR_TX_STALL, USB_EPR(0));
}

static u8 ep0state = EP0_IDLE;
static u8 newaddr;

void usb_handle_ep0_tx(unsigned n) {
	switch (ep0state) {
	case EP0_TX_ACK_ADDR:
		writel(newaddr | USB_DADDR_ENABLE, USB_DADDR);
	case EP0_TX_ACK:
		ep0state = EP0_IDLE;
		writel((n & EPMASK), USB_EPR(0));
		break;
	case EP0_TX:
		ep0state = EP0_RX_ACK;
		ep0_recv_ack(n);
		break;
	}
}

void usb_handle_ep0_rx(unsigned n) {
	switch (ep0state) {
	case EP0_RX_ACK:
		/* ack txn and make sure STATUS_OUT is cleared */
		writel(((n & EPMASK) & (~USB_EPR_STATUS_OUT)) |
			USB_EPR_CTR_TX, USB_EPR(0));
		ep0state = EP0_IDLE;
		break;
	case EP0_RX:
		;
	}
}

void usb_handle_ep0_setup(unsigned n) {
	u16 req, val, idx, len, x;

	req = readl(ep0rxb + 0x00);
	val = readl(ep0rxb + 0x04);
	idx = readl(ep0rxb + 0x08);
	len = readl(ep0rxb + 0x0C);
	x = readl(USB_COUNT_RX(0));

	/* release SETUP latch by acking RX */
	writel((n & EPMASK), USB_EPR(0));

	switch (req) {
	case GET_DESCRIPTOR:
		x = load_desc(val);
		if (x == 0)
			goto error;
		if (x > len)
			x = len;
		ep0state = EP0_TX;
		writel(x, USB_COUNT_TX(0));
		writel((n & EPMASK) | USB_EPR_TX_STALL, USB_EPR(0));
		return;
	case SET_ADDRESS:
		ep0state = EP0_TX_ACK_ADDR;
		newaddr = val & 0x7F;
		ep0_send_ack(n);
		return;
	case SET_CONFIGURATION:
		ep0state = EP0_TX_ACK;
		ep0_send_ack(n);
		_usb_online = 1; /* TODO: check value */
		return;	
	}

	/* unknown request */
	printx("? %b %b %h %h %h\n", req, req >> 8, val, idx, len);

error:
	/* error, stall TX */
	writel((n & EPMASK) | USB_EPR_TX_NAK | USB_EPR_TX_STALL, USB_EPR(0));
}

void usb_handle_ep0(void) {
	unsigned n = readl(USB_EPR(0));
	if (n & USB_EPR_SETUP) {
		usb_handle_ep0_setup(n);
	} else if (n & USB_EPR_CTR_TX) {
		usb_handle_ep0_tx(n);
	} else if (n & USB_EPR_CTR_RX) {
		usb_handle_ep0_rx(n);
	}
}

void usb_handle_ep1(void) {
	unsigned n;
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

int usb_recv(void *_data, int count) {
	int r, rx = 0;
	unsigned n;
	u8 *data = _data;

	while (!_usb_online)
		usb_handle_irq();

	while (count > 0) {
		if (!_usb_online)
			return -ENODEV;

		ep1_rx_data = data;
		ep1_rx_status = -EBUSY;

		/* move from NAK to VALID, don't touch any other bits */
		n = readl(USB_EPR(1)) & EPMASK;
		writel(n | USB_EPR_CTR_RX | USB_EPR_CTR_TX | USB_EPR_RX_STALL, USB_EPR(1));

		while (ep1_rx_status == -EBUSY)
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
			return -ENODEV;

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

void usb_init(unsigned vid, unsigned pid) {
	unsigned n;

	_dev00[8] = vid;
	_dev00[9] = vid >> 8;
	_dev00[10] = pid;
	_dev00[11] = pid >> 8;

	/* enable GPIOC */
	writel(readl(RCC_APB2ENR) | RCC_APB2_GPIOC, RCC_APB2ENR);

	/* configure GPIOC-12 */
	writel(1 << 12, GPIOC_BASE + GPIO_BSR);
	n = readl(GPIOC_BASE + GPIO_CRH);
	n = (n & 0xFFF0FFFF) | 0x00050000;
	writel(n, GPIOC_BASE + GPIO_CRH);

	printx("usb_init()\n");

	/* enable USB clock */
	writel(readl(RCC_APB1ENR) | RCC_APB1_USB, RCC_APB1ENR);

	/* reset */
	writel(USB_CR_PDWN | USB_CR_FRES, USB_CR);
	for (n = 0; n < 100000; n++) asm("nop");
	writel(~USB_CR_PDWN, USB_CR); /* power up analog block */
	for (n = 0; n < 100000; n++) asm("nop");
	writel(0, USB_CR);
	writel(0, USB_ISR);

	usb_handle_reset();

	/* become active on the bus */
	writel(1 << 12, GPIOC_BASE + GPIO_BRR);
}

void usb_handle_irq(void) {
	unsigned n;
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
}

