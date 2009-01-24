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
#include <reg.h>
#include <stdlib.h>
#include <string.h>
#include <dev/flash.h>
#include <lib/ptable.h>

#include "dmov.h"
#include "nand.h"

#define VERBOSE 0

typedef struct dmov_ch dmov_ch;
struct dmov_ch 
{
	volatile unsigned cmd;
	volatile unsigned result;
	volatile unsigned status;
	volatile unsigned config;
};

static void dmov_prep_ch(dmov_ch *ch, unsigned id)
{
	ch->cmd = DMOV_CMD_PTR(id);
	ch->result = DMOV_RSLT(id);
	ch->status = DMOV_STATUS(id);
	ch->config = DMOV_CONFIG(id);
}

#define SRC_CRCI_NAND_CMD  CMD_SRC_CRCI(DMOV_NAND_CRCI_CMD)
#define DST_CRCI_NAND_CMD  CMD_DST_CRCI(DMOV_NAND_CRCI_CMD)
#define SRC_CRCI_NAND_DATA CMD_SRC_CRCI(DMOV_NAND_CRCI_DATA)
#define DST_CRCI_NAND_DATA CMD_DST_CRCI(DMOV_NAND_CRCI_DATA)

static unsigned CFG0, CFG1;

#define CFG1_WIDE_FLASH (1U << 1)

#define paddr(n) ((unsigned) (n))

static int dmov_exec_cmdptr(unsigned id, unsigned *ptr)
{
	dmov_ch ch;
	unsigned n;

	dmov_prep_ch(&ch, id);

	writel(DMOV_CMD_PTR_LIST | DMOV_CMD_ADDR(paddr(ptr)), ch.cmd);

	while(!(readl(ch.status) & DMOV_STATUS_RSLT_VALID)) ;

	n = readl(ch.status);
	while(DMOV_STATUS_RSLT_COUNT(n)) {
		n = readl(ch.result);
		if(n != 0x80000002) {
			dprintf(CRITICAL, "ERROR: result: %x\n", n);
			dprintf(CRITICAL, "ERROR:  flush: %x %x %x %x\n",
				readl(DMOV_FLUSH0(DMOV_NAND_CHAN)),
				readl(DMOV_FLUSH1(DMOV_NAND_CHAN)),
				readl(DMOV_FLUSH2(DMOV_NAND_CHAN)),
				readl(DMOV_FLUSH3(DMOV_NAND_CHAN)));
		}
		n = readl(ch.status);
	}

	return 0;
}

static unsigned flash_maker = 0;
static unsigned flash_device = 0;

static void flash_read_id(dmov_s *cmdlist, unsigned *ptrlist)
{
	dmov_s *cmd = cmdlist;
	unsigned *ptr = ptrlist;
	unsigned *data = ptrlist + 4;

	data[0] = 0 | 4;
	data[1] = NAND_CMD_FETCH_ID;
	data[2] = 1;
	data[3] = 0;
	data[4] = 0;

	cmd[0].cmd = 0 | CMD_OCB;
	cmd[0].src = paddr(&data[0]);
	cmd[0].dst = NAND_FLASH_CHIP_SELECT;
	cmd[0].len = 4;

	cmd[1].cmd = DST_CRCI_NAND_CMD;
	cmd[1].src = paddr(&data[1]);
	cmd[1].dst = NAND_FLASH_CMD;
	cmd[1].len = 4;

	cmd[2].cmd = 0;
	cmd[2].src = paddr(&data[2]);
	cmd[2].dst = NAND_EXEC_CMD;
	cmd[2].len = 4;

	cmd[3].cmd = SRC_CRCI_NAND_DATA;
	cmd[3].src = NAND_FLASH_STATUS;
	cmd[3].dst = paddr(&data[3]);
	cmd[3].len = 4;

	cmd[4].cmd = CMD_OCU | CMD_LC;
	cmd[4].src = NAND_READ_ID;
	cmd[4].dst = paddr(&data[4]);
	cmd[4].len = 4;

	ptr[0] = (paddr(cmd) >> 3) | CMD_PTR_LP;

	dmov_exec_cmdptr(DMOV_NAND_CHAN, ptr);

#if VERBOSE
	dprintf(INFO, "status: %x\n", data[3]);
#endif
	dprintf(INFO, "nandid: %x maker %02x device %02x\n",
		data[4], data[4] & 0xff, (data[4] >> 8) & 0xff);

	flash_maker = data[4] & 0xff;
	flash_device = (data[4] >> 8) & 0xff;
}

