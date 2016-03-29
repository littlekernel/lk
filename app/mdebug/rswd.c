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

#include <platform.h>

#include "swd.h"
#include "rswdp.h"

void usb_xmit(void *data, unsigned len);
int usb_recv(void *data, unsigned len);

unsigned swdp_trace = 0;

// indicates host knows about v1.0 protocol features
unsigned host_version = 0;

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

static const char *board_str = TARGET;
static const char *build_str = "fw v0.91 (" __DATE__ ", " __TIME__ ")";

static void _reboot(void) {
	platform_halt(HALT_ACTION_REBOOT, HALT_REASON_SW_RESET);
}

#define MODE_SWD	0
#define MODE_JTAG	1
static unsigned mode = MODE_SWD;

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
				status = swd_write(optable[op], *rx++);
				if (status) {
					goto done;
				}
			}
			continue;
		case CMD_SWD_READ:
			tx[txc++] = RSWD_MSG(CMD_SWD_DATA, 0, n);
			while (n-- > 0) {
				status = swd_read(optable[op], tx + txc);
				if (status) {
					txc++;
					while (n-- > 0)
						tx[txc++] = 0xfefefefe;
					goto done;
				}
				txc++;
			}
			continue;
		case CMD_SWD_DISCARD:
			while (n-- > 0) {
				u32 tmp;
				status = swd_read(optable[op], &tmp);
				if (status) {
					goto done;
				}
			}
			continue;
		case CMD_ATTACH:
			if (mode != MODE_SWD) {
				mode = MODE_SWD;
				swd_init();
			}
			swd_reset();
			continue;
		case CMD_JTAG_IO:
			if (mode != MODE_JTAG) {
				mode = MODE_JTAG;
				jtag_init();
			}
			tx[txc++] = RSWD_MSG(CMD_JTAG_DATA, 0, n);
			while (n > 0) {
				unsigned xfer = (n > 32) ? 32 : n;
				jtag_io(xfer, rx[0], rx[1], tx + txc);
				rx += 2;
				rxc -= 2;
				txc += 1;
				n -= xfer;
			}
			continue;
		case CMD_JTAG_VRFY:
			if (mode != MODE_JTAG) {
				mode = MODE_JTAG;
				jtag_init();
			}
			// (n/32) x 4 words: TMS, TDI, DATA, MASK
			while (n > 0) {
				unsigned xfer = (n > 32) ? 32 : n;
				jtag_io(xfer, rx[0], rx[1], tx + txc);
				if ((tx[txc] & rx[3]) != rx[2]) {
					status = ERR_BAD_MATCH;
					goto done;
				}
				rx += 4;
				rxc -= 4;
				n -= xfer;
			}
			continue;
		case CMD_JTAG_TX: {
			unsigned tms = (op & 1) ? 0xFFFFFFFF : 0;
			if (mode != MODE_JTAG) {
				mode = MODE_JTAG;
				jtag_init();
			}
			while (n > 0) {
				unsigned xfer = (n > 32) ? 32 : n;
				jtag_io(xfer, tms, rx[0], rx);
				rx++;
				rxc--;
				n -= xfer;
			}
			continue;
		}
		case CMD_JTAG_RX: {
			unsigned tms = (op & 1) ? 0xFFFFFFFF : 0;
			unsigned tdi = (op & 2) ? 0xFFFFFFFF : 0;
			if (mode != MODE_JTAG) {
				mode = MODE_JTAG;
				jtag_init();
			}
			tx[txc++] = RSWD_MSG(CMD_JTAG_DATA, 0, n);
			while (n > 0) {
				unsigned xfer = (n > 32) ? 32 : n;
				jtag_io(xfer, tms, tdi, tx + txc);
				txc++;
				n -= xfer;
			}
			continue;
		}
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
			func = _reboot;
			continue;
		case CMD_SET_CLOCK:
			n = swd_set_clock(n);
			printf("swdp clock is now %d KHz\n", n);
			if (host_version >= RSWD_VERSION_1_0) {
				tx[txc++] = RSWD_MSG(CMD_CLOCK_KHZ, 0, n);
			}
			continue;
		case CMD_SWO_CLOCK:
			n = swo_set_clock(n);
			printf("swo clock is now %d KHz\n", n);
			continue;
		case CMD_VERSION:
			host_version = n;
			tx[txc++] = RSWD_MSG(CMD_VERSION, 0, RSWD_VERSION);

			n = strlen(board_str);
			memcpy(tx + txc + 1, board_str, n + 1);
			n = (n + 4) / 4;
			tx[txc++] = RSWD_MSG(CMD_BOARD_STR, 0, n);
			txc += n;

			n = strlen(build_str);
			memcpy(tx + txc + 1, build_str, n + 1);
			n = (n + 4) / 4;
			tx[txc++] = RSWD_MSG(CMD_BUILD_STR, 0, n);
			txc += n;

			tx[txc++] = RSWD_MSG(CMD_RX_MAXDATA, 0, 8192);
			txc += n;
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
		rxc = usb_recv(rxbuffer, 8192);

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
			printf("error, runt frame, or strange frame... %d\n", rxc);
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
