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
#include <string.h>
#include <stdlib.h>
#include <kernel/thread.h>
#include <dev/usbc.h>
#include <dev/twl4030.h>
#include <reg.h>
#include <platform/omap3.h>
#include <platform/interrupts.h>
#include <hw/usb.h>

#define LOCAL_TRACE 0

#define hsusb_reg8(reg) *REG8(USB_HS_BASE + (reg))
#define hsusb_reg16(reg) *REG16(USB_HS_BASE + (reg))
#define hsusb_reg32(reg) *REG32(USB_HS_BASE + (reg))

/* registers */
#define FADDR	0x0
#define POWER	0x1
#define INTRTX	0x2
#define INTRRX	0x4
#define INTRTXE	0x6
#define INTRRXE	0x8
#define INTRUSB	0xa
#define INTRUSBE 0xb
#define FRAME 	0xc
#define INDEX 	0xe
#define TESTMODE 0xf

// indexed endpoint regs
#define IDX_TXMAXP	0x10
#define IDX_TXCSR	0x12
#define IDX_TXCSRL	0x12
#define IDX_TXCSRH	0x13
#define IDX_RXMAXP	0x14
#define IDX_RXCSR	0x16
#define IDX_RXCSRL	0x16
#define IDX_RXCSRH	0x17
#define IDX_RXCOUNT	0x18
#define IDX_FIFOSIZE	0x1f

// if endpoint 0 is selected
#define IDX_CSR0	0x12
#define IDX_CONFIGDATA	0x1f

// endpoint FIFOs
#define FIFOBASE	0x20

#define DEVCTL	0x60
#define TXFIFOSZ	0x62
#define RXFIFOSZ	0x63
#define TXFIFOADD	0x64
#define RXFIFOADD	0x66
#define HWVERS	0x6c
#define EPINFO	0x78
#define RAMINFO	0x79
#define LINKINFO	0x7a

static void setup_dynamic_fifos(void);

enum usb_state {
	USB_DEFAULT = 0,
	USB_ADDRESS,
	USB_CONFIGURED
};

struct usbc_ep {
	bool active;
	uint width;
	uint blocksize;
	
	/* current data buffer */
	usbc_transfer *transfer;

	/* callback when tx or rx happens on the endpoint */
	int (*callback)(ep_t endpoint, usbc_callback_op_t op, usbc_transfer *transfer);
};

struct usbc_stat {
	bool active;
	enum usb_state state;
	uint8_t active_config;

	// callback for device events
	usb_callback callback;

	// ep0 pending tx
	const void *ep0_tx_buf;
	size_t ep0_tx_len;
	uint ep0_tx_pos;

	struct usbc_ep inep[16];  // IN endpoints (device to host)
	struct usbc_ep outep[16]; // OUT endpoint (host to device)
};

static struct usbc_stat *usbc;

struct usbc_callback {
	struct list_node node;
	usb_callback callback;
};

static struct list_node usbc_callback_list;

static void call_all_callbacks(usbc_callback_op_t op, const union usb_callback_args *arg)
{
	struct usbc_callback *cb;

	list_for_every_entry(&usbc_callback_list, cb, struct usbc_callback, node) {
		LTRACEF("calling %p, op %d, arg %p\n", cb->callback, op, arg);
		cb->callback(op, arg);
	}
}

static void print_usb_setup(const struct usb_setup *setup)
{
	printf("usb_setup:\n");
	printf("\ttype 0x%hhx\n", setup->request_type);
	printf("\trequest 0x%hhx\n", setup->request);
	printf("\tvalue 0x%hx\n", setup->value);
	printf("\tindex 0x%hx\n", setup->index);
	printf("\tlength 0x%hx\n", setup->length);
}

static void select_ep(uint ep)
{
	DEBUG_ASSERT(ep < 16);
	hsusb_reg8(INDEX) = ep;
}

