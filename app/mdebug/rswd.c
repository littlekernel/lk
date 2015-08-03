/* rswd.c
 *
 * Copyright 2011-2015 Brian Swetland <swetland@frotz.net>
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

#include <reg.h>
#include <debug.h>
#include <string.h>
#include <stdlib.h>
#include <printf.h>

#include "swd.h"
#include "rswdp.h"

void usb_xmit(void *data, unsigned len);
unsigned usb_recv(void *data, unsigned len);

unsigned swdp_trace = 0;

static u8 optable[16] = {
	[OP_RD | OP_DP | OP_X0] = RD_IDCODE,
	[OP_RD | OP_DP | OP_X4] = RD_DPCTRL,
	[OP_RD | OP_DP | OP_X8] = RD_RESEND,
	[OP_RD | OP_DP | OP_XC] = RD_BUFFER,
	[OP_WR | OP_DP | OP_X0] = WR_ABORT,
	[OP_WR | OP_DP | OP_X4] = WR_DPCTRL,
	[OP_WR | OP_DP | OP_X8] = WR_SELECT,
	[OP_WR | OP_DP | OP_XC] = WR_BUFFER,
	[OP_RD | OP_AP | OP_X0] = RD_AP0,
	[OP_RD | OP_AP | OP_X4] = RD_AP1,
	[OP_RD | OP_AP | OP_X8] = RD_AP2,
	[OP_RD | OP_AP | OP_XC] = RD_AP3,
	[OP_WR | OP_AP | OP_X0] = WR_AP0,
	[OP_WR | OP_AP | OP_X4] = WR_AP1,
	[OP_WR | OP_AP | OP_X8] = WR_AP2,
	[OP_WR | OP_AP | OP_XC] = WR_AP3,
};

/* TODO bounds checking -- we trust the host far too much */
void process_txn(u32 txnid, u32 *rx, int rxc, u32 *tx) {
	unsigned msg, op, n;
	unsigned txc = 1;
	unsigned count = 0;
	unsigned status = 0;
	void (*func)(void) = 0;

	tx[0] = txnid;

	while (rxc-- > 0) {
		count++;
		msg = *rx++;
		op = RSWD_MSG_OP(msg);
		n = RSWD_MSG_ARG(msg);
#if CONFIG_MDEBUG_TRACE
		printf("> %02x %02x %04x <\n", RSWD_MSG_CMD(msg), op, n);
#endif
		switch (RSWD_MSG_CMD(msg)) {
		case CMD_NULL:
			continue;
		case CMD_SWD_WRITE:
			while (n-- > 0) {
				rxc--;
				if (swd_write(optable[op], *rx++)) {
					status = 3;
					goto done;
				}
			}
			continue;
		case CMD_SWD_READ:
			tx[txc++] = RSWD_MSG(CMD_SWD_DATA, 0, n); 
			while (n-- > 0) {
				if (swd_read(optable[op], tx + txc)) {
					txc++;
					while (n-- > 0)
						tx[txc++] = 0xfefefefe;
					status = 3;
					goto done;
				}
				txc++;
			}
			continue;
		case CMD_SWD_DISCARD:
			while (n-- > 0) {
				u32 tmp;
				if (swd_read(optable[op], &tmp)) {
					status = 3;
					goto done;
				}
			}
			continue;
		case CMD_ATTACH:
			swd_reset();
			continue;
		case CMD_RESET:
			swd_hw_reset(n);
			continue;
		case CMD_DOWNLOAD: {
			//u32 *addr = (void*) *rx++;
			rxc--;
			while (n) {
				//*addr++ = *rx++;
				rx++;
				rxc--;
			}
			continue;
		}
		case CMD_EXECUTE:
			//func = (void*) *rx++;
			rxc--;
			continue;
		case CMD_TRACE:
			swdp_trace = op;
			continue;
		case CMD_BOOTLOADER:
			//func = reboot_bootloader;
			continue;
		case CMD_SET_CLOCK:
			n = swd_set_clock(n);
			printf("swdp clock is now %d KHz\n", n);
			continue;
		default:
			printf("unknown command %02x\n", RSWD_MSG_CMD(msg));
			status = 1;
			goto done;
		}
	}

done:
	tx[txc++] = RSWD_MSG(CMD_STATUS, status, count);

	/* if we're about to send an even multiple of the packet size
	 * (64), add a NULL op on the end to create a short packet at
	 * the end.
	 */
	if ((txc & 0xf) == 0)
		tx[txc++] = RSWD_MSG(CMD_NULL, 0, 0);

#if CONFIG_MDEBUG_TRACE
	printf("[ send %d words ]\n", txc);
	for (n = 0; n < txc; n+=4) {
		printx("%08x %08x %08x %08x\n",
			tx[n], tx[n+1], tx[n+2], tx[n+3]);
	}
#endif
	usb_xmit(tx, txc * 4);

	if (func) {
		for (n = 0; n < 1000000; n++) asm("nop");
		func();
		for (;;) ;
	}	
}

// io buffers in AHB SRAM
static u32 *rxbuffer = (void*) 0x20001000;
static u32 *txbuffer[2] = {(void*) 0x20003000, (void*) 0x20005000 };

#include <kernel/thread.h>

void handle_rswd(void) {
	int rxc;
	int toggle = 0;

#if CONFIG_MDEBUG_TRACE
	printf("[ rswdp agent v0.9 ]\n");
	printf("[ built " __DATE__ " " __TIME__ " ]\n");
#endif

	for (;;) {
		rxc = usb_recv(rxbuffer, 4096);

#if CONFIG_MDEBUG_TRACE
		int n;
		printx("[ recv %d words ]\n", rxc/4);
		for (n = 0; n < (rxc/4); n+=4) {
			printx("%08x %08x %08x %08x\n",
				rxbuffer[n], rxbuffer[n+1],
				rxbuffer[n+2], rxbuffer[n+3]);
		}
#endif

		if ((rxc < 4) || (rxc & 3)) {
			printf("error, runt frame, or strange frame... %x\n", rxc);
			continue;
		}

		rxc = rxc / 4;

		if ((rxbuffer[0] & 0xFFFF0000) != 0xAA770000) {
			printf("invalid frame %x\n", rxbuffer[0]);
			continue;
		}

		process_txn(rxbuffer[0], rxbuffer + 1, rxc - 1, txbuffer[toggle]);
		toggle ^= 1;
	}
}