static int flash_erase_block(dmov_s *cmdlist, unsigned *ptrlist, unsigned page)
{
	dmov_s *cmd = cmdlist;
	unsigned *ptr = ptrlist;
	unsigned *data = ptrlist + 4;

	/* only allow erasing on block boundaries */
	if(page & 63) return -1;

	data[0] = NAND_CMD_BLOCK_ERASE;
	data[1] = page;
	data[2] = 0;
	data[3] = 0 | 4;
	data[4] = 1;
	data[5] = 0xeeeeeeee;
	data[6] = CFG0 & (~(7 << 6));  /* CW_PER_PAGE = 0 */
	data[7] = CFG1;

	cmd[0].cmd = DST_CRCI_NAND_CMD | CMD_OCB;
	cmd[0].src = paddr(&data[0]);
	cmd[0].dst = NAND_FLASH_CMD;
	cmd[0].len = 16;

	cmd[1].cmd = 0;
	cmd[1].src = paddr(&data[6]);
	cmd[1].dst = NAND_DEV0_CFG0;
	cmd[1].len = 8;

	cmd[2].cmd = 0;
	cmd[2].src = paddr(&data[4]);
	cmd[2].dst = NAND_EXEC_CMD;
	cmd[2].len = 4;

	cmd[3].cmd = SRC_CRCI_NAND_DATA | CMD_OCU | CMD_LC;
	cmd[3].src = NAND_FLASH_STATUS;
	cmd[3].dst = paddr(&data[5]);
	cmd[3].len = 4;

	ptr[0] = (paddr(cmd) >> 3) | CMD_PTR_LP;

	dmov_exec_cmdptr(DMOV_NAND_CHAN, ptr);

#if VERBOSE
	dprintf(INFO, "status: %x\n", data[5]);
#endif

	/* we fail if there was an operation error, a mpu error, or the
	 ** erase success bit was not set.
	 */
	if(data[5] & 0x110) return -1;
	if(!(data[5] & 0x80)) return -1;

	return 0;
}

struct data_flash_io {
	unsigned cmd;
	unsigned addr0;
	unsigned addr1;
	unsigned chipsel;
	unsigned cfg0;
	unsigned cfg1;
	unsigned exec;
	unsigned ecc_cfg;
	unsigned ecc_cfg_save;
	struct {
		unsigned flash_status;
		unsigned buffer_status;
	} result[4];
};