static void dump_ep_regs(uint ep)
{
#if 0
	select_ep(ep);

	LTRACEF("%d txmaxp 0x%hx\n", ep, hsusb_reg16(IDX_TXMAXP));
	LTRACEF("%d rxmaxp 0x%hx\n", ep, hsusb_reg16(IDX_RXMAXP));
	LTRACEF("%d txfifosz 0x%hhx\n", ep, hsusb_reg8(TXFIFOSZ));
	LTRACEF("%d rxfifosz 0x%hhx\n", ep, hsusb_reg8(RXFIFOSZ));
	LTRACEF("%d txfifoadd 0x%hx\n", ep, hsusb_reg16(TXFIFOADD));
	LTRACEF("%d rxfifoadd 0x%hx\n", ep, hsusb_reg16(RXFIFOADD));
#endif
}

#define MULOF4(val) (((uint32_t)(val) & 0x3) == 0)

static int read_ep_fifo(uint ep, void *_buf, size_t maxlen)
{
	char *buf = (void *)_buf;

	select_ep(ep);

	uint8_t fifo_reg = FIFOBASE + ep * 4;
	size_t rxcount = hsusb_reg16(IDX_RXCOUNT);

	if (rxcount > maxlen)
		rxcount = maxlen;

	if (MULOF4(buf) && MULOF4(rxcount)) {
		uint i;
		uint32_t *buf32 = (uint32_t *)_buf;
		for (i=0; i < rxcount / 4; i++) {
			buf32[i] = hsusb_reg32(fifo_reg);
		}
	} else {
		/* slow path */
		uint i;
		for (i=0; i < rxcount; i++) {
			buf[i] = hsusb_reg8(fifo_reg);
		}
	}

	return rxcount;
}

static int write_ep_fifo(uint ep, const void *_buf, size_t len)
{
	char *buf = (void *)_buf;

	select_ep(ep);

	uint8_t fifo_reg = FIFOBASE + ep * 4;

	if (MULOF4(buf) && MULOF4(len)) {
		uint i;
		uint32_t *buf32 = (uint32_t *)_buf;
		for (i=0; i < len / 4; i++) {
			hsusb_reg32(fifo_reg) = buf32[i];
		}
	} else {
		/* slow path */
		uint i;
		for (i=0; i < len; i++) {
			hsusb_reg8(fifo_reg) = buf[i];
		}
	}

	return len;
}

#undef MULOF4

void usbc_ep0_send(const void *buf, size_t len, size_t maxlen)
{
	LTRACEF("buf %p, len %zu, maxlen %zu\n", buf, len, maxlen);

	// trim the transfer
	len = MIN(len, maxlen);

	size_t transfer_len = MIN(64, len);

	// write the first 64 bytes
	write_ep_fifo(0, buf, transfer_len);

	// set txpktready
	select_ep(0);
	if (len > 64) {
		// we have more data to send, don't mark data end
		hsusb_reg16(IDX_CSR0) |= (1<<1); // TxPktRdy

		// save our position so we can continue
		usbc->ep0_tx_buf = buf;
		usbc->ep0_tx_pos = 64;
		usbc->ep0_tx_len = len;
	} else {
		hsusb_reg16(IDX_CSR0) |= (1<<3) | (1<<1); // DataEnd, TxPktRdy
		usbc->ep0_tx_buf = NULL;
	}
}

static void ep0_control_send_resume(void)
{
	DEBUG_ASSERT(usbc->ep0_tx_buf != NULL);
	DEBUG_ASSERT(usbc->ep0_tx_len > usbc->ep0_tx_pos);

	LTRACEF("buf %p pos %d len %d\n", usbc->ep0_tx_buf, usbc->ep0_tx_pos, usbc->ep0_tx_len);

	size_t transfer_len = MIN(64, usbc->ep0_tx_len - usbc->ep0_tx_pos);
	
	write_ep_fifo(0, (const uint8_t *)usbc->ep0_tx_buf + usbc->ep0_tx_pos, transfer_len);

	usbc->ep0_tx_pos += transfer_len;

	if (usbc->ep0_tx_pos >= usbc->ep0_tx_len) {
		// completes the transfer
		hsusb_reg16(IDX_CSR0) |= (1<<3) | (1<<1); // DataEnd, TxPktRdy
		usbc->ep0_tx_buf = NULL;
	} else {
		hsusb_reg16(IDX_CSR0) |= (1<<1); // TxPktRdy
	}
}

void usbc_ep0_ack(void)
{
	hsusb_reg16(IDX_CSR0) |= (1<<6)|(1<<3); // servicedrxpktrdy & dataend
}

void usbc_ep0_stall(void)
{
	printf("USB STALL\n");
}

