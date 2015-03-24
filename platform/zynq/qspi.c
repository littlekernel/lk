/*
 * Copyright (c) 2014 Brian Swetland
 * Copyright (c) 2014 Travis Geiselbrecht
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
#include <dev/qspi.h>

#include <debug.h>
#include <assert.h>
#include <compiler.h>
#include <printf.h>
#include <string.h>
#include <reg.h>

#include <lib/console.h>

#include <platform/zynq.h>

#define QSPI_CONFIG             0xE000D000
#define  CFG_IFMODE             (1 << 31) // Inteligent Flash Mode
#define  CFG_LITTLE_ENDIAN      (0 << 26)
#define  CFG_BIG_ENDIAN         (1 << 26)
#define  CFG_HOLDB_DR           (1 << 19) // set to 1 for dual/quad spi mode
#define  CFG_NO_MODIFY_MASK     (1 << 17) // do not modify this bit
#define  CFG_MANUAL_START       (1 << 16) // start transaction
#define  CFG_MANUAL_START_EN    (1 << 15) // enable manual start mode
#define  CFG_MANUAL_CS_EN       (1 << 14) // enable manual CS control
#define  CFG_MANUAL_CS          (1 << 10) // directly drives n_ss_out if MANUAL_CS_EN==1
#define  CFG_FIFO_WIDTH_32      (3 << 6)  // only valid setting
#define  CFG_BAUD_MASK          (7 << 3)
#define  CFG_BAUD_DIV_2         (0 << 3)
#define  CFG_BAUD_DIV_4         (1 << 3)
#define  CFG_BAUD_DIV_8         (2 << 3)
#define  CFG_BAUD_DIV_16        (3 << 3)
#define  CFG_CPHA               (1 << 2) // clock phase
#define  CFG_CPOL               (1 << 1) // clock polarity
#define  CFG_MASTER_MODE        (1 << 0) // only valid setting

#define QSPI_IRQ_STATUS         0xE000D004 // ro status (write UNDERFLOW/OVERFLOW to clear)
#define QSPI_IRQ_ENABLE         0xE000D008 // write 1s to set mask bits
#define QSPI_IRQ_DISABLE        0xE000D00C // write 1s to clear mask bits
#define QSPI_IRQ_MASK           0xE000D010 // ro mask value (1 = irq enabled)
#define  TX_UNDERFLOW           (1 << 6)
#define  RX_FIFO_FULL           (1 << 5)
#define  RX_FIFO_NOT_EMPTY      (1 << 4)
#define  TX_FIFO_FULL           (1 << 3)
#define  TX_FIFO_NOT_FULL       (1 << 2)
#define  RX_OVERFLOW            (1 << 0)

#define QSPI_ENABLE             0xE000D014 // write 1 to enable

#define QSPI_DELAY              0xE000D018
#define QSPI_TXD0               0xE000D01C
#define QSPI_RXDATA             0xE000D020
#define QSPI_SLAVE_IDLE_COUNT   0xE000D024
#define QSPI_TX_THRESHOLD       0xE000D028
#define QSPI_RX_THRESHOLD       0xE000D02C
#define QSPI_GPIO               0xE000D030
#define QSPI_LPBK_DLY_ADJ       0xE000D038
#define QSPI_TXD1               0xE000D080
#define QSPI_TXD2               0xE000D084
#define QSPI_TXD3               0xE000D088

#define QSPI_LINEAR_CONFIG      0xE000D0A0
#define  LCFG_ENABLE            (1 << 31) // enable linear quad spi mode
#define  LCFG_TWO_MEM           (1 << 30)
#define  LCFG_SEP_BUS           (1 << 29) // 0=shared 1=separate
#define  LCFG_U_PAGE            (1 << 28)
#define  LCFG_MODE_EN           (1 << 25) // send mode bits (required for dual/quad io)
#define  LCFG_MODE_ON           (1 << 24) // only send instruction code for first read
#define  LCFG_MODE_BITS(n)      (((n) & 0xFF) << 16)
#define  LCFG_DUMMY_BYTES(n)    (((n) & 7) << 8)
#define  LCFG_INST_CODE(n)      ((n) & 0xFF)

#define QSPI_LINEAR_STATUS      0xE000D0A4
#define QSPI_MODULE_ID          0xE000D0FC

int qspi_set_speed(struct qspi_ctxt *qspi, uint32_t khz)
{
	uint32_t n;

	if (khz >= 100000) {
		n = CFG_BAUD_DIV_2;
		khz = 100000;
	} else if (khz >= 50000) {
		n = CFG_BAUD_DIV_4;
		khz = 50000;
	} else if (khz >= 25000) {
		n = CFG_BAUD_DIV_8;
		khz = 25000;
	} else {
		return -1;
	}

	if (khz == qspi->khz)
		return 0;

	qspi->khz = khz;

	writel(0, QSPI_ENABLE);
	if (n == CFG_BAUD_DIV_2) {
		writel(0x20, QSPI_LPBK_DLY_ADJ);
	} else {
		writel(0, QSPI_LPBK_DLY_ADJ);
	}

	qspi->cfg &= ~CFG_BAUD_MASK;
	qspi->cfg |= n;

	writel(qspi->cfg, QSPI_CONFIG);
	writel(1, QSPI_ENABLE);

	return 0;
}

int qspi_init(struct qspi_ctxt *qspi, uint32_t khz)
{
	writel(0, QSPI_ENABLE);
	writel(0, QSPI_LINEAR_CONFIG);

	// flush rx fifo
	while (readl(QSPI_IRQ_STATUS) & RX_FIFO_NOT_EMPTY)
		readl(QSPI_RXDATA);

	qspi->cfg = (readl(QSPI_CONFIG) & CFG_NO_MODIFY_MASK) |
	            CFG_IFMODE |
	            CFG_HOLDB_DR |
	            CFG_FIFO_WIDTH_32 |
	            CFG_CPHA | CFG_CPOL |
	            CFG_MASTER_MODE |
	            CFG_BAUD_DIV_2 |
	            CFG_MANUAL_START_EN | CFG_MANUAL_CS_EN | CFG_MANUAL_CS;

	writel(qspi->cfg, QSPI_CONFIG);
	qspi->khz = 100000;
	qspi->linear_mode = false;

	writel(1, QSPI_ENABLE);

	// clear sticky irqs
	writel(TX_UNDERFLOW | RX_OVERFLOW, QSPI_IRQ_STATUS);

	return 0;
}

int qspi_enable_linear(struct qspi_ctxt *qspi)
{
	if (qspi->linear_mode)
		return 0;

	/* disable the controller */
	writel(0, QSPI_ENABLE);
	writel(0, QSPI_LINEAR_CONFIG);

	/* put the controller in auto chip select mode and assert chip select */
	qspi->cfg &= ~(CFG_MANUAL_START_EN | CFG_MANUAL_CS_EN | CFG_MANUAL_CS);
	writel(qspi->cfg, QSPI_CONFIG);