static int _flash_read_page(dmov_s *cmdlist, unsigned *ptrlist, unsigned page, void *_addr, void *_spareaddr)
{
	dmov_s *cmd = cmdlist;
	unsigned *ptr = ptrlist;
	struct data_flash_io *data = (void*) (ptrlist + 4);
	unsigned addr = (unsigned) _addr;
	unsigned spareaddr = (unsigned) _spareaddr;
	unsigned n;

	data->cmd = NAND_CMD_PAGE_READ_ECC;
	data->addr0 = page << 16;
	data->addr1 = (page >> 16) & 0xff;
	data->chipsel = 0 | 4; /* flash0 + undoc bit */

	/* GO bit for the EXEC register */
	data->exec = 1;

	data->cfg0 = CFG0;
	data->cfg1 = CFG1;

	data->ecc_cfg = 0x203;

	/* save existing ecc config */
	cmd->cmd = CMD_OCB;
	cmd->src = NAND_EBI2_ECC_BUF_CFG;
	cmd->dst = paddr(&data->ecc_cfg_save);
	cmd->len = 4;
	cmd++;

	for(n = 0; n < 4; n++) {
		/* write CMD / ADDR0 / ADDR1 / CHIPSEL regs in a burst */
		cmd->cmd = DST_CRCI_NAND_CMD;
		cmd->src = paddr(&data->cmd);
		cmd->dst = NAND_FLASH_CMD;
		cmd->len = ((n == 0) ? 16 : 4);
		cmd++;

		if (n == 0) {
			/* block on cmd ready, set configuration */
			cmd->cmd = 0;
			cmd->src = paddr(&data->cfg0);
			cmd->dst = NAND_DEV0_CFG0;
			cmd->len = 8;
			cmd++;

			/* set our ecc config */
			cmd->cmd = 0;
			cmd->src = paddr(&data->ecc_cfg);
			cmd->dst = NAND_EBI2_ECC_BUF_CFG;
			cmd->len = 4;
			cmd++;
		}
		/* kick the execute register */
		cmd->cmd = 0;
		cmd->src = paddr(&data->exec);
		cmd->dst = NAND_EXEC_CMD;
		cmd->len = 4;
		cmd++;

		/* block on data ready, then read the status register */
		cmd->cmd = SRC_CRCI_NAND_DATA;
		cmd->src = NAND_FLASH_STATUS;
		cmd->dst = paddr(&data->result[n]);
		cmd->len = 8;
		cmd++;

		/* read data block */
		cmd->cmd = 0;
		cmd->src = NAND_FLASH_BUFFER;
		cmd->dst = addr + n * 516;
		cmd->len = ((n < 3) ? 516 : 500);
		cmd++;
	}

	/* read extra data */
	cmd->cmd = 0;
	cmd->src = NAND_FLASH_BUFFER + 500;
	cmd->dst = spareaddr;
	cmd->len = 16;
	cmd++;

	/* restore saved ecc config */
	cmd->cmd = CMD_OCU | CMD_LC;
	cmd->src = paddr(&data->ecc_cfg_save);
	cmd->dst = NAND_EBI2_ECC_BUF_CFG;
	cmd->len = 4;

	ptr[0] = (paddr(cmdlist) >> 3) | CMD_PTR_LP;

	dmov_exec_cmdptr(DMOV_NAND_CHAN, ptr);

#if VERBOSE
	dprintf(INFO, "read page %d: status: %x %x %x %x\n",
		page, data[5], data[6], data[7], data[8]);
	for(n = 0; n < 4; n++) {
		ptr = (unsigned*)(addr + 512 * n);
		dprintf(INFO, "data%d:   %x %x %x %x\n", n, ptr[0], ptr[1], ptr[2], ptr[3]);
		ptr = (unsigned*)(spareaddr + 16 * n);
		dprintf(INFO, "spare data%d   %x %x %x %x\n", n, ptr[0], ptr[1], ptr[2], ptr[3]);
	}
#endif

	/* if any of the writes failed (0x10), or there was a
	 ** protection violation (0x100), we lose
	 */
	for(n = 0; n < 4; n++) {
		if (data->result[n].flash_status & 0x110) {
			return -1;
		}
	}

	return 0;
}

static int _flash_write_page(dmov_s *cmdlist, unsigned *ptrlist, unsigned page,
			     const void *_addr, const void *_spareaddr)
{
	dmov_s *cmd = cmdlist;
	unsigned *ptr = ptrlist;
	struct data_flash_io *data = (void*) (ptrlist + 4);
	unsigned addr = (unsigned) _addr;
	unsigned spareaddr = (unsigned) _spareaddr;
	unsigned n;    

	data->cmd = NAND_CMD_PRG_PAGE;
	data->addr0 = page << 16;
	data->addr1 = (page >> 16) & 0xff;
	data->chipsel = 0 | 4; /* flash0 + undoc bit */

	data->cfg0 = CFG0;
	data->cfg1 = CFG1;

	/* GO bit for the EXEC register */
	data->exec = 1;

	data->ecc_cfg = 0x203;

	/* save existing ecc config */
	cmd->cmd = CMD_OCB;
	cmd->src = NAND_EBI2_ECC_BUF_CFG;
	cmd->dst = paddr(&data->ecc_cfg_save);
	cmd->len = 4;
	cmd++;

	for(n = 0; n < 4; n++) {
		/* write CMD / ADDR0 / ADDR1 / CHIPSEL regs in a burst */
		cmd->cmd = DST_CRCI_NAND_CMD;
		cmd->src = paddr(&data->cmd);
		cmd->dst = NAND_FLASH_CMD;
		cmd->len = ((n == 0) ? 16 : 4);
		cmd++;

		if (n == 0) {
			/*  set configuration */
			cmd->cmd = 0;
			cmd->src = paddr(&data->cfg0);
			cmd->dst = NAND_DEV0_CFG0;
			cmd->len = 8;
			cmd++;

			/* set our ecc config */
			cmd->cmd = 0;
			cmd->src = paddr(&data->ecc_cfg);
			cmd->dst = NAND_EBI2_ECC_BUF_CFG;
			cmd->len = 4;
			cmd++;
		}

		/* write data block */
		cmd->cmd = 0;
		cmd->src = addr + n * 516;
		cmd->dst = NAND_FLASH_BUFFER;
		cmd->len = ((n < 3) ? 516 : 510);
		cmd++;

		if (n == 3) {
			/* write extra data */
			cmd->cmd = 0;
			cmd->src = spareaddr;
			cmd->dst = NAND_FLASH_BUFFER + 500;
			cmd->len = 16;
			cmd++;
		}

		/* kick the execute register */
		cmd->cmd = 0;
		cmd->src = paddr(&data->exec);
		cmd->dst = NAND_EXEC_CMD;
		cmd->len = 4;
		cmd++;

		/* block on data ready, then read the status register */
		cmd->cmd = SRC_CRCI_NAND_DATA;
		cmd->src = NAND_FLASH_STATUS;
		cmd->dst = paddr(&data->result[n]);
		cmd->len = 8;
		cmd++;
	}

	/* restore saved ecc config */
	cmd->cmd = CMD_OCU | CMD_LC;
	cmd->src = paddr(&data->ecc_cfg_save);
	cmd->dst = NAND_EBI2_ECC_BUF_CFG;
	cmd->len = 4;

	ptr[0] = (paddr(cmdlist) >> 3) | CMD_PTR_LP;

	dmov_exec_cmdptr(DMOV_NAND_CHAN, ptr);

#if VERBOSE
	dprintf(INFO, "write page %d: status: %x %x %x %x\n",
		page, data[5], data[6], data[7], data[8]);
#endif

	/* if any of the writes failed (0x10), or there was a
	 ** protection violation (0x100), or the program success
	 ** bit (0x80) is unset, we lose
	 */
	for(n = 0; n < 4; n++) {
		if(data->result[n].flash_status & 0x110) return -1;
		if(!(data->result[n].flash_status & 0x80)) return -1;
	}

	return 0;
}

