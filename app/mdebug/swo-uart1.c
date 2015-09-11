/* swo-uart1.c
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

#include <string.h>
#include <debug.h>
#include <reg.h>
#include <kernel/thread.h>

#include <dev/udc.h>
#include <arch/arm/cm.h>

#include <platform/lpc43xx-uart.h>
#include <platform/lpc43xx-gpdma.h>
#include <platform/lpc43xx-clocks.h>

#include "rswdp.h"

#define UART_BASE	UART1_BASE
#define BASE_UART_CLK	BASE_UART1_CLK

extern uint8_t __lpc43xx_main_clock_sel;
extern uint32_t __lpc43xx_main_clock_mhz;

#define TXNSIZE		128
#define TXNCOUNT	4

typedef struct swo_txn {
	unsigned buf[TXNSIZE/4 + 2];
	struct swo_txn *next;
	udc_request_t *req;
	unsigned busy;
	unsigned num;
} txn_t;

static txn_t TXN[TXNCOUNT];
static txn_t *txwr = TXN;
static udc_endpoint_t *txept;

void swo_start_dma(void *ptr);

static void tx_done(udc_request_t *req, unsigned actual, int status) {
	txn_t *txn = req->context;
	txn->busy = 0;
	if (txwr == txn) {
		// writer wants to write here, and is waiting...
		swo_start_dma(txn->buf + 2);
	}
}

void lpc43xx_DMA_IRQ(void) {
	txn_t *txn = txwr;
	arm_cm_irq_entry();
	writel(0xFF, DMA_INTTCCLR);
	writel(0xFF, DMA_INTERRCLR);
	if (udc_request_queue(txept, txn->req)) {
		// failed, usb probably offline, just re-use the buffer
	} else {
		txn->busy = 1;
		txwr = txn = txn->next;
	}
	if (!txn->busy) {
		// if busy, when the usb txn completes, it will start dma then
		swo_start_dma(txn->buf + 2);
	}
	arm_cm_irq_exit(0);
}

void swo_init(udc_endpoint_t *_txept) {
	int n;
	txept = _txept;
	for (n = 0; n < TXNCOUNT; n++) {
		TXN[n].req = udc_request_alloc();
		TXN[n].req->context = TXN + n;
		TXN[n].req->buffer = TXN[n].buf;
		TXN[n].req->length = TXNSIZE + 8;
		TXN[n].req->complete = tx_done;
		TXN[n].num = n;
		TXN[n].busy = 0;
		TXN[n].next = TXN + (n + 1);
		TXN[n].buf[0] = RSWD_TXN_ASYNC;
		TXN[n].buf[1] = RSWD_MSG(CMD_SWO_DATA, 0, TXNSIZE);
	}
	TXN[n-1].next = TXN;

	// configure peripheral 4 as uart1_rx
	writel((readl(DMAMUX_REG) & DMAMUX_M(4)) | DMAMUX_P(4, P4_UART1_RX), DMAMUX_REG);
	writel(DMA_CONFIG_EN, DMA_CONFIG);
	NVIC_EnableIRQ(DMA_IRQn);

	// kick off the process with an initial DMA
	swo_start_dma(txwr->buf + 2);
}

void swo_start_dma(void *ptr) {
	writel(UART1_BASE + REG_RBR, DMA_SRC(0));
	writel((u32) ptr, DMA_DST(0));
	writel(0, DMA_LLI(0));
	writel(DMA_XFER_SIZE(TXNSIZE) |
		DMA_SRC_BURST(BURST_1) | DMA_DST_BURST(BURST_4) |
		DMA_SRC_BYTE | DMA_DST_WORD | DMA_SRC_MASTER1 | DMA_DST_MASTER0 |
		DMA_DST_INCR | DMA_PROT1 | DMA_TC_IE,
		DMA_CTL(0));
	writel(DMA_ENABLE | DMA_SRC_PERIPH(4) | DMA_FLOW_P2M_DMAc | DMA_TC_IRQ_EN,
		DMA_CFG(0));
}
void swo_config(unsigned mhz) {
	if (mhz > 0) {
		uint32_t div = __lpc43xx_main_clock_mhz / 16 / mhz;
		writel(BASE_CLK_SEL(__lpc43xx_main_clock_sel), BASE_UART_CLK);
		writel(LCR_DLAB, UART_BASE + REG_LCR);
		writel(div & 0xFF, UART_BASE + REG_DLL);
		writel((div >> 8) & 0xFF, UART_BASE + REG_DLM);
		writel(LCR_WLS_8 | LCR_SBS_1, UART_BASE + REG_LCR);
		writel(FCR_FIFOEN | FCR_RX_TRIG_1 | FCR_DMAMODE, UART_BASE + REG_FCR);
	}
}

unsigned swo_set_clock(unsigned khz) {
	if (khz >= 12000) {
		khz = 12000;
	} else if (khz >= 8000) {
		khz = 8000;
	} else if (khz >= 6000) {
		khz = 6000;
	} else if (khz >= 4000) {
		khz = 4000;
	} else if (khz >= 3000) {
		khz = 3000;
	} else if (khz >= 2000) {
		khz = 2000;
	} else {
		khz = 1000;
	}
	swo_config(khz * 1000);
	return khz;
}

