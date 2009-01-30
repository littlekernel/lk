/*
 * Copyright (c) 2008, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the 
 *    distribution.
 *  * Neither the name of Google, Inc. nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED 
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <debug.h>

#include <platform/iomap.h>
#include <dev/uart.h>
#include <reg.h>

#define UART_MR1         0x0000

#define UART_MR1_AUTO_RFR_LEVEL0(n) (((n) & 0x3f) << 8)
#define UART_MR1_RX_RDY_CTL    (1 << 7)
#define UART_MR1_CTS_CTL       (1 << 6)
#define UART_MR1_AUTO_RFR_LEVEL1(n) ((n) & 0x3f)

#define UART_MR2         0x0004
#define UART_MR2_ERROR_MODE        (1 << 6)
#define UART_MR2_BITS_PER_CHAR_5   (0 << 4)
#define UART_MR2_BITS_PER_CHAR_6   (1 << 4)
#define UART_MR2_BITS_PER_CHAR_7   (2 << 4)
#define UART_MR2_BITS_PER_CHAR_8   (3 << 4)
#define UART_MR2_STOP_BIT_LEN_0563 (0 << 2)
#define UART_MR2_STOP_BIT_LEN_1000 (1 << 2)
#define UART_MR2_STOP_BIT_LEN_1563 (2 << 2)
#define UART_MR2_STOP_BIT_LEN_2000 (3 << 2)
#define UART_MR2_PARITY_MODE_NONE  (0)
#define UART_MR2_PARITY_MODE_ODD   (1)
#define UART_MR2_PARITY_MODE_EVEN  (2)
#define UART_MR2_PARITY_MODE_SPACE (3)

#define UART_CSR         0x0008
#define UART_CSR_115200  0xFF
#define UART_CSR_57600   0xEE
#define UART_CSR_38400   0xDD
#define UART_CSR_19200   0xBB

#define UART_TF          0x000C

#define UART_CR          0x0010
#define UART_CR_CMD_NULL           (0 << 4)
#define UART_CR_CMD_RESET_RX       (1 << 4)
#define UART_CR_CMD_RESET_TX       (2 << 4)
#define UART_CR_CMD_RESET_ERR      (3 << 4)
#define UART_CR_CMD_RESET_BCI      (4 << 4)
#define UART_CR_CMD_START_BREAK    (5 << 4)
#define UART_CR_CMD_STOP_BREAK     (6 << 4)
#define UART_CR_CMD_RESET_CTS_N    (7 << 4)
#define UART_CR_CMD_PACKET_MODE    (9 << 4)
#define UART_CR_CMD_MODE_RESET     (12<< 4)
#define UART_CR_CMD_SET_RFR_N      (13<< 4)
#define UART_CR_CMD_RESET_RFR_ND   (14<< 4)
#define UART_CR_TX_DISABLE         (1 << 3)
#define UART_CR_TX_ENABLE          (1 << 3)
#define UART_CR_RX_DISABLE         (1 << 3)
#define UART_CR_RX_ENABLE          (1 << 3)

#define UART_IMR         0x0014
#define UART_IMR_RXLEV (1 << 4)
#define UART_IMR_TXLEV (1 << 0)

#define UART_IPR         0x0018
#define UART_TFWR        0x001C
#define UART_RFWR        0x0020
#define UART_HCR         0x0024

#define UART_MREG        0x0028
#define UART_NREG        0x002C
#define UART_DREG        0x0030
#define UART_MNDREG      0x0034
#define UART_IRDA        0x0038
#define UART_MISR_MODE   0x0040
#define UART_MISR_RESET  0x0044
#define UART_MISR_EXPORT 0x0048
#define UART_MISR_VAL    0x004C
#define UART_TEST_CTRL   0x0050

#define UART_SR          0x0008
#define UART_SR_HUNT_CHAR      (1 << 7)
#define UART_SR_RX_BREAK       (1 << 6)
#define UART_SR_PAR_FRAME_ERR  (1 << 5)
#define UART_SR_OVERRUN        (1 << 4)
#define UART_SR_TX_EMPTY       (1 << 3)
#define UART_SR_TX_READY       (1 << 2)
#define UART_SR_RX_FULL        (1 << 1)
#define UART_SR_RX_READY       (1 << 0)

#define UART_RF          0x000C
#define UART_MISR        0x0010
#define UART_ISR         0x0014


static unsigned uart_ready = 0;
static unsigned uart_base = MSM_UART3_BASE;

#define uwr(v,a) writel(v, uart_base + (a))
#define urd(a) readl(uart_base + (a))

void uart_init(void)
{
	uwr(0x0A, UART_CR);  /* disable TX and RX */
	
	uwr(0x30, UART_CR);  /* reset error status */
	uwr(0x10, UART_CR);  /* reset receiver */
	uwr(0x20, UART_CR);  /* reset transmitter */
	
#if PLATFORM_QSD8K
	/* TCXO */
	uwr(0x06, UART_MREG);
	uwr(0xF1, UART_NREG);
	uwr(0x0F, UART_DREG);
	uwr(0x1A, UART_MNDREG);
#else
	/* TCXO/4 */
	uwr(0xC0, UART_MREG);
	uwr(0xAF, UART_NREG);
	uwr(0x80, UART_DREG);
	uwr(0x19, UART_MNDREG);    
#endif
	
	uwr(0x10, UART_CR);  /* reset RX */
	uwr(0x20, UART_CR);  /* reset TX */
	uwr(0x30, UART_CR);  /* reset error status */
	uwr(0x40, UART_CR);  /* reset RX break */
	uwr(0x70, UART_CR);  /* rest? */
	uwr(0xD0, UART_CR);  /* reset */
	
	uwr(0x7BF, UART_IPR); /* stale timeout = 630 * bitrate */
	uwr(0, UART_IMR);
	uwr(115, UART_RFWR); /* RX watermark = 58 * 2 - 1 */
	uwr(10, UART_TFWR);  /* TX watermark */
	
	uwr(0, UART_RFWR); 
	
	uwr(UART_CSR_115200, UART_CSR);
	uwr(0, UART_IRDA);
	uwr(0x1E, UART_HCR);
//	uwr(0x7F4, UART_MR1); /* RFS/ CTS/ 500chr RFR */
	uwr(16, UART_MR1);
	uwr(0x34, UART_MR2); /* 8N1 */
	
	uwr(0x05, UART_CR); /* enable TX & RX */

	uart_ready = 1;
}

int uart_putc(int port, char c)
{
	if (!uart_ready)
		return -1;
	while (!(urd(UART_SR) & UART_SR_TX_READY)) ;
	uwr(c, UART_TF);
	return 0;
}

int uart_getc(int port, bool wait)
{
	if (!uart_ready)
		return -1;
	while (!(urd(UART_SR) & UART_SR_RX_READY))
		if (!wait)
			return -1;

	return urd(UART_RF);
}