static int flash_read_config(dmov_s *cmdlist, unsigned *ptrlist)
{
	cmdlist[0].cmd = CMD_OCB;
	cmdlist[0].src = NAND_DEV0_CFG0;
	cmdlist[0].dst = paddr(&CFG0);
	cmdlist[0].len = 4;

	cmdlist[1].cmd = CMD_OCU | CMD_LC;
	cmdlist[1].src = NAND_DEV0_CFG1;
	cmdlist[1].dst = paddr(&CFG1);
	cmdlist[1].len = 4;

	*ptrlist = (paddr(cmdlist) >> 3) | CMD_PTR_LP;

	dmov_exec_cmdptr(DMOV_NAND_CHAN, ptrlist);

	if((CFG0 == 0) || (CFG1 == 0)) {
		return -1;
	}

	dprintf(INFO, "nandcfg: %x %x (initial)\n", CFG0, CFG1);

	CFG0 = (3 <<  6)  /* 4 codeword per page for 2k nand */
		|  (516 <<  9)  /* 516 user data bytes */
		|   (10 << 19)  /* 10 parity bytes */
		|    (5 << 27)  /* 5 address cycles */
		|    (1 << 30)  /* Read status before data */
		|    (1 << 31)  /* Send read cmd */
		/* 0 spare bytes for 16 bit nand or 1 spare bytes for 8 bit */
		| ((CFG1 & CFG1_WIDE_FLASH) ? (0 << 23) : (1 << 23));
	CFG1 = (0 <<  0)  /* Enable ecc */
		|    (7 <<  2)  /* 8 recovery cycles */
		|    (0 <<  5)  /* Allow CS deassertion */
		|  (465 <<  6)  /* Bad block marker location */
		|    (0 << 16)  /* Bad block in user data area */
		|    (2 << 17)  /* 6 cycle tWB/tRB */
		| (CFG1 & CFG1_WIDE_FLASH); /* preserve wide flash flag */

	dprintf(INFO, "nandcfg: %x %x (used)\n", CFG0, CFG1);

	return 0;
}

static unsigned *flash_ptrlist;
static dmov_s *flash_cmdlist;
static void *flash_spare;
static void *flash_data;

static struct ptable *flash_ptable = NULL;

void flash_init(struct ptable *new_ptable)
{
	ASSERT(flash_ptable == NULL && new_ptable != NULL);

	flash_ptable = new_ptable;

	flash_ptrlist = memalign(32, 1024);
	flash_cmdlist = memalign(32, 1024);
	flash_data = memalign(32, 2048);
	flash_spare = memalign(32, 64);

	if(flash_read_config(flash_cmdlist, flash_ptrlist)) {
		dprintf(CRITICAL, "ERROR: could not read CFG0/CFG1 state\n");
		ASSERT(0);
	}

	flash_read_id(flash_cmdlist, flash_ptrlist);
}