static void usb_shutdown_endpoints(void)
{
	// iterate through all the endpoints, cancelling any pending io and shut down the endpoint
	ep_t i;
	for (i=1; i < 16; i++) {
		if (usbc->inep[i].active && usbc->inep[i].transfer) {
			// pool's closed
			usbc_transfer *t = usbc->inep[i].transfer;
			usbc->inep[i].transfer = NULL;
			t->result = USB_TRANSFER_RESULT_CANCELLED;
			usbc->inep[i].callback(i, CB_EP_TRANSFER_CANCELLED, t);
		}
		if (usbc->outep[i].active && usbc->outep[i].transfer) {
			// pool's closed
			usbc_transfer *t = usbc->outep[i].transfer;
			usbc->outep[i].transfer = NULL;
			t->result = USB_TRANSFER_RESULT_CANCELLED;
			usbc->outep[i].callback(i, CB_EP_TRANSFER_CANCELLED, t);
		}
	}

	// clear pending ep0 data
	usbc->ep0_tx_buf = 0;
}

static void usb_enable_endpoints(void)
{
	setup_dynamic_fifos();	
}

static void usb_disconnect(void)
{
	// we've been disconnected
	usbc->state = USB_DEFAULT;
	usbc->active_config = 0;
	
	usb_shutdown_endpoints();
}

static void usb_reset(void)
{
	// this wipes out our endpoint interrupt disables
	hsusb_reg16(INTRTXE) = (1<<0);
	hsusb_reg16(INTRRXE) = 0;

	usb_shutdown_endpoints();
}

static int handle_ep_rx(int ep)
{
	struct usbc_ep *e = &usbc->outep[ep];

	DEBUG_ASSERT(e->active);

	DEBUG_ASSERT(e->transfer); // can't rx to no transfer
	usbc_transfer *t = e->transfer;

	uint rxcount = hsusb_reg16(IDX_RXCOUNT);
	uint readcount = MIN(rxcount, t->buflen - t->bufpos);
	readcount = MIN(readcount, e->blocksize);

	int len = read_ep_fifo(ep, (uint8_t *)t->buf + t->bufpos, readcount);
	LTRACEF("read %d bytes from the fifo\n", len);

	// no more packet ready
	hsusb_reg16(IDX_RXCSRL) &= ~(1<<0); // clear rxpktrdy

	t->bufpos += len;

	if (rxcount < e->blocksize || t->bufpos >= t->buflen) {
		// we're done with this transfer, clear it and disable the endpoint
		e->transfer = NULL;
		hsusb_reg16(INTRRXE) &= ~(1<<ep);

		t->result = USB_TRANSFER_RESULT_OK;

		DEBUG_ASSERT(e->callback);
		e->callback(ep, CB_EP_RXCOMPLETE, t);

		return 1;
	}

	return 0;
}

bool usbc_is_highspeed(void)
{
	return (hsusb_reg8(POWER) & (1<<4)) ? true : false;
}

static enum handler_return hsusb_interrupt(void *arg)
{
	uint16_t intrtx = hsusb_reg16(INTRTX);
	uint16_t intrrx = hsusb_reg16(INTRRX);
	uint8_t intrusb = hsusb_reg8(INTRUSB);
	enum handler_return ret = INT_NO_RESCHEDULE;

	LTRACEF("intrtx 0x%hx (0x%x), intrrx 0x%hx (0x%x), intrusb 0x%hhx, intrusbe 0x%hhx\n", 
			intrtx, hsusb_reg16(INTRTXE), intrrx, hsusb_reg16(INTRRXE), intrusb, hsusb_reg8(INTRUSBE));

	dump_ep_regs(2);

