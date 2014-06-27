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
#include <debug.h>
#include <assert.h>
#include <trace.h>
#include <compiler.h>
#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <string.h>
#include <reg.h>

#include <lib/bio.h>
#include <lib/console.h>
#include <dev/qspi.h>

#include <platform/zynq.h>

#define LOCAL_TRACE 1

#define PARAMETER_AREA_SIZE (128*1024)
#define PAGE_PROGRAM_SIZE (256) // XXX can be something else
#define SECTOR_ERASE_SIZE (4096)
#define LARGE_SECTOR_ERASE_SIZE (64*1024)

#define STS_PROGRAM_ERR (1<<6)
#define STS_ERASE_ERR (1<<5)
#define STS_BUSY (1<<0)

struct spi_flash {
	bool detected;

	struct qspi_ctxt qspi;
	bdev_t bdev;

	off_t size;
};

static struct spi_flash flash;

// adjust 24 bit address to be correct-byte-order for 32bit qspi commands
static uint32_t qspi_fix_addr(uint32_t addr)
{
	DEBUG_ASSERT((addr & ~(0x00ffffff)) == 0); // only dealing with 24bit addresses

	return ((addr & 0xff) << 24) | ((addr&0xff00) << 8) | ((addr>>8) & 0xff00);
}

static void qspi_rd32(struct qspi_ctxt *qspi, uint32_t addr, uint32_t *data, uint32_t count)
{
	qspi_rd(qspi, qspi_fix_addr(addr) | 0x6B, 4, data, count);
}

static inline void qspi_wren(struct qspi_ctxt *qspi)
{
	qspi_wr0(qspi, 0x06);
}

static inline void qspi_clsr(struct qspi_ctxt *qspi)
{
	qspi_wr0(qspi, 0x30);
}

static inline uint32_t qspi_rd_cr1(struct qspi_ctxt *qspi)
{
	return qspi_rd1(qspi, 0x35) >> 24;
}

static inline uint32_t qspi_rd_status(struct qspi_ctxt *qspi)
{
	return qspi_rd1(qspi, 0x05) >> 24;
}

static int qspi_erase_sector(struct qspi_ctxt *qspi, uint32_t addr)
{
	uint32_t cmd;
	uint32_t status;

	LTRACEF("addr 0x%x\n", addr);

	DEBUG_ASSERT(qspi);

	if (addr < PARAMETER_AREA_SIZE) {
		// erase a small parameter sector (4K)
		DEBUG_ASSERT(IS_ALIGNED(addr, SECTOR_ERASE_SIZE));
		if (!IS_ALIGNED(addr, SECTOR_ERASE_SIZE))
			return ERR_INVALID_ARGS;

		cmd = 0x20;
	} else {
		// erase a large sector (64k or 256k)
		DEBUG_ASSERT(IS_ALIGNED(addr, LARGE_SECTOR_ERASE_SIZE));
		if (!IS_ALIGNED(addr, LARGE_SECTOR_ERASE_SIZE))
			return ERR_INVALID_ARGS;

		cmd = 0xd8;
	}

	qspi_wren(qspi);
	qspi_wr(qspi, qspi_fix_addr(addr) | cmd, 3, 0, 0);

	while ((status = qspi_rd_status(qspi)) & STS_BUSY) ;

	LTRACEF("status 0x%x\n", status);
	if (status & (STS_PROGRAM_ERR | STS_ERASE_ERR)) {
		TRACEF("failed @ 0x%x\n", addr);
		qspi_clsr(qspi);
		return ERR_IO;
	}
	return 0;
}

static int qspi_write_page(struct qspi_ctxt *qspi, uint32_t addr, const uint8_t *data)
{
	uint32_t oldkhz, status;

	DEBUG_ASSERT(qspi);
	DEBUG_ASSERT(data);
	DEBUG_ASSERT(IS_ALIGNED(addr, PAGE_PROGRAM_SIZE));

	if (!IS_ALIGNED(addr, PAGE_PROGRAM_SIZE))
		return ERR_INVALID_ARGS;

	oldkhz = qspi->khz;
	if (qspi_set_speed(qspi, 80000))
		return ERR_IO;

	qspi_wren(qspi);
	qspi_wr(qspi, qspi_fix_addr(addr) | 0x32, 3, (uint32_t *)data, PAGE_PROGRAM_SIZE / 4);
	qspi_set_speed(qspi, oldkhz);

	while ((status = qspi_rd_status(qspi)) & STS_BUSY) ;

	if (status & (STS_PROGRAM_ERR | STS_ERASE_ERR)) {
		printf("qspi_write_page failed @ %x\n", addr);
		qspi_clsr(qspi);
		return ERR_IO;
	}
	return PAGE_PROGRAM_SIZE;
}