#if 1
	// uses Quad I/O mode
	// should be 0x82FF02EB according to xilinx manual for spansion flashes
	writel(LCFG_ENABLE |
			LCFG_MODE_EN |
			LCFG_MODE_BITS(0xff) |
			LCFG_DUMMY_BYTES(2) |
			LCFG_INST_CODE(0xeb),
			QSPI_LINEAR_CONFIG);
#else
	// uses Quad Output Read mode
	// should be 0x8000016B according to xilinx manual for spansion flashes
	writel(LCFG_ENABLE |
			LCFG_MODE_BITS(0) |
			LCFG_DUMMY_BYTES(1) |
			LCFG_INST_CODE(0x6b),
			QSPI_LINEAR_CONFIG);
#endif

	/* enable the controller */
	writel(1, QSPI_ENABLE);

	qspi->linear_mode = true;

	DSB;

	return 0;
}

int qspi_disable_linear(struct qspi_ctxt *qspi)
{
	if (!qspi->linear_mode)
		return 0;

	/* disable the controller */
	writel(0, QSPI_ENABLE);
	writel(0, QSPI_LINEAR_CONFIG);

	/* put the controller back into manual chip select mode */
	qspi->cfg |= (CFG_MANUAL_START_EN | CFG_MANUAL_CS_EN | CFG_MANUAL_CS);
	writel(qspi->cfg, QSPI_CONFIG);

	/* enable the controller */
	writel(1, QSPI_ENABLE);

	qspi->linear_mode = false;

	DSB;

	return 0;
}

void qspi_cs(struct qspi_ctxt *qspi, unsigned int cs)
{
	DEBUG_ASSERT(cs <= 1);

	if (cs == 0)
		qspi->cfg &= ~(CFG_MANUAL_CS);
	else
		qspi->cfg |= CFG_MANUAL_CS;
	writel(qspi->cfg, QSPI_CONFIG);
}

static inline void qspi_xmit(struct qspi_ctxt *qspi)
{
	// start txn
	writel(qspi->cfg | CFG_MANUAL_START, QSPI_CONFIG);

	// wait for command to transmit and TX fifo to be empty
	while ((readl(QSPI_IRQ_STATUS) & TX_FIFO_NOT_FULL) == 0) ;
}

static inline void qspi_flush_rx(void)
{
	while (!(readl(QSPI_IRQ_STATUS) & RX_FIFO_NOT_EMPTY)) ;
	readl(QSPI_RXDATA);
}

static const uint32_t TXFIFO[] = { QSPI_TXD1, QSPI_TXD2, QSPI_TXD3, QSPI_TXD0, QSPI_TXD0, QSPI_TXD0 };

void qspi_rd(struct qspi_ctxt *qspi, uint32_t cmd, uint32_t asize, uint32_t *data, uint32_t count)
{
	uint32_t sent = 0;
	uint32_t rcvd = 0;

	DEBUG_ASSERT(qspi);
	DEBUG_ASSERT(asize < 6);

	qspi_cs(qspi, 0);

	writel(cmd, TXFIFO[asize]);
	qspi_xmit(qspi);

	if (asize == 4) { // dummy byte
		writel(0, QSPI_TXD1);
		qspi_xmit(qspi);
		qspi_flush_rx();
	}

	qspi_flush_rx();

	while (rcvd < count) {
		while (readl(QSPI_IRQ_STATUS) & RX_FIFO_NOT_EMPTY) {
			*data++ = readl(QSPI_RXDATA);
			rcvd++;
		}
		while ((readl(QSPI_IRQ_STATUS) & TX_FIFO_NOT_FULL) && (sent < count)) {
			writel(0, QSPI_TXD0);
			sent++;
		}
		qspi_xmit(qspi);
	}
	qspi_cs(qspi, 1);
}

void qspi_wr(struct qspi_ctxt *qspi, uint32_t cmd, uint32_t asize, uint32_t *data, uint32_t count)
{
	uint32_t sent = 0;
	uint32_t rcvd = 0;

	DEBUG_ASSERT(qspi);
	DEBUG_ASSERT(asize < 6);

	qspi_cs(qspi, 0);

	writel(cmd, TXFIFO[asize]);
	qspi_xmit(qspi);

	if (asize == 4) { // dummy byte
		writel(0, QSPI_TXD1);
		qspi_xmit(qspi);
		qspi_flush_rx();
	}

	qspi_flush_rx();

	while (rcvd < count) {
		while (readl(QSPI_IRQ_STATUS) & RX_FIFO_NOT_EMPTY) {
			readl(QSPI_RXDATA); // discard
			rcvd++;
		}
		while ((readl(QSPI_IRQ_STATUS) & TX_FIFO_NOT_FULL) && (sent < count)) {
			writel(*data++, QSPI_TXD0);
			sent++;
		}
		qspi_xmit(qspi);
	}

	qspi_cs(qspi, 1);
}

void qspi_wr1(struct qspi_ctxt *qspi, uint32_t cmd)
{
	DEBUG_ASSERT(qspi);

	qspi_cs(qspi, 0);
	writel(cmd, QSPI_TXD1);
	qspi_xmit(qspi);

	while (!(readl(QSPI_IRQ_STATUS) & RX_FIFO_NOT_EMPTY)) ;

	readl(QSPI_RXDATA);
	qspi_cs(qspi, 1);
}

void qspi_wr2(struct qspi_ctxt *qspi, uint32_t cmd)
{
	DEBUG_ASSERT(qspi);

	qspi_cs(qspi, 0);
	writel(cmd, QSPI_TXD2);
	qspi_xmit(qspi);

	while (!(readl(QSPI_IRQ_STATUS) & RX_FIFO_NOT_EMPTY)) ;

	readl(QSPI_RXDATA);
	qspi_cs(qspi, 1);
}

void qspi_wr3(struct qspi_ctxt *qspi, uint32_t cmd)
{
	DEBUG_ASSERT(qspi);

	qspi_cs(qspi, 0);
	writel(cmd, QSPI_TXD3);
	qspi_xmit(qspi);

	while (!(readl(QSPI_IRQ_STATUS) & RX_FIFO_NOT_EMPTY)) ;

	readl(QSPI_RXDATA);
	qspi_cs(qspi, 1);
}

uint32_t qspi_rd1(struct qspi_ctxt *qspi, uint32_t cmd)
{
	qspi_cs(qspi, 0);
	writel(cmd, QSPI_TXD2);
	qspi_xmit(qspi);

	while (!(readl(QSPI_IRQ_STATUS) & RX_FIFO_NOT_EMPTY)) ;

	qspi_cs(qspi, 1);
	return readl(QSPI_RXDATA);
}

// vim: set ts=4 sw=4 noexpandtab:

