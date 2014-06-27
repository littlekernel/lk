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
#include <debug.h>
#include <compiler.h>
#include <printf.h>
#include <reg.h>

#include <lib/console.h>

typedef uint32_t u32;

#include <platform/zynq.h>
#include <platform/fpga.h>

#define QSPI_CONFIG		0xE000D000
#define  CFG_IFMODE		(1 << 31) // Inteligent Flash Mode
#define  CFG_LITTLE_ENDIAN	(0 << 26)
#define  CFG_BIG_ENDIAN		(1 << 26)
#define  CFG_HOLDB_DR		(1 << 19) // set to 1 for dual/quad spi mode
#define  CFG_NO_MODIFY_MASK	(1 << 17) // do not modify this bit
#define  CFG_MANUAL_START	(1 << 16) // start transaction
#define  CFG_MANUAL_START_EN	(1 << 15) // enable manual start mode
#define  CFG_MANUAL_CS_EN	(1 << 14) // enable manual CS control
#define  CFG_MANUAL_CS		(1 << 10) // directly drives n_ss_out if MANUAL_CS_EN==1
#define  CFG_FIFO_WIDTH_32	(3 << 6)  // only valid setting
#define  CFG_BAUD_MASK		(7 << 3)
#define  CFG_BAUD_DIV_2		(0 << 3)
#define  CFG_BAUD_DIV_4		(1 << 3)
#define  CFG_BAUD_DIV_8		(2 << 3)
#define  CFG_BAUD_DIV_16	(3 << 3)
#define  CFG_CPHA		(1 << 2) // clock phase
#define  CFG_CPOL		(1 << 1) // clock polarity
#define  CFG_MASTER_MODE	(1 << 0) // only valid setting

#define QSPI_IRQ_STATUS		0xE000D004 // ro status (write UNDERFLOW/OVERFLOW to clear)
#define QSPI_IRQ_ENABLE		0xE000D008 // write 1s to set mask bits
#define QSPI_IRQ_DISABLE	0xE000D00C // write 1s to clear mask bits
#define QSPI_IRQ_MASK		0xE000D010 // ro mask value (1 = irq enabled)
#define  TX_UNDERFLOW		(1 << 6)
#define  RX_FIFO_FULL		(1 << 5)
#define  RX_FIFO_NOT_EMPTY	(1 << 4)
#define  TX_FIFO_FULL		(1 << 3)
#define  TX_FIFO_NOT_FULL	(1 << 2)
#define  RX_OVERFLOW		(1 << 0)

#define QSPI_ENABLE		0xE000D014 // write 1 to enable

#define QSPI_DELAY		0xE000D018
#define QSPI_TXD0		0xE000D01C
#define QSPI_RXDATA		0xE000D020
#define QSPI_SLAVE_IDLE_COUNT	0xE000D024
#define QSPI_TX_THRESHOLD	0xE000D028
#define QSPI_RX_THRESHOLD	0xE000D02C
#define QSPI_GPIO		0xE000D030
#define QSPI_LPBK_DLY_ADJ	0xE000D038
#define QSPI_TXD1		0xE000D080
#define QSPI_TXD2		0xE000D084
#define QSPI_TXD3		0xE000D088

#define QSPI_LINEAR_CONFIG	0xE000D0A0
#define  LCFG_ENABLE		(1 << 31) // enable linear quad spi mode
#define  LCFG_TWO_MEM		(1 << 30)
#define  LCFG_SEP_BUS		(1 << 29) // 0=shared 1=separate
#define  LCFG_U_PAGE		(1 << 28)
#define  LCFG_MODE_EN		(1 << 25) // send mode bits (required for dual/quad io)
#define  LCFG_MODE_ON		(1 << 24) // only send instruction code for first read
#define  LCFG_MODE_BITS(n)	(((n) & 0xFF) << 16)
#define  LCFG_DUMMY_BYTES(n)	(((n) & 7) << 8)
#define  LCFG_INST_CODE(n)	((n) & 0xFF)

#define QSPI_LINEAR_STATUS	0xE000D0A4
#define QSPI_MODULE_ID		0xE000D0FC

struct qspi_ctxt {
	u32 cfg;
	u32 khz;
};

int qspi_set_speed(struct qspi_ctxt *qspi, u32 khz) {
	u32 n;

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

	writel(0, QSPI_ENABLE);
	if (n == CFG_BAUD_DIV_2) {
		writel(0x20, QSPI_LPBK_DLY_ADJ);
	} else {
		writel(0, QSPI_LPBK_DLY_ADJ);
	}
	writel((readl(QSPI_CONFIG) & (~CFG_BAUD_MASK)) | n, QSPI_CONFIG);
	writel(1, QSPI_ENABLE);
	return 0;
}