static ssize_t spiflash_read_cfi(void *buf, size_t len)
{
	DEBUG_ASSERT(len > 0 && (len % 4) == 0);

	qspi_rd(&flash.qspi, 0x9f, 0, buf, len / 4);

	if (len < 4)
		return len;

	/* look at byte 3 of the cfi, which says the total length of the cfi structure */
	size_t cfi_len = ((uint8_t *)buf)[3];
	if (cfi_len == 0)
		cfi_len = 512;
	else
		cfi_len += 3;

	return MIN(len, cfi_len);
}

status_t spiflash_detect(void)
{
	if (flash.detected)
		return NO_ERROR;

	qspi_init(&flash.qspi, 100000);

	/* read and parse the cfi */
	uint8_t *buf = calloc(1, 512);
	ssize_t len = spiflash_read_cfi(buf, 512);
	if (len < 4)
		goto nodetect;

	LTRACEF("looking at vendor/device id combination: %02x:%02x:%02x\n", buf[0], buf[1], buf[2]);

	/* at the moment, we only support particular spansion flashes */
	if (buf[0] != 0x01) goto nodetect;

	if (buf[1] == 0x20 && buf[2] == 0x18) {
		/* 128Mb version */
		flash.size = 16*1024*1024;
	} else if (buf[1] == 0x02 && buf[2] == 0x19) {
		/* 256Mb version */
		flash.size = 32*1024*1024;
	} else {
		TRACEF("unknown vendor/device id combination: %02x:%02x:%02x\n",
			buf[0], buf[1], buf[2]);
		goto nodetect;
	}

	free(buf);

	flash.detected = true;

	/* construct the block device */
	bio_initialize_bdev(&flash.bdev, "spi0", PAGE_PROGRAM_SIZE, flash.size / PAGE_PROGRAM_SIZE);
	bio_register_device(&flash.bdev);

	LTRACEF("found flash of size 0x%llx\n", flash.size);

	return NO_ERROR;

nodetect:
	LTRACEF("flash not found\n");

	free(buf);
	flash.detected = false;
	return ERR_NOT_FOUND;
}

// debug tests
int cmd_spiflash(int argc, const cmd_args *argv)
{
	if (argc < 2) {
notenoughargs:
		printf("not enough arguments\n");
usage:
		printf("usage:\n");
		printf("\t%s detect\n", argv[0].str);
		printf("\t%s cfi\n", argv[0].str);
		printf("\t%s read <address> <length>\n", argv[0].str);
		printf("\t%s write <address> <length> <memory_address>\n", argv[0].str);
		printf("\t%s erase <address>\n", argv[0].str);
		return ERR_INVALID_ARGS;
	}

	if (!strcmp(argv[1].str, "detect")) {
		spiflash_detect();
	} else if (!strcmp(argv[1].str, "cfi")) {
		if (!flash.detected) {
			printf("flash not detected\n");
			return -1;
		}

		uint8_t *buf = calloc(1, 512);
		ssize_t len = spiflash_read_cfi(buf, 512);
		printf("returned cfi len %ld\n", len);

		hexdump8(buf, len);

		free(buf);
	} else if (!strcmp(argv[1].str, "read")) {
		if (argc < 4) goto notenoughargs;

		uint8_t *buf = calloc(1, argv[3].u);

		qspi_rd32(&flash.qspi, argv[2].u, (uint32_t *)buf, argv[3].u / 4);

		hexdump8(buf, argv[3].u);
		free(buf);
	} else if (!strcmp(argv[1].str, "write")) {
		if (argc < 5) goto notenoughargs;

		status_t err = qspi_write_page(&flash.qspi, argv[2].u, (void *)argv[4].u);
		printf("write_page returns %d\n", err);
	} else if (!strcmp(argv[1].str, "erase")) {
		if (argc < 3) goto notenoughargs;

		status_t err = qspi_erase_sector(&flash.qspi, argv[2].u);
		printf("erase returns %d\n", err);
	} else {
		printf("unknown command\n");
		goto usage;
	}

	return 0;
}

#if defined(WITH_LIB_CONSOLE)
#include <lib/console.h>

STATIC_COMMAND_START
STATIC_COMMAND("spiflash", "spi flash manipulation utilities", cmd_spiflash)
STATIC_COMMAND_END(qspi);

#endif

// vim: set ts=4 sw=4 noexpandtab:


