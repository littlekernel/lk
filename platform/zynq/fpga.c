/*
 * Copyright (c) 2014 Brian Swetland
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

#include <platform/fpga.h>
#include <reg.h>

#define DEVCFG_CTRL		0xF8007000
#define  PCFG_PROG_B		(1 << 30)
#define  PCFG_POR_CNT_4K	(1 << 29)
#define  PCAP_PR		(1 << 27) // 0 = ICAP CFG, 1 = PCAP CFG
#define  PCAP_MODE		(1 << 26) // 1 = Enable PCAP Interface
#define DEVCFG_LOCK		0xF8007004
#define DEVCFG_CFG		0xF8007008
#define DEVCFG_INT_STS		0xF800700C
#define  PSS_CFG_RESET_B	(1 << 5) // 1 = PL in reset state
#define  PCFG_DONE_INT		(1 << 2) // 1 = PL successfully programmed
#define  PCFG_INIT_PE_INT	(1 << 1)
#define  PCFG_INIT_NE_INT	(1 << 0)
#define DEVCFG_INT_MASK		0xF8007010
#define DEVCFG_STATUS		0xF8007014
#define  PCFG_INIT		(1 << 4) // 1 = ready for bitstream
#define DEVCFG_DMA_SRC_ADDR	0xF8007018
#define DEVCFG_DMA_DST_ADDR	0xF800701C
#define DEVCFG_DMA_SRC_LEN	0xF8007020 // words
#define DEVCFG_DMA_DST_LEN	0xF8007024 // words
#define DEVCFG_SW_ID		0xF8007030
#define DEVCFG_MCTRL		0xF8007080
#define  PCFG_POR_B		(1 << 8) // 1 = PL is powered on
#define  INT_PCAP_LPBK		(1 << 4) // 1 = Loopback Enabled


// Per Zynq TRM, 6.4.4
// 1. wait for PCFG_INIT==1
// 2. disable loopback
// 3. set DEVCFG CTRL PCAP_PR and PCAP_MODE
// 4. set dma src, dst, srclen, dstlen (in that specific order)
// 5. wait for PCFG_DONE_INT==1

void zynq_program_fpga(u32 physaddr, u32 length) {
	while(!(readl(DEVCFG_STATUS) & PCFG_INIT)) ;
	writel(readl(DEVCFG_CTRL) | PCAP_PR | PCAP_MODE, DEVCFG_CTRL);
	writel(readl(DEVCFG_MCTRL) & (~INT_PCAP_LPBK), DEVCFG_MCTRL);
	writel(physaddr, DEVCFG_DMA_SRC_ADDR);
	writel(0xFFFFFFFF, DEVCFG_DMA_DST_ADDR);
	writel(length, DEVCFG_DMA_SRC_LEN);
	writel(length, DEVCFG_DMA_DST_LEN);
	while (!(readl(DEVCFG_INT_STS) & PCFG_DONE_INT)) ;
}

void zynq_reset_fpga(void) {
	writel(readl(DEVCFG_CTRL) & (~PCFG_PROG_B), DEVCFG_CTRL);
	writel(readl(DEVCFG_CTRL) | PCFG_PROG_B, DEVCFG_CTRL);
}

