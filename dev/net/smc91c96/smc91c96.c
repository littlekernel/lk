/*
 * Copyright (c) 2008-2009 Travis Geiselbrecht
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
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <debug.h>
#include <printf.h>
#include <kernel/thread.h>
#include <kernel/event.h>
#include <kernel/timer.h>
#include <lib/net/if.h>
#include <lib/net/ethernet.h>
#include <dev/net/smc91c96.h>
#include "smc91c96_p.h"
#include <dev/gpio.h>

#define LOCAL_TRACE 0

#if !defined(SMC91C96_BASE_ADDR) || !defined(SMC91C96_IRQ)
#error need to define SMC91C96_BASE_ADDR and SMC91C96_IRQ in project
#endif

static addr_t smc91c96_base = SMC91C96_BASE_ADDR;
static uint8_t mac_addr[6];
static event_t rx_event;
static void *rx_buffer;
static size_t rx_buffer_len;

#define SMC_REG16(reg) ((volatile uint16_t *)(smc91c96_base + (reg)))
#define SMC_REG8(reg) ((volatile uint8_t *)(smc91c96_base + (reg)))

static inline void smc_bank(int bank)
{
	*SMC_REG16(SMC_BSR) = bank;
}

static void smc91c96_reset(void)
{
	TRACE;

	smc_bank(0);

	*SMC_REG16(SMC_RCR) = (1<<15);
	thread_sleep(10);
	*SMC_REG16(SMC_RCR) = 0;
}

static int smc91c96_read_packet(uint8_t *data, size_t *len)
{
	int rx_fifo = (*SMC_REG16(SMC_FIFO) >> 8) & 0xf;

	LTRACEF("RCV_INT: fifo port %d\n", rx_fifo);

	// load it into the pointer register
	*SMC_REG16(SMC_PTR) = (1<<15) | (1<<14) | (1<<13) | rx_fifo;	

	uint16_t status = *SMC_REG16(SMC_DATA0);
	LTRACEF("status 0x%hx\n", status);

	uint16_t count = *SMC_REG16(SMC_DATA0) & 0x7ff;
	LTRACEF("count %d\n", count);

	// bad status?
	if (status & (0xbc00)) {
		LTRACEF("bad status\n");
		*len = 0;
		return -1;
	}

	// malformed count
	if (count <= 6 || count & 1) {
		LTRACEF("bad count\n");
		*len = 0;
		return -1;
	}

	int i;
	for (i = 0; i < (count >> 1) - 3; i++) {
		((uint16_t *)data)[i] = *SMC_REG16(SMC_DATA0);
	}

	uint16_t control_last = *SMC_REG16(SMC_DATA0);
	LTRACEF("control_last 0x%hx\n", control_last);

	if (control_last & (1<<13)) {
		// odd size, stuff the other byte
		data[count - 6] = control_last & 0xff;
		count++;
	}

	*len = count - 6;
	LTRACEF("total len %d\n", *len);

	// parse the header
	LTRACEF("ethernet type 0x%x\n", ((uint16_t *)data)[6]);

	return 0;
}

static int smc91c96_tx_packet(const uint8_t *data, size_t len)
{
	enter_critical_section();

	smc_bank(2);

	len += 6; // for our packet overhead

	// send alloc tx buffer command
	*SMC_REG16(SMC_MMUCR) = (2 << 4) | (len / 256);

	// wait for alloc
	while ((*SMC_REG8(SMC_IST) & (1<<3)) == 0)
		;

	uint8_t arr = *SMC_REG8(SMC_ARR);
	LTRACEF("ARR 0x%x\n", arr);

	// load the packet number register
	*SMC_REG8(SMC_PNR) = (arr & 0xf);

	// load our allocated fifo into the pointer register
	*SMC_REG16(SMC_PTR) = (1<<14) | (arr & 0xf);
	
	*SMC_REG16(SMC_DATA0) = 0; // status word
	*SMC_REG16(SMC_DATA0) = len & 0x7fe; // make sure it's even

	unsigned int i;
	for (i = 0; i < (len >> 1) - 3; i++) {
		*SMC_REG16(SMC_DATA0) = ((uint16_t *)data)[i];
	}

	uint16_t control_last = (1<<12); // do crc
	if (len & 1) {
		control_last |= (1<<13); // odd sized
		control_last |= data[len - 6];
	}

	*SMC_REG16(SMC_DATA0) = control_last;

	*SMC_REG16(SMC_MMUCR) = (0xc << 4); // queue tx packet

	exit_critical_section();

	return 0;
}

static enum handler_return smc91c96_interrupt(void *arg)
{
	enum handler_return ret = INT_NO_RESCHEDULE;

	smc_bank(2);
	uint8_t ist = *SMC_REG8(SMC_IST);
	uint8_t msk = *SMC_REG8(SMC_MSK);

	LTRACEF("ist 0x%x MSK 0x%x\n", ist, msk);

	// wipe out the int mask to guarantee the interrupt assert drops low
	*SMC_REG8(SMC_MSK) = 0;

	if (ist & SMC_RCV_INT) {
		// RCV INT

		if (rx_buffer != 0 && rx_buffer_len > 0) {
			size_t len;
			
			LTRACEF("reading packet into buffer at %p\n", rx_buffer);

			smc91c96_read_packet(rx_buffer, &len);

			rx_buffer_len = len;
			event_signal(&rx_event, false);

#if LOCAL_TRACE
			hexdump8(rx_buffer, len);	
#endif
			ret = INT_RESCHEDULE;

			/* dont reenable the rx interrupt at the end */
			msk &= ~SMC_RCV_INT;
		}

		*SMC_REG8(SMC_MMUCR) = (8 << 4); // remove frame from rx fifo

	}

	if (ist & SMC_TX_INT) {
		// TX INT
		LTRACEF("TX\n");

		uint16_t fifo = *SMC_REG16(SMC_FIFO);
		LTRACEF("fifo 0x%x\n", fifo);

		// write the packet number out
		*SMC_REG8(SMC_PNR) = fifo & 0xf;

		smc_bank(0);
		uint16_t ephsr = *SMC_REG16(SMC_EPHSR);
		LTRACEF("ephsr 0x%x\n", ephsr);

		smc_bank(2);
		*SMC_REG16(SMC_MMUCR) = (0xa << 4); // release specific packet

		*SMC_REG8(SMC_ACK) = SMC_TX_INT; // TX ACK

		ret = INT_RESCHEDULE;
	}

	if (ist & SMC_ALLOC_INT) {
		// allocation result

	}

	// put the old mask back, which should guarantee a new edge if we're edge triggered
	*SMC_REG8(SMC_MSK) = msk;

	return ret;
}

