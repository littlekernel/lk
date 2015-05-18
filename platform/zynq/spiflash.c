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
#include <rand.h>
#include <reg.h>

#include <lib/bio.h>
#include <lib/console.h>
#include <dev/qspi.h>
#include <kernel/thread.h>

#include <platform/zynq.h>

#define LOCAL_TRACE 0

// parameters specifically for the 16MB spansion S25FL128S flash
#define PARAMETER_AREA_SIZE (128*1024)
#define PAGE_PROGRAM_SIZE (256)     // can be something else based on the part
#define PAGE_ERASE_SLEEP_TIME (150) // amount of time before waiting to check if erase completed
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

static ssize_t spiflash_bdev_read(struct bdev *, void *buf, off_t offset, size_t len);
static ssize_t spiflash_bdev_read_block(struct bdev *, void *buf, bnum_t block, uint count);
static ssize_t spiflash_bdev_write_block(struct bdev *, const void *buf, bnum_t block, uint count);
static ssize_t spiflash_bdev_erase(struct bdev *, off_t offset, size_t len);
static int spiflash_ioctl(struct bdev *, int request, void *argp);

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
	qspi_wr1(qspi, 0x06);
}

static inline void qspi_clsr(struct qspi_ctxt *qspi)
{
	qspi_wr1(qspi, 0x30);
}

static inline uint32_t qspi_rd_cr1(struct qspi_ctxt *qspi)
{
	return qspi_rd1(qspi, 0x35) >> 24;
}

static inline uint32_t qspi_rd_status(struct qspi_ctxt *qspi)
{
	return qspi_rd1(qspi, 0x05) >> 24;
}

static inline void qspi_wr_status_cr1(struct qspi_ctxt *qspi, uint8_t status, uint8_t cr1)
{
	uint32_t cmd = (cr1 << 16) | (status << 8) | 0x01;

	qspi_wren(qspi);
	qspi_wr3(qspi, cmd);
}

static ssize_t qspi_erase_sector(struct qspi_ctxt *qspi, uint32_t addr)
{
	uint32_t cmd;
	uint32_t status;
	ssize_t toerase;

	LTRACEF("addr 0x%x\n", addr);

	DEBUG_ASSERT(qspi);

	if (addr < PARAMETER_AREA_SIZE) {
		// erase a small parameter sector (4K)
		DEBUG_ASSERT(IS_ALIGNED(addr, SECTOR_ERASE_SIZE));
		if (!IS_ALIGNED(addr, SECTOR_ERASE_SIZE))
			return ERR_INVALID_ARGS;

		cmd = 0x20;
		toerase = SECTOR_ERASE_SIZE;
	} else {
		// erase a large sector (64k or 256k)
		DEBUG_ASSERT(IS_ALIGNED(addr, LARGE_SECTOR_ERASE_SIZE));
		if (!IS_ALIGNED(addr, LARGE_SECTOR_ERASE_SIZE))
			return ERR_INVALID_ARGS;

		cmd = 0xd8;
		toerase = LARGE_SECTOR_ERASE_SIZE;
	}

	qspi_wren(qspi);
	qspi_wr(qspi, qspi_fix_addr(addr) | cmd, 3, 0, 0);

	thread_sleep(PAGE_ERASE_SLEEP_TIME);
	while ((status = qspi_rd_status(qspi)) & STS_BUSY)
		;

	LTRACEF("status 0x%x\n", status);
	if (status & (STS_PROGRAM_ERR | STS_ERASE_ERR)) {
		TRACEF("failed @ 0x%x\n", addr);
		qspi_clsr(qspi);
		return ERR_IO;
	}

	return toerase;
}

static ssize_t qspi_write_page(struct qspi_ctxt *qspi, uint32_t addr, const uint8_t *data)
{
	uint32_t oldkhz, status;

	LTRACEF("addr 0x%x, data %p\n", addr, data);

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

static ssize_t spiflash_read_otp(void *buf, uint32_t addr, size_t len)
{
	DEBUG_ASSERT(len > 0 && (len % 4) == 0);

	if (len > 1024)
		len = 1024;

	qspi_rd(&flash.qspi, 0x4b, 4, buf, len / 4);

	if (len < 4)
		return len;

	return len;
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

	/* read the 16 byte random number out of the OTP area and add to the rand entropy pool */
	uint32_t r[4];
	memset(r, 0, sizeof(r));
	spiflash_read_otp(r, 0, 16);

	LTRACEF("OTP random %08x%08x%08x%08x\n", r[0], r[1], r[2], r[3]);
	rand_add_entropy(r, sizeof(r));

	flash.detected = true;

	/* see if we're in serial mode */
	uint32_t cr1 = qspi_rd_cr1(&flash.qspi);
	if ((cr1 & (1<<1)) == 0) {
		printf("spiflash: device not in quad mode, cannot use for read/write\n");
		goto nouse;
	}

	/* construct the block device */
	bio_initialize_bdev(&flash.bdev, "spi0", PAGE_PROGRAM_SIZE, flash.size / PAGE_PROGRAM_SIZE);

	/* override our block device hooks */
	flash.bdev.read = &spiflash_bdev_read;
	flash.bdev.read_block = &spiflash_bdev_read_block;
	// flash.bdev.write has a default hook that will be okay
	flash.bdev.write_block = &spiflash_bdev_write_block;
	flash.bdev.erase = &spiflash_bdev_erase;
	flash.bdev.ioctl = &spiflash_ioctl;

	bio_register_device(&flash.bdev);

	LTRACEF("found flash of size 0x%llx\n", flash.size);

nouse:
	return NO_ERROR;

nodetect:
	LTRACEF("flash not found\n");

	free(buf);
	flash.detected = false;
	return ERR_NOT_FOUND;
}

// bio layer hooks
static ssize_t spiflash_bdev_read(struct bdev *bdev, void *buf, off_t offset, size_t len)
{
	LTRACEF("dev %p, buf %p, offset 0x%llx, len 0x%zx\n", bdev, buf, offset, len);

	DEBUG_ASSERT(flash.detected);

	len = bio_trim_range(bdev, offset, len);
	if (len == 0)
		return 0;

	// XXX handle not mulitple of 4
	qspi_rd32(&flash.qspi, offset, buf, len / 4);

	return len;
}

static ssize_t spiflash_bdev_read_block(struct bdev *bdev, void *buf, bnum_t block, uint count)
{
	LTRACEF("dev %p, buf %p, block 0x%x, count %u\n", bdev, buf, block, count);

	count = bio_trim_block_range(bdev, block, count);
	if (count == 0)
		return 0;

	return spiflash_bdev_read(bdev, buf, block << bdev->block_shift, count << bdev->block_shift);
}

static ssize_t spiflash_bdev_write_block(struct bdev *bdev, const void *_buf, bnum_t block, uint count)
{
	LTRACEF("dev %p, buf %p, block 0x%x, count %u\n", bdev, _buf, block, count);

	DEBUG_ASSERT(bdev->block_size == PAGE_PROGRAM_SIZE);

	count = bio_trim_block_range(bdev, block, count);
	if (count == 0)
		return 0;

	const uint8_t *buf = _buf;

	ssize_t written = 0;
	while (count > 0) {
		ssize_t err = qspi_write_page(&flash.qspi, block * PAGE_PROGRAM_SIZE, buf);
		if (err < 0)
			return err;

		buf += PAGE_PROGRAM_SIZE;
		written += err;
		block++;
		count--;
	}

	return written;
}

static ssize_t spiflash_bdev_erase(struct bdev *bdev, off_t offset, size_t len)
{
	LTRACEF("dev %p, offset 0x%llx, len 0x%zx\n", bdev, offset, len);

	len = bio_trim_range(bdev, offset, len);
	if (len == 0)
		return 0;

	ssize_t erased = 0;
	while (erased < (ssize_t)len) {
		ssize_t err = qspi_erase_sector(&flash.qspi, offset);
		if (err < 0)
			return err;

		erased += err;
		offset += err;
	}

	return erased;
}

static int spiflash_ioctl(struct bdev *bdev, int request, void *argp)
{
	LTRACEF("dev %p, request %d, argp %p\n", bdev, request, argp);

	int ret = ERR_NOT_SUPPORTED;
	switch (request) {
		case BIO_IOCTL_GET_MEM_MAP:
			/* put the device into linear mode */
			ret = qspi_enable_linear(&flash.qspi);
			if (argp)
				*(void **)argp = (void *)QSPI_LINEAR_BASE;
			break;
		case BIO_IOCTL_PUT_MEM_MAP:
			/* put the device back into regular mode */
			ret = qspi_disable_linear(&flash.qspi);
			break;
	}

	return ret;
}

// debug tests
int cmd_spiflash(int argc, const cmd_args *argv)
{
	if (argc < 2) {
notenoughargs:
		printf("not enough arguments\n");
usage:
		printf("usage:\n");
#if LK_DEBUGLEVEL > 1
		printf("\t%s detect\n", argv[0].str);
		printf("\t%s cfi\n", argv[0].str);
		printf("\t%s cr1\n", argv[0].str);
		printf("\t%s otp\n", argv[0].str);
		printf("\t%s linear [true/false]\n", argv[0].str);
		printf("\t%s read <offset> <length>\n", argv[0].str);
		printf("\t%s write <offset> <length> <address>\n", argv[0].str);
		printf("\t%s erase <offset>\n", argv[0].str);
#endif
		printf("\t%s setquad (dangerous)\n", argv[0].str);
		return ERR_INVALID_ARGS;
	}

#if LK_DEBUGLEVEL > 1
	if (!strcmp(argv[1].str, "detect")) {
		spiflash_detect();
	} else if (!strcmp(argv[1].str, "cr1")) {
		if (!flash.detected) {
			printf("flash not detected\n");
			return -1;
		}

		uint32_t cr1 = qspi_rd_cr1(&flash.qspi);
		printf("cr1 0x%x\n", cr1);
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
	} else if (!strcmp(argv[1].str, "otp")) {
		if (!flash.detected) {
			printf("flash not detected\n");
			return -1;
		}

		uint8_t *buf = calloc(1, 1024);
		ssize_t len = spiflash_read_otp(buf, 0, 1024);
		printf("spiflash_read_otp returns %ld\n", len);

		hexdump8(buf, len);

		free(buf);
	} else if (!strcmp(argv[1].str, "linear")) {
		if (argc < 3) goto notenoughargs;
		if (!flash.detected) {
			printf("flash not detected\n");
			return -1;
		}

		if (argv[2].b)
			qspi_enable_linear(&flash.qspi);
		else
			qspi_disable_linear(&flash.qspi);
	} else if (!strcmp(argv[1].str, "read")) {
		if (argc < 4) goto notenoughargs;
		if (!flash.detected) {
			printf("flash not detected\n");
			return -1;
		}

		uint8_t *buf = calloc(1, argv[3].u);

		qspi_rd32(&flash.qspi, argv[2].u, (uint32_t *)buf, argv[3].u / 4);

		hexdump8(buf, argv[3].u);
		free(buf);
	} else if (!strcmp(argv[1].str, "write")) {
		if (argc < 5) goto notenoughargs;
		if (!flash.detected) {
			printf("flash not detected\n");
			return -1;
		}

		status_t err = qspi_write_page(&flash.qspi, argv[2].u, (void *)argv[4].u);
		printf("write_page returns %d\n", err);
	} else if (!strcmp(argv[1].str, "erase")) {
		if (argc < 3) goto notenoughargs;
		if (!flash.detected) {
			printf("flash not detected\n");
			return -1;
		}

		status_t err = qspi_erase_sector(&flash.qspi, argv[2].u);
		printf("erase returns %d\n", err);
	} else
#endif
	if (!strcmp(argv[1].str, "setquad")) {
		if (!flash.detected) {
			printf("flash not detected\n");
			return -1;
		}

		uint32_t cr1 = qspi_rd_cr1(&flash.qspi);
		printf("cr1 before 0x%x\n", cr1);

		if (cr1 & (1<<1)) {
			printf("flash already in quad mode\n");
			return 0;
		}

		qspi_wr_status_cr1(&flash.qspi, 0, cr1 | (1<<1));

		thread_sleep(500);
		cr1 = qspi_rd_cr1(&flash.qspi);
		printf("cr1 after 0x%x\n", cr1);
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