int qspi_init(struct qspi_ctxt *qspi, u32 khz) {
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

	writel(1, QSPI_ENABLE);

	// clear sticky irqs
	writel(TX_UNDERFLOW | RX_OVERFLOW, QSPI_IRQ_STATUS);

	qspi->khz = 100000;
	return 0;
}

static inline void qspi_cs0(struct qspi_ctxt *qspi) {
	qspi->cfg &= ~(CFG_MANUAL_CS);
	writel(qspi->cfg, QSPI_CONFIG);
}

static inline void qspi_cs1(struct qspi_ctxt *qspi) {
	qspi->cfg |= CFG_MANUAL_CS;
	writel(qspi->cfg, QSPI_CONFIG);
}

static inline void qspi_xmit(struct qspi_ctxt *qspi) {
	// start txn
	writel(qspi->cfg | CFG_MANUAL_START, QSPI_CONFIG);
	// wait for command to transmit and TX fifo to be empty
	while((readl(QSPI_IRQ_STATUS) & TX_FIFO_NOT_FULL) == 0) ;
}

static inline void qspi_flush_rx(void) {
	while (!(readl(QSPI_IRQ_STATUS) & RX_FIFO_NOT_EMPTY)) ;
	readl(QSPI_RXDATA);
}

static u32 TXFIFO[] = { QSPI_TXD1, QSPI_TXD2, QSPI_TXD3, QSPI_TXD0, QSPI_TXD0, QSPI_TXD0 };

static void qspi_rd(struct qspi_ctxt *qspi, u32 cmd, u32 asize, u32 *data, u32 count) {
	u32 sent = 0;
	u32 rcvd = 0;
	qspi_cs0(qspi);

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
	qspi_cs1(qspi);
}

static void qspi_wr(struct qspi_ctxt *qspi, u32 cmd, u32 asize, u32 *data, u32 count) {
	u32 sent = 0;
	u32 rcvd = 0;
	qspi_cs0(qspi);

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

	qspi_cs1(qspi);
}

// adjust 24 bit address to be correct-byte-order for 32bit qspi commands
static u32 qspi_fix_addr(u32 addr) {
	return ((addr & 0xff) << 24) | ((addr&0xff00) << 8) | ((addr>>8) & 0xff00);
}

static void qspi_rd32(struct qspi_ctxt *qspi, u32 addr, u32 *data, u32 count) {
	qspi_rd(qspi, qspi_fix_addr(addr) | 0x6B, 4, data, count);
}

void qspi_wr0(struct qspi_ctxt *qspi, u32 cmd) {
	qspi_cs0(qspi);
	writel(cmd, QSPI_TXD1);
	qspi_xmit(qspi);
	while (!(readl(QSPI_IRQ_STATUS) & RX_FIFO_NOT_EMPTY)) ;
	readl(QSPI_RXDATA);
	qspi_cs1(qspi);
}

static u32 qspi_rd1(struct qspi_ctxt *qspi, u32 cmd) {
	qspi_cs0(qspi);
	writel(cmd, QSPI_TXD2);
	qspi_xmit(qspi);
	while (!(readl(QSPI_IRQ_STATUS) & RX_FIFO_NOT_EMPTY)) ;
	qspi_cs1(qspi);
	return readl(QSPI_RXDATA);
}

static inline void qspi_wren(struct qspi_ctxt *qspi) {
	qspi_wr0(qspi, 0x06);
}
static inline void qspi_clsr(struct qspi_ctxt *qspi) {
	qspi_wr0(qspi, 0x30);
}
static inline u32 qspi_rd_cr1(struct qspi_ctxt *qspi) {
	return qspi_rd1(qspi, 0x35) >> 24;
}
static inline u32 qspi_rd_status(struct qspi_ctxt *qspi) {
	return qspi_rd1(qspi, 0x05) >> 24;
}

#define STS_PROGRAM_ERR (1<<6)
#define STS_ERASE_ERR (1<<5)
#define STS_BUSY (1<<0)

static int _qspi_erase(struct qspi_ctxt *qspi, u32 cmd, u32 addr) {
	u32 status;
	qspi_wren(qspi);
	qspi_wr(qspi, qspi_fix_addr(addr) | cmd, 3, 0, 0);
	while ((status = qspi_rd_status(qspi)) & STS_BUSY) ;
	if (status & (STS_PROGRAM_ERR | STS_ERASE_ERR)) {
		printf("qspi_erase_sector failed @ %x\n", addr);
		qspi_clsr(qspi);
		return -1;
	}
	return 0;
}

static int qspi_erase_sector(struct qspi_ctxt *qspi, u32 addr) {
	return _qspi_erase(qspi, 0xD8, addr);
}

static int qspi_erase_subsector(struct qspi_ctxt *qspi, u32 addr) {
	return _qspi_erase(qspi, 0x20, addr);
}

static int qspi_write_page(struct qspi_ctxt *qspi, u32 addr, u32 *data, u32 count) {
	u32 oldkhz, status;
	if (addr & 0xFF)
		return -1;
	if (count != 64)
		return -1;
	oldkhz = qspi->khz;
	if (qspi_set_speed(qspi, 80000))
		return -1;
	qspi_wren(qspi);
	qspi_wr(qspi, qspi_fix_addr(addr) | 0x32, 3, data, count);	
	qspi_set_speed(qspi, oldkhz);
	while ((status = qspi_rd_status(qspi)) & STS_BUSY) ;
	if (status & (STS_PROGRAM_ERR | STS_ERASE_ERR)) {
		printf("qspi_write_page failed @ %x\n", addr);
		qspi_clsr(qspi);
		return -1;
	}
	return 0;
}

void se_test(void) {
	struct qspi_ctxt qspi;
	qspi_init(&qspi, 100000);
	qspi_erase_sector(&qspi, 0x1000);
}

u32 data[64];
void sw_test(int argc, const cmd_args *argv) {
	struct qspi_ctxt qspi;
	u32 n;

	if (argc < 2) return;

	for (n = 0; n < 64; n++) 
		data[n] = 0x12345600 | n;
	qspi_init(&qspi, 100000);
	if (qspi_write_page(&qspi, argv[1].u, data, 64))
		printf("write failed\n");
}

void sf_test(int argc, const cmd_args *argv) {
	struct qspi_ctxt qspi;
	u32 n, addr = 0;
	unsigned char *x = (void*) data;

	if (argc > 1)
		addr = argv[1].u;

	printf("argc %d\n", argc);
	qspi_init(&qspi, 100000);
	printf("sts: %08x\n", qspi_rd_status(&qspi));
	printf("cr1: %08x\n", qspi_rd_cr1(&qspi));
	qspi_rd32(&qspi, addr, data, 32);
	for (n = 0; n < 32; n++) printf("%08x ", data[n]);
	printf("\n");

	qspi_rd(&qspi, 0x9F, 0, data, 21);
	for (n = 0; n < 81; n++) printf("%02x ",x[n]);
	printf("\n");
}

void flash(int argc, const cmd_args *argv) {
	struct qspi_ctxt qspi;
	u32 dst, src, count, n;
	if (argc != 4) {
		printf("usage: flash <flash-addr-dst> <phys-addr-src> <bytes>\n");
		return;
	}
	dst = argv[1].u;
	src = argv[2].u;
	count = argv[3].u;
	if (count & 0xFF)
		count = (count & 0xFFFFFF00) + 0x100;
	if (dst & 0xFFFF) {
		printf("error: destination must be sector aligned\n");
		return;
	}
	if (src & 3) {
		printf("error: source must be word aligned\n");
		return;
	}

	qspi_init(&qspi, 100000);
	for (n = 0; n < count; n += 0x10000) {
		printf("e");
		if (qspi_erase_sector(&qspi, dst + n)) {
			printf("\nfailed to erase sector at %x\n", dst + n);
			return;
		}
	}
	for (n = 0; n < count; n += 0x100) {
		if ((n & 0xffff) == 0) printf("w");
		if (qspi_write_page(&qspi, dst + n, (u32*) (src + n), 64)) {
			printf("failed to write page at %x\n", dst + n);
			return;
		}
	}
	printf("\ndone.\n");

}

#if defined(WITH_LIB_CONSOLE)
#include <lib/console.h>

STATIC_COMMAND_START
STATIC_COMMAND("sf", "sf test", (console_cmd)&sf_test)
STATIC_COMMAND("se", "se test", (console_cmd)&se_test)
STATIC_COMMAND("sw", "sw test", (console_cmd)&sw_test)
STATIC_COMMAND("flash", "flash", (console_cmd)&flash)
STATIC_COMMAND_END(qspi);

#endif

// vim: set ts=4 sw=4 noexpandtab