	// look for global usb interrupts
	intrusb &= hsusb_reg8(INTRUSBE);
	if (intrusb) {
		if (intrusb & (1<<0)) {
			// suspend
			TRACEF("suspend\n");
			call_all_callbacks(CB_SUSPEND, 0);
			ret = INT_RESCHEDULE;
		}
		if (intrusb & (1<<1)) {
			// resume
			TRACEF("resume\n");
			call_all_callbacks(CB_RESUME, 0);
			ret = INT_RESCHEDULE;
		}
		if (intrusb & (1<<2)) {
			// reset
			TRACEF("reset\n");
			TRACEF("high speed %d\n", hsusb_reg8(POWER) & (1<<4) ? 1 : 0);
			call_all_callbacks(CB_RESET, 0);
			usb_reset();
			ret = INT_RESCHEDULE;
		}
		if (intrusb & (1<<3)) {
			// SOF
			TRACEF("sof\n");
		}
		if (intrusb & (1<<4)) {
			// connect (host only)
			TRACEF("connect\n");
		}
		if (intrusb & (1<<5)) {
			// disconnect
			TRACEF("disconnect\n");
			call_all_callbacks(CB_DISCONNECT, 0);
			usb_disconnect();
			ret = INT_RESCHEDULE;
		}
		if (intrusb & (1<<6)) {
			// session request (A device only)
			TRACEF("session request\n");
		}
		if (intrusb & (1<<7)) {
			// vbus error (A device only)
			TRACEF("vbus error\n");
		}
	}

	// look for endpoint 0 interrupt
	if (intrtx & 1) {
		select_ep(0);
		uint16_t csr = hsusb_reg16(IDX_CSR0);
		LTRACEF("ep0 csr 0x%hhx\n", csr);
	
		// clear the stall bit
		if (csr & (1<<2))
			hsusb_reg16(IDX_CSR0) &= ~(1<<2);

		// do we have any pending tx data?
		if (usbc->ep0_tx_buf != NULL) {
			if (csr & (1<<4)) { // setup end
				// we got an abort on the data transfer
				usbc->ep0_tx_buf = NULL;
			} else {
				// send more data
				ep0_control_send_resume();
			}
		}

		// clear the setup end bit
		if (csr & (1<<4)) {
			hsusb_reg16(IDX_CSR0) |= (1<<7); // servicedsetupend
		}

		if (csr & 0x1) {
			// rxpktrdy
			LTRACEF("ep0: rxpktrdy, count %d\n", hsusb_reg16(IDX_RXCOUNT));

			struct usb_setup setup;
			read_ep_fifo(0, (void *)&setup, sizeof(setup));
//			print_usb_setup(&setup);

			hsusb_reg16(IDX_CSR0) |= (1<<6); // servicedrxpktrdy

			union usb_callback_args args;
			args.setup = &setup;
			call_all_callbacks(CB_SETUP_MSG, &args);

			switch (setup.request) {
				case SET_ADDRESS: {
					LTRACEF("got SET_ADDRESS: value %d\n", setup.value);
					dprintf(INFO, "usb: got assigned address %d\n", setup.value);
					usbc_ep0_ack();

					hsusb_reg8(FADDR) = setup.value;
					if (setup.value == 0)
						usbc->state = USB_DEFAULT;
					else
						usbc->state = USB_ADDRESS;

					break;
				}
				case SET_CONFIGURATION:
					LTRACEF("got SET_CONFIGURATION, config %d\n", setup.value);

					if (setup.value == 0) {
						if (usbc->state == USB_CONFIGURED)
							usbc->state = USB_ADDRESS;
						call_all_callbacks(CB_OFFLINE, 0);
					} else {
						usbc->state = USB_CONFIGURED;
						call_all_callbacks(CB_ONLINE, 0);
					}
					usbc->active_config = setup.value;
					ret = INT_RESCHEDULE;

					// set up all of the endpoints
					usb_enable_endpoints();
					break;
			}
		}
	}

	// handle endpoint interrupts

	// mask out ones we don't want to play with
	intrtx &= hsusb_reg16(INTRTXE);
	intrrx &= hsusb_reg16(INTRRXE);

	int i;
	for (i=1; i < 16; i++) {
		if (intrtx & (1<<i)) {
			select_ep(i);

			LTRACEF("txcsr %i: 0x%hx\n", i, hsusb_reg16(IDX_TXCSR));

			// data was sent, see if we have more to send
			struct usbc_ep *e = &usbc->inep[i];

			DEBUG_ASSERT(e->transfer); // interrupts shouldn't be enabled if there isn't a transfer queued
			usbc_transfer *t = e->transfer;

			if (t->bufpos < t->buflen) {
				// cram more stuff in the buffer
				uint queuelen = MIN(e->blocksize, t->buflen - t->bufpos);
				LTRACEF("writing more tx data into fifo: len %u, remaining %zu\n", queuelen, t->buflen - t->bufpos);
				write_ep_fifo(i, (uint8_t *)t->buf + t->bufpos, queuelen);
				t->bufpos += queuelen;

				// start the transfer
				hsusb_reg16(IDX_TXCSR) |= (1<<0); // txpktrdy
			} else {
				// we're done, callback
				e->transfer = NULL;
				hsusb_reg16(INTRTXE) &= ~(1<<i);

				t->result = USB_TRANSFER_RESULT_OK;

				DEBUG_ASSERT(e->callback);
				e->callback(i, CB_EP_TXCOMPLETE, t);
				ret = INT_RESCHEDULE;
			}
		}
		if (intrrx & (1<<i)) {
			select_ep(i);

			uint16_t csr = hsusb_reg16(IDX_RXCSR);
			LTRACEF("rxcsr %i: 0x%hx\n", i, csr);

			if (csr & 0x1) { // rxpktrdy
				// see if the endpoint is ready
				struct usbc_ep *e = &usbc->outep[i];
				if (!e->active) {
					// stall it
					hsusb_reg16(IDX_RXCSR) |= (1<<6); // stall
					hsusb_reg16(IDX_RXCSR) |= (1<<4); // flush fifo
					panic("rx on inactive endpoint\n");
					continue;
				}
	
				if (handle_ep_rx(i) > 0)
					ret = INT_RESCHEDULE;
			}
		}
	}

	return ret;
}

static enum handler_return hsusb_dma_interrupt(void *arg)
{
	LTRACE;

	return INT_NO_RESCHEDULE;
}

void usbc_setup_endpoint(ep_t ep, ep_dir_t dir, bool active, ep_callback callback, uint width, uint blocksize)
{
	DEBUG_ASSERT(ep != 0);
	DEBUG_ASSERT(ep < 16);
	DEBUG_ASSERT(dir == IN || dir == OUT);

	struct usbc_ep *e;
	if (dir == IN)
		e = &usbc->inep[ep];
	else
		e = &usbc->outep[ep];

	// for now we can only make active
	e->active = active;
	e->callback = callback;
	e->width = width;
	e->blocksize = blocksize;
}

int usbc_queue_rx(ep_t ep, usbc_transfer *transfer)
{
	LTRACE;
	struct usbc_ep *e = &usbc->outep[ep];

	DEBUG_ASSERT(ep != 0);
	DEBUG_ASSERT(ep < 16);
	DEBUG_ASSERT(e->active);

	DEBUG_ASSERT(transfer);
	DEBUG_ASSERT(transfer->buf);

	DEBUG_ASSERT(e->transfer == NULL);

	// can only queue up multiples of the endpoint blocksize
	DEBUG_ASSERT(transfer->buflen >= e->blocksize && (transfer->buflen % e->blocksize) == 0);

	enter_critical_section();

	if (usbc->state != USB_CONFIGURED) {
		// can't transfer now
		exit_critical_section();
		return -1;
	}

	e->transfer = transfer;

	// make sure the ep is set up right
//	select_ep(ep);
//	hsusb_reg8(IDX_RXCSRH) = 0;
	dump_ep_regs(ep);

	select_ep(ep);
	if (hsusb_reg16(IDX_RXCSR) & (1<<0)) {
		// packet already ready
		LTRACEF("****packet already ready (%d)\n", hsusb_reg16(IDX_RXCOUNT));

		int rc = handle_ep_rx(ep);
		if (rc > 0) {
			// the transfer was completed
			goto done;
		}
	}

	// unmask irqs for this endpoint
	hsusb_reg16(INTRRXE) |= (1<<ep);

done:
	exit_critical_section();

	return 0;
}

int usbc_queue_tx(ep_t ep, usbc_transfer *transfer)
{
	LTRACEF("ep %u, transfer %p (buf %p, len %zu)\n", ep, transfer, transfer->buf, transfer->buflen);
	struct usbc_ep *e = &usbc->inep[ep];

	DEBUG_ASSERT(ep != 0);
	DEBUG_ASSERT(ep < 16);
	DEBUG_ASSERT(e->active);

	DEBUG_ASSERT(e->transfer == NULL);

	enter_critical_section();

	if (usbc->state != USB_CONFIGURED) {
		// can't transfer now
		exit_critical_section();
		return -1;
	}

e->transfer = transfer;

	select_ep(ep);

	// set this endpoint in tx mode
//	hsusb_reg8(IDX_TXCSRH) = (1<<7)|(1<<5); // autoset, tx direction
	dump_ep_regs(ep);

	// unmask irqs for this endpoint
	hsusb_reg16(INTRTXE) |= (1<<ep);
	
	// if the fifo is empty, start the transfer
	if ((hsusb_reg16(IDX_TXCSR) & (1<<1)) == 0) {
		// dump the start of the transfer in the fifo
		uint queuelen = MIN(e->blocksize, transfer->buflen);
		write_ep_fifo(ep, transfer->buf, queuelen);
		transfer->bufpos = queuelen;

		// start the transfer
		hsusb_reg16(IDX_TXCSR) |= (1<<0); // txpktrdy
	}

	exit_critical_section();

	return 0;
}

int usbc_set_callback(usb_callback callback)
{
	DEBUG_ASSERT(callback != NULL);

	struct usbc_callback *cb = malloc(sizeof(struct usbc_callback));

	enter_critical_section();

	cb->callback = callback;
	list_add_head(&usbc_callback_list, &cb->node);

	exit_critical_section();
	return 0;
}

int usbc_set_active(bool active)
{
	LTRACEF("active %d\n", active);
	if (active) {
		DEBUG_ASSERT(!usbc->active);

		hsusb_reg8(POWER) |= (1<<6); // soft conn
		twl4030_set_usb_pullup(true);
		usbc->active = true;
	} else {
		hsusb_reg8(POWER) &= ~(1<<6); // soft conn
		twl4030_set_usb_pullup(false);
		usbc->active = false;
	}

	return 0;
}

static void setup_dynamic_fifos(void)
{
//	LTRACE;

#if LOCAL_TRACE
	uint8_t raminfo = hsusb_reg8(RAMINFO);
	size_t ramsize = (1 << ((raminfo & 0xf) + 2));
	LTRACEF("%zd bytes of onboard ram\n", ramsize);
#endif

	uint32_t offset = 128;

	int highspeed = hsusb_reg8(POWER) & (1<<4);

	int i;
	for (i=1; i < 16; i++) {
		select_ep(i);
		if (usbc->inep[i].active) {
			hsusb_reg8(TXFIFOSZ) = (1<<4)|(0x6); // 512 byte, double buffered
			hsusb_reg8(RXFIFOSZ) = 0;
			hsusb_reg16(TXFIFOADD) = offset / 8;
			hsusb_reg16(RXFIFOADD) = 0;
			if (highspeed) {
				hsusb_reg16(IDX_TXMAXP) = usbc->inep[i].width;
			} else {
				hsusb_reg16(IDX_TXMAXP) = (((usbc->inep[i].blocksize / 64) - 1) << 11) | 64;
//				hsusb_reg16(IDX_TXMAXP) = 64;
//				usbc->inep[i].blocksize = 64;
			}

			hsusb_reg16(IDX_RXMAXP) = 0;
			LTRACEF("%d: txmaxp 0x%hx\n", i, hsusb_reg16(IDX_TXMAXP));
			hsusb_reg8(IDX_TXCSRH) = (1<<5)|(1<<3);
			hsusb_reg8(IDX_TXCSRL) = (1<<3);
			hsusb_reg8(IDX_TXCSRL) = (1<<3);
			offset += 512*2;
		} else {
			hsusb_reg8(TXFIFOSZ) = 0;
			hsusb_reg16(TXFIFOADD) = 0;
			hsusb_reg16(IDX_TXMAXP) = 0;
		}
		if (usbc->outep[i].active) {
			hsusb_reg8(TXFIFOSZ) = 0;
			hsusb_reg8(RXFIFOSZ) = (0<<4)|(0x6); // 512 byte, single buffered
			hsusb_reg16(TXFIFOADD) = 0;
			hsusb_reg16(RXFIFOADD) = offset / 8;
			hsusb_reg16(IDX_TXMAXP) = 0;
			if (highspeed) {
				hsusb_reg16(IDX_RXMAXP) = usbc->inep[i].width;
			} else {
				hsusb_reg16(IDX_RXMAXP) = (((usbc->outep[i].blocksize / 64) - 1) << 11) | 64;
//				hsusb_reg16(IDX_RXMAXP) = 64;
//				usbc->outep[i].blocksize = 64;
			}
			LTRACEF("%d: rxmaxp 0x%hx\n", i, hsusb_reg16(IDX_RXMAXP));
			offset += 512;
			hsusb_reg8(IDX_RXCSRH) = (1<<7);
			hsusb_reg8(IDX_RXCSRL) = (1<<7);

//			LTRACEF("rxcsr 0x%hx\n", hsusb_reg16(IDX_RXCSR));
		} else {
			hsusb_reg8(RXFIFOSZ) = 0;
			hsusb_reg16(RXFIFOADD) = 0;
			hsusb_reg16(IDX_RXMAXP) = 0;
		}
//		LTRACEF("%d txfifosz 0x%hhx\n", i, hsusb_reg8(TXFIFOSZ));
//		LTRACEF("%d rxfifosz 0x%hhx\n", i, hsusb_reg8(RXFIFOSZ));
//		LTRACEF("%d txfifoadd 0x%hx\n", i, hsusb_reg16(TXFIFOADD));
//		LTRACEF("%d rxfifoadd 0x%hx\n", i, hsusb_reg16(RXFIFOADD));
	}
}

static void otg_reset(void)
{
	/* reset the chip */
	*REG32(OTG_SYSCONFIG) |= (1<<1);
	while ((*REG32(OTG_SYSSTATUS) & 1) == 0)
		;

	/* power up the controller */
	*REG32(OTG_FORCESTDBY) = 0; // disable forced standby
	*REG32(OTG_SYSCONFIG) &= ~(1<<1); // autoidle off
	*REG32(OTG_SYSCONFIG) = (2<<12) | (2<<3) | (0<<0); // master in smart-standby, periph in smart-idle, autoidle off

	*REG32(OTG_SYSCONFIG) |= 1; // autoidle on

	*REG32(OTG_INTERFSEL) = 1; // 8 bit ULPI
}

static void hsusb_init(void)
{
	LTRACE_ENTRY;

	// select endpoint 0
	dprintf(SPEW, "hwvers 0x%hx\n", hsusb_reg16(HWVERS)); 
	dprintf(SPEW, "epinfo 0x%hhx\n", hsusb_reg8(EPINFO)); 
	dprintf(SPEW, "raminfo 0x%hhx\n", hsusb_reg8(RAMINFO)); 
	hsusb_reg8(INDEX) = 0;
	dprintf(SPEW, "config 0x%hhx\n", hsusb_reg8(IDX_CONFIGDATA));

	// assert that we have dynamic fifo sizing
	DEBUG_ASSERT(hsusb_reg8(IDX_CONFIGDATA) & (1<<2));

	// mask all the interrupts for the endpoints (except 0)
	hsusb_reg16(INTRTXE) = (1<<0);
	hsusb_reg16(INTRRXE) = 0;

	twl4030_usb_reset();
	twl4030_init_hs();

	hsusb_reg8(DEVCTL) = 0; // peripheral mode
//	hsusb_reg8(POWER) &= (1<<5); // disable high speed
	hsusb_reg8(POWER) |= (1<<5); // enable high speed

	hsusb_reg8(INTRUSBE) = (1<<5)|(1<<2)|(1<<1)|(1<<0); // disconnect, reset, resume, suspend

	LTRACE_EXIT;
}

void usbc_init(void)
{
	LTRACE_ENTRY;

	// enable the clock
	RMWREG32(CM_ICLKEN1_CORE, 4, 1, 1);

	// allocate some ram for the usb struct
	usbc = malloc(sizeof(struct usbc_stat));
	memset(usbc, 0, sizeof(struct usbc_stat));

	usbc->state = USB_DEFAULT;

	// initialize the callback list
	list_initialize(&usbc_callback_list);

	// register the interrupt handlers
	register_int_handler(92, hsusb_interrupt, NULL);
//	register_int_handler(93, hsusb_dma_interrupt, NULL);

	otg_reset();
	hsusb_init();

	unmask_interrupt(92);
//	unmask_interrupt(93);

	LTRACE_EXIT;
}