struct ptable *flash_get_ptable(void)
{
	return flash_ptable;
}

int flash_erase(struct ptentry *ptn)
{
	unsigned block = ptn->start;
	unsigned count = ptn->length;

	while(count-- > 0) {
		if(flash_erase_block(flash_cmdlist, flash_ptrlist, block * 64)) {
			dprintf(INFO, "cannot erase @ %d (bad block?)\n", block);
		}
		block++;
	}
	return 0;
}

int flash_read_ext(struct ptentry *ptn, unsigned extra_per_page,
		   unsigned offset, void *data, unsigned bytes)
{
	unsigned page = (ptn->start * 64) + (offset / 2048);
	unsigned lastpage = (ptn->start + ptn->length) * 64;
	unsigned count = (bytes + 2047 + extra_per_page) / (2048 + extra_per_page);
	unsigned *spare = (unsigned*) flash_spare;
	unsigned errors = 0;
	unsigned char *image = data;

	if(offset & 2047)
		return -1;

	while(page < lastpage) {
		if(count == 0) {
			dprintf(INFO, "flash_read_image: success (%d errors)\n", errors);
			return 0;
		}

		if(_flash_read_page(flash_cmdlist, flash_ptrlist, page++, image, spare)) {
			errors++;
			continue;
		}
		image += 2048;
		memcpy(image, spare, extra_per_page);
		image += extra_per_page;
		count -= 1;
	}

	/* could not find enough valid pages before we hit the end */
	dprintf(INFO, "flash_read_image: failed (%d errors)\n", errors);
	return 0xffffffff;
}

int flash_write(struct ptentry *ptn, unsigned extra_per_page, const void *data,
		unsigned bytes)
{
	unsigned page = ptn->start * 64;
	unsigned lastpage = (ptn->start + ptn->length) * 64;
	unsigned *spare = (unsigned*) flash_spare;
	const unsigned char *image = data;
	unsigned wsize = 2048 + extra_per_page;
	unsigned n;
	int r;

	for(n = 0; n < 16; n++) spare[n] = 0xffffffff;

	while(bytes > 0) {
		if(bytes < wsize) {
			dprintf(CRITICAL, "flash_write_image: image undersized (%d < %d)\n", bytes, wsize);
			return -1;
		}
		if(page >= lastpage) {
			dprintf(CRITICAL, "flash_write_image: out of space\n");
			return -1;
		}

		if((page & 63) == 0) {
			if(flash_erase_block(flash_cmdlist, flash_ptrlist, page)) {
				dprintf(INFO, "flash_write_image: bad block @ %d\n", page >> 6);
				page += 64;
				continue;
			}
		}

		if(extra_per_page) {
			r = _flash_write_page(flash_cmdlist, flash_ptrlist, page++, image, image + 2048);
		} else {
			r = _flash_write_page(flash_cmdlist, flash_ptrlist, page++, image, spare);
		}
		if(r) {
			dprintf(INFO, "flash_write_image: write failure @ page %d (src %d)\n", page, image - (const unsigned char *)data);
			image -= (page & 63) * wsize;
			bytes += (page & 63) * wsize;
			page &= ~63;
			if(flash_erase_block(flash_cmdlist, flash_ptrlist, page)) {
				dprintf(INFO, "flash_write_image: erase failure @ page %d\n", page);
			}
			dprintf(INFO, "flash_write_image: restart write @ page %d (src %d)\n", page, image - (const unsigned char *)data);
			page += 64;
			continue;
		}

		image += wsize;
		bytes -= wsize;
	}

	/* erase any remaining pages in the partition */
	page = (page + 63) & (~63);
	while(page < lastpage){
		if(flash_erase_block(flash_cmdlist, flash_ptrlist, page)) {
			dprintf(INFO, "flash_write_image: bad block @ %d\n", page >> 6);
		}
		page += 64;
	}

	dprintf(INFO, "flash_write_image: success\n");
	return 0;
}

#if 0
static int flash_read_page(unsigned page, void *data, void *extra)
{
	return _flash_read_page(flash_cmdlist, flash_ptrlist,
				page, data, extra);
}
#endif