void smc91c96_init(void)
{
	int i;

	TRACE;

	// try to detect it
	if ((*SMC_REG16(SMC_BSR) & 0xff00) != 0x3300) {
		LTRACEF("didn't see smc91c96 chip at 0x%x\n", (unsigned int)smc91c96_base);
		return;
	}

	smc91c96_reset();

	// read revision
	smc_bank(3);
	TRACEF("detected, revision 0x%x\n", *SMC_REG16(SMC_REV));

	// memory config
	smc_bank(0);
	uint16_t mir = *SMC_REG16(SMC_MIR);
	uint16_t mcr = *SMC_REG16(SMC_MCR);
	LTRACEF("mir 0x%x, mcr 0x%x\n", mir, mcr);

	// read in the mac address
	smc_bank(1);
	for (i=0; i < 6; i++) {
		mac_addr[i] = *SMC_REG8(SMC_IAR0 + i);
	}
	TRACEF("mac address %02x:%02x:%02x:%02x:%02x:%02x\n", 
		mac_addr[0], mac_addr[1], mac_addr[2],
		mac_addr[3], mac_addr[4], mac_addr[5]);

	// set up the interrupt pin
	smc_bank(1);
	*SMC_REG16(SMC_CR) = *SMC_REG16(SMC_CR) & ~(3<<1); // clear bits 1 and 2

	// set up interrupt masks
	smc_bank(2);
	*SMC_REG8(SMC_MSK) = 0;

	smc_bank(0);

	/* initialize the rx engine */
	event_init(&rx_event, false, 0);

#if 0
	char buf[818];
	memset(buf, 0, sizeof(buf));
//	for (;;)
	smc91c96_tx_packet(buf, sizeof(buf));
	smc91c96_tx_packet(buf, sizeof(buf));
	smc91c96_tx_packet(buf, sizeof(buf));

	for(;;) {
		smc91c96_interrupt();
	}
#endif

	gpio_config(0, GPIO_INPUT);
	gpio_set_interrupt(0, GPIO_EDGE | GPIO_RISING, &smc91c96_interrupt, NULL);
}

int smc91c96_input(void *cookie, void *buf, size_t len)
{
	int ret;

	LTRACEF("buf %p, len %d\n", buf, len);

	enter_critical_section();

	event_unsignal(&rx_event);

	rx_buffer = buf;
	rx_buffer_len = len;

	/* enable the rx interrupt */
	smc_bank(2);
	*SMC_REG8(SMC_MSK) = SMC_RCV_INT;

	event_wait(&rx_event);

	ret = rx_buffer_len;

	exit_critical_section();

	return ret;
}

int smc91c96_output(void *cookie, const void *buf, size_t len)
{
	LTRACEF("buf %p, len %d\n", buf, len);

	return smc91c96_tx_packet(buf, len);
}

void smc91c96_start(void)
{
	LTRACE_ENTRY;

	enter_critical_section();

	// start the receiver
	smc_bank(0);
	*SMC_REG16(SMC_RCR) = (1<<8) | (1<<1); // RXEN, PRMS (promiscuous)

	// enable tx
	*SMC_REG16(SMC_TCR) = (1<<0); // TXEN

	// clear interrupts
	smc_bank(2);
	*SMC_REG8(SMC_MSK) = 0;
	*SMC_REG8(SMC_ACK) = 0xff;
	*SMC_REG8(SMC_MSK) = 0;

	exit_critical_section();

	/* register ourselves with the net stack */
	ifhook *hook = malloc(sizeof(ifhook));
	hook->type = IF_TYPE_ETHERNET;
	hook->mtu = ETHERNET_MAX_SIZE - ETHERNET_HEADER_SIZE;
	hook->linkaddr.len = 6;
	hook->linkaddr.type = ADDR_TYPE_ETHERNET;
	memcpy(hook->linkaddr.addr, mac_addr, 6);
	hook->cookie = 0;
	hook->if_input = smc91c96_input;
	hook->if_output = smc91c96_output;

	/* start the interface */
	ifnet *i;
	if_register_interface(hook, &i);

	LTRACE_EXIT;
}

