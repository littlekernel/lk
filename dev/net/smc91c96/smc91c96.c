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
#include <sys/types.h>
#include <debug.h>
#include <printf.h>
#include <kernel/thread.h>
#include <dev/net/smc91c96.h>
#include "smc91c96_p.h"

#if !defined(SMC91C96_BASE_ADDR) || !defined(SMC91C96_IRQ)
#error need to define SMC91C96_BASE_ADDR and SMC91C96_IRQ in project
#endif

static addr_t smc91c96_base = SMC91C96_BASE_ADDR;
static uint8_t mac_addr[6];

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

	*SMC_REG16(SMC_RCR) = (1<<16);
	thread_sleep(10);
	*SMC_REG16(SMC_RCR) = 0;
}

static int smc91c96_read_packet(uint8_t *data, size_t *len)
{
	int rx_fifo = (*SMC_REG16(SMC_FIFO) >> 8) & 0xf;

	TRACEF("RCV_INT: fifo port %d\n", rx_fifo);

	// load it into the pointer register
	*SMC_REG16(SMC_PTR) = (1<<15) | (1<<14) | (1<<13) | rx_fifo;	

	uint16_t status = *SMC_REG16(SMC_DATA0);
	TRACEF("status 0x%hx\n", status);

	uint16_t count = *SMC_REG16(SMC_DATA0) & 0x7ff;
	TRACEF("count %d\n", count);

	// bad status?
	if (status & (0xbc00)) {
		TRACEF("bad status\n");
		*len = 0;
		return -1;
	}

	// malformed count
	if (count <= 6 || count & 1) {
		TRACEF("bad count\n");
		*len = 0;
		return -1;
	}

	int i;
	for (i = 0; i < (count >> 1) - 3; i++) {
		((uint16_t *)data)[i] = *SMC_REG16(SMC_DATA0);
	}

	uint16_t control_last = *SMC_REG16(SMC_DATA0);
	TRACEF("control_last 0x%hx\n", control_last);

	if (control_last & (1<<13)) {
		// odd size, stuff the other byte
		data[count - 6] = control_last & 0xff;
		count++;
	}

	*len = count - 6;
	TRACEF("total len %d\n", *len);

	// parse the header
	TRACEF("ethernet type 0x%x\n", ((uint16_t *)data)[6]);

	return 0;
}

static int smc91c96_tx_packet(const uint8_t *data, size_t len)
{
	smc_bank(2);

	len += 6; // for our packet overhead

	// send alloc tx buffer command
	*SMC_REG16(SMC_MMUCR) = (2 << 4) | (len / 256);

	// wait for alloc
	while ((*SMC_REG8(SMC_IST) & (1<<3)) == 0)
		;

	uint8_t arr = *SMC_REG8(SMC_ARR);
	TRACEF("ARR 0x%x\n", arr);

	// load the packet number register
	*SMC_REG8(SMC_PNR) = (arr & 0xf);

	// load our allocated fifo into the pointer register
	*SMC_REG16(SMC_PTR) = (1<<14) | (arr & 0xf);
	
	*SMC_REG16(SMC_DATA0) = 0; // status word
	*SMC_REG16(SMC_DATA0) = len & 0x7fe; // make sure it's even

	int i;
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

	return 0;
}

static int smc91c96_interrupt(void)
{
	smc_bank(2);
	uint8_t ist = *SMC_REG8(SMC_IST);

	if (ist & (1<<0)) {
		// RCV INT
		uint8_t buf[1600];
		size_t len;
		smc91c96_read_packet(buf, &len);

		hexdump8(buf, len);	

		*SMC_REG8(SMC_MMUCR) = (8 << 4); // remove frame from rx fifo
	}

	if (ist & (1<<1)) {
		// TX INT
		TRACEF("TX\n");

		uint16_t fifo = *SMC_REG16(SMC_FIFO);
		TRACEF("fifo 0x%x\n", fifo);

		// write the packet number out
		*SMC_REG8(SMC_PNR) = fifo & 0xf;

		smc_bank(0);
		uint16_t ephsr = *SMC_REG16(SMC_EPHSR);
		TRACEF("ephsr 0x%x\n", ephsr);

		smc_bank(2);
		*SMC_REG16(SMC_MMUCR) = (0xa << 4); // release specific packet

		*SMC_REG8(SMC_ACK) = (1<<1); // TX ACK
	}

	if (ist & (1<<4)) {
		// allocation result

	}
}

void smc91c96_init(void)
{
	int i;

	TRACE;

	// try to detect it
	if ((*SMC_REG16(SMC_BSR) & 0xff00) != 0x3300) {
		TRACEF("didn't see smc91c96 chip at 0x%x\n", (unsigned int)smc91c96_base);
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
	TRACEF("mir 0x%x, mcr 0x%x\n", mir, mcr);

	// read in the mac address
	smc_bank(1);
	for (i=0; i < 6; i++) {
		mac_addr[i] = *SMC_REG8(SMC_IAR0 + i);
	}
	TRACEF("mac address %02x:%02x:%02x:%02x:%02x:%02x\n", 
		mac_addr[0], mac_addr[1], mac_addr[2],
		mac_addr[3], mac_addr[4], mac_addr[5]);

	smc_bank(0);

	// start the receiver
	smc_bank(0);
	*SMC_REG16(SMC_RCR) = (1<<8) | (1<<1); // RXEN, PRMS (promiscuous)

	// enable tx
	*SMC_REG16(SMC_TCR) = (1<<0); // TXEN

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
}

