/*
 * Copyright (c) 2013 Corey Tabaka
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

#include <reg.h>
#include <debug.h>
#include <assert.h>
#include <err.h>
#include <malloc.h>
#include <arch/x86.h>
#include <sys/types.h>
#include <platform/interrupts.h>
#include <platform/ide.h>
#include <platform/pc.h>
#include <platform.h>
#include <dev/pci.h>
#include <dev/driver.h>
#include <dev/class/block.h>
#include <kernel/event.h>

#define LOCAL_TRACE 1

// status register bits
#define IDE_CTRL_BSY   0x80
#define IDE_DRV_RDY    0x40
#define IDE_DRV_WRTFLT 0x20
#define IDE_DRV_SKCOMP 0x10
#define IDE_DRV_DRQ    0x08
#define IDE_DRV_CORDAT 0x04
#define IDE_DRV_IDX    0x02
#define IDE_DRV_ERR    0x01

// ATA commands
#define ATA_NOP            0x00
#define ATA_ATAPIRESET     0x08
#define ATA_RECALIBRATE    0x10
#define ATA_READMULT_RET   0x20
#define ATA_READMULT       0x21
#define ATA_READECC_RET    0x22
#define ATA_READECC        0x23
#define ATA_WRITEMULT_RET  0x30
#define ATA_WRITEMULT      0x31
#define ATA_WRITEECC_RET   0x32
#define ATA_WRITEECC       0x33
#define ATA_VERIFYMULT_RET 0x40
#define ATA_VERIFYMULT     0x41
#define ATA_FORMATTRACK    0x50
#define ATA_SEEK           0x70
#define ATA_DIAG           0x90
#define ATA_INITPARAMS     0x91
#define ATA_ATAPIPACKET    0xA0
#define ATA_ATAPIIDENTIFY  0xA1
#define ATA_ATAPISERVICE   0xA2
#define ATA_READ_DMA		0xC8
#define ATA_READ_DMA_EXT	0x25
#define ATA_WRITE_DMA		0xCA
#define ATA_WRITE_DMA_EXT	0x35
#define ATA_GETDEVINFO     0xEC
#define ATA_ATAPISETFEAT   0xEF

// error codes
#define IDE_NOERROR			0
#define IDE_ADDRESSMARK     1
#define IDE_CYLINDER0       2
#define IDE_INVALIDCOMMAND  3
#define IDE_MEDIAREQ        4
#define IDE_SECTNOTFOUND    5
#define IDE_MEDIACHANGED    6
#define IDE_BADDATA         7
#define IDE_BADSECTOR       8
#define IDE_TIMEOUT         9
#define IDE_DMAERROR		10

enum {
	IDE_REG_DATA			= 0,
	IDE_REG_ERROR			= 1,
	IDE_REG_PRECOMP			= 1,
	IDE_REG_SECTOR_COUNT	= 2,
	IDE_REG_SECTOR_NUM		= 3,
	IDE_REG_CYLINDER_LOW	= 4,
	IDE_REG_CYLINDER_HIGH	= 5,
	IDE_REG_DRIVE_HEAD		= 6,
	IDE_REG_STATUS			= 7,
	IDE_REG_COMMAND			= 7,
	IDE_REG_ALT_STATUS		= 8,
	IDE_REG_DEVICE_CONTROL	= 8,

	IDE_REG_NUM,
};

enum {
	TYPE_NONE,
	TYPE_UNKNOWN,
	TYPE_FLOPPY,
	TYPE_IDECDROM,
	TYPE_SCSICDROM,
	TYPE_IDEDISK,
	TYPE_SCSIDISK
};

static const char *ide_type_str[] = {
	"None",
	"Unknown",
	"Floppy",
	"IDE CDROM",
	"SCSI CDROM",
	"IDE Disk",
	"SCSI Disk",
};

static const char *ide_error_str[] = {
  "Unknown error",
  "Address mark not found",
  "Cylinder 0 not found",
  "Command aborted - invalid command",
  "Media change requested",
  "ID or target sector not found",
  "Media changed",
  "Uncorrectable data error",
  "Bad sector detected",
  "Command timed out",
  "DMA error"
};

struct ide_driver_state {
	int irq;
	const uint16_t *regs;

	event_t completion;

	int type[2];
	struct {
		int sectors;
		int sector_size;
	} drive[2];
};

static const uint16_t ide_device_regs[][IDE_REG_NUM] = {
  { 0x01F0, 0x01F1, 0x01F2, 0x01F3, 0x01F4, 0x01F5, 0x01F6, 0x01F7, 0x03F6 },
  { 0x0170, 0x0171, 0x0172, 0x0173, 0x0174, 0x0175, 0x0176, 0x0177, 0x0376 },
};

static const int ide_device_irqs[] = {
	INT_IDE0,
	INT_IDE1,
};

static status_t ide_init(struct device *dev);

static enum handler_return ide_irq_handler(void *arg);

static status_t ide_init(struct device *dev);
static ssize_t ide_get_block_size(struct device *dev);
static ssize_t ide_get_block_count(struct device *dev);
static ssize_t ide_write(struct device *dev, off_t offset, const void *buf, size_t count);
static ssize_t ide_read(struct device *dev, off_t offset, void *buf, size_t count);

static struct block_ops the_ops = {
	.std = {
		.init = ide_init,
	},
	.get_block_size = ide_get_block_size,
	.get_block_count = ide_get_block_count,
	.write = ide_write,
	.read = ide_read,
};

DRIVER_EXPORT(ide, &the_ops.std);

static uint8_t ide_read_reg8(struct device *dev, int index);
static uint16_t ide_read_reg16(struct device *dev, int index);
static uint32_t ide_read_reg32(struct device *dev, int index);

static void ide_write_reg8(struct device *dev, int index, uint8_t value);
static void ide_write_reg16(struct device *dev, int index, uint16_t value);
static void ide_write_reg32(struct device *dev, int index, uint32_t value);

static void ide_read_reg8_array(struct device *dev, int index, void *buf, size_t count);
static void ide_read_reg16_array(struct device *dev, int index, void *buf, size_t count);
static void ide_read_reg32_array(struct device *dev, int index, void *buf, size_t count);

static void ide_write_reg8_array(struct device *dev, int index, const void *buf, size_t count);
static void ide_write_reg16_array(struct device *dev, int index, const void *buf, size_t count);
static void ide_write_reg32_array(struct device *dev, int index, const void *buf, size_t count);

static void ide_device_select(struct device *dev, int index);
static void ide_device_reset(struct device *dev);
static void ide_delay_400ns(struct device *dev);
static int ide_poll_status(struct device *dev, uint8_t on_mask, uint8_t off_mask);
static int ide_eval_error(struct device *dev);
static void ide_detect_drives(struct device *dev);
static int ide_wait_for_completion(struct device *dev);
static int ide_detect_ata(struct device *dev, int index);
static void ide_lba_setup(struct device *dev, uint32_t addr, int index);

static status_t ide_init(struct device *dev)
{
	pci_location_t loc;
	pci_config_t pci_config;
	status_t res = NO_ERROR;
	uint32_t i;
	int err;

	if (!dev)
		return ERR_INVALID_ARGS;
	
	if (!dev->config)
		return ERR_NOT_CONFIGURED;
	
	const struct platform_ide_config *config = dev->config;

	err = pci_find_pci_class_code(&loc, 0x010180, 0);
	if (err != _PCI_SUCCESSFUL) {
		LTRACEF("Failed to find IDE device\n");
		res = ERR_NOT_FOUND;
	}

	LTRACEF("Found IDE device at %02x:%02x\n", loc.bus, loc.dev_fn);

	for (i=0; i < sizeof(pci_config) / sizeof(uint32_t); i++) {
		uint32_t reg = sizeof(uint32_t) * i;

		err = pci_read_config_word(&loc, reg, ((uint32_t *) &pci_config) + i);
		if (err != _PCI_SUCCESSFUL) {
			LTRACEF("Failed to read config reg %d: 0x%02x\n", reg, err);
			res = ERR_NOT_CONFIGURED;
			goto done;
		}
	}

	for (i=0; i < 6; i++) {
		LTRACEF("BAR[%d]: 0x%08x\n", i, pci_config.base_addresses[i]);
	}

	struct ide_driver_state *state = malloc(sizeof(struct ide_driver_state));
	if (!state) {
		res = ERR_NO_MEMORY;
		goto done;
	}
	dev->state = state;

	/* TODO: select io regs and irq based on device index */
	state->irq = ide_device_irqs[0];
	state->regs = ide_device_regs[0];
	state->type[0] = state->type[1] = TYPE_NONE;

	event_init(&state->completion, false, EVENT_FLAG_AUTOUNSIGNAL);

	register_int_handler(state->irq, ide_irq_handler, dev);
	unmask_interrupt(state->irq);

	/* enable interrupts */
	ide_write_reg8(dev, IDE_REG_DEVICE_CONTROL, 0);

	/* detect drives */
	ide_detect_drives(dev);

done:
	return res;
}

static enum handler_return ide_irq_handler(void *arg)
{
	struct device *dev = arg;
	struct ide_driver_state *state = dev->state;
	uint8_t val;

	val = ide_read_reg8(dev, IDE_REG_STATUS);

	if ((val & IDE_DRV_ERR) == 0) {
		event_signal(&state->completion, false);	

		return INT_RESCHEDULE;
	} else {
		return INT_NO_RESCHEDULE;
	}
}

static ssize_t ide_get_block_size(struct device *dev)
{
	DEBUG_ASSERT(dev);
	DEBUG_ASSERT(dev->state);

	struct ide_driver_state *state = dev->state;
	return state->drive[0].sector_size;
}

static ssize_t ide_get_block_count(struct device *dev)
{
	DEBUG_ASSERT(dev);
	DEBUG_ASSERT(dev->state);

	struct ide_driver_state *state = dev->state;
	return state->drive[0].sectors;
}

static ssize_t ide_write(struct device *dev, off_t offset, const void *buf, size_t count)
{
	DEBUG_ASSERT(dev);
	DEBUG_ASSERT(dev->state);

	struct ide_driver_state *state = dev->state;

	size_t sectors, do_sectors, i;
	const uint16_t *ubuf = buf;
	int index = 0; // hard code drive for now
	ssize_t ret = 0;
	int err;

	ide_device_select(dev, index);
	ide_delay_400ns(dev);

	err = ide_poll_status(dev, 0, IDE_CTRL_BSY | IDE_DRV_DRQ);
	if (err) {
		LTRACEF("Error while waiting for controller: %s\n", ide_error_str[err]);
		ret = ERR_GENERIC;
		goto done;
	}

	sectors = count;
	
	while (sectors > 0) {
		do_sectors = sectors;

		if (do_sectors > 256)
			do_sectors = 256;

		err = ide_poll_status(dev, 0, IDE_CTRL_BSY);
		if (err) {
			LTRACEF("Error while waiting for controller: %s\n", ide_error_str[err]);
			ret = ERR_GENERIC;
			goto done;
		}

		ide_lba_setup(dev, offset, index);

		if (do_sectors == 256)
			ide_write_reg8(dev, IDE_REG_SECTOR_COUNT, 0);
		else
			ide_write_reg8(dev, IDE_REG_SECTOR_COUNT, do_sectors);

		err = ide_poll_status(dev, IDE_DRV_RDY, 0);
		if (err) {
			LTRACEF("Error while waiting for controller: %s\n", ide_error_str[err]);
			ret = ERR_GENERIC;
			goto done;
		}

		ide_write_reg8(dev, IDE_REG_COMMAND, ATA_WRITEMULT_RET);
		ide_delay_400ns(dev);

		for (i=0; i < do_sectors; i++) {
			err = ide_poll_status(dev, IDE_DRV_DRQ, 0);
			if (err) {
				LTRACEF("Error while waiting for drive: %s\n", ide_error_str[err]);
				ret = ERR_GENERIC;
				goto done;
			}
			
			ide_write_reg16_array(dev, IDE_REG_DATA, ubuf, 256);

			ubuf += 256;
		}

		err = ide_wait_for_completion(dev);
		if (err) {
			LTRACEF("Error waiting for completion: %s\n", ide_error_str[err]);
			ret = ERR_TIMED_OUT;
			goto done;
		}

		sectors -= do_sectors;
		offset += do_sectors;
	}

	ret = count;

done:
	return ret;
}

static ssize_t ide_read(struct device *dev, off_t offset, void *buf, size_t count)
{
	DEBUG_ASSERT(dev);
	DEBUG_ASSERT(dev->state);

	struct ide_driver_state *state = dev->state;

	size_t sectors, do_sectors, i;
	uint16_t *ubuf = buf;
	int index = 0; // hard code drive for now
	ssize_t ret = 0;
	int err;

	ide_device_select(dev, index);
	ide_delay_400ns(dev);

	err = ide_poll_status(dev, 0, IDE_CTRL_BSY | IDE_DRV_DRQ);
	if (err) {
		LTRACEF("Error while waiting for controller: %s\n", ide_error_str[err]);
		ret = ERR_GENERIC;
		goto done;
	}

	sectors = count;
	
	while (sectors > 0) {
		do_sectors = sectors;

		if (do_sectors > 256)
			do_sectors = 256;

		err = ide_poll_status(dev, 0, IDE_CTRL_BSY);
		if (err) {
			LTRACEF("Error while waiting for controller: %s\n", ide_error_str[err]);
			ret = ERR_GENERIC;
			goto done;
		}

		ide_lba_setup(dev, offset, index);

		if (do_sectors == 256)
			ide_write_reg8(dev, IDE_REG_SECTOR_COUNT, 0);
		else
			ide_write_reg8(dev, IDE_REG_SECTOR_COUNT, do_sectors);

		err = ide_poll_status(dev, IDE_DRV_RDY, 0);
		if (err) {
			LTRACEF("Error while waiting for controller: %s\n", ide_error_str[err]);
			ret = ERR_GENERIC;
			goto done;
		}

		ide_write_reg8(dev, IDE_REG_COMMAND, ATA_READMULT_RET);
		ide_delay_400ns(dev);

		for (i=0; i < do_sectors; i++) {
			err = ide_poll_status(dev, IDE_DRV_DRQ, 0);
			if (err) {
				LTRACEF("Error while waiting for drive: %s\n", ide_error_str[err]);
				ret = ERR_GENERIC;
				goto done;
			}
			
			ide_read_reg16_array(dev, IDE_REG_DATA, ubuf, 256);

			ubuf += 256;
		}

		err = ide_wait_for_completion(dev);
		if (err) {
			LTRACEF("Error waiting for completion: %s\n", ide_error_str[err]);
			ret = ERR_TIMED_OUT;
			goto done;
		}

		sectors -= do_sectors;
		offset += do_sectors;
	}

	ret = count;

done:
	return ret;
}

static uint8_t ide_read_reg8(struct device *dev, int index)
{
	DEBUG_ASSERT(index >= 0 && index < IDE_REG_NUM);

	struct ide_driver_state *state = dev->state;

	return inp(state->regs[index]);
}

static uint16_t ide_read_reg16(struct device *dev, int index)
{
	DEBUG_ASSERT(index >= 0 && index < IDE_REG_NUM);

	struct ide_driver_state *state = dev->state;

	return inpw(state->regs[index]);
}

static uint32_t ide_read_reg32(struct device *dev, int index)
{
	DEBUG_ASSERT(index >= 0 && index < IDE_REG_NUM);

	struct ide_driver_state *state = dev->state;

	return inpd(state->regs[index]);
}

static void ide_read_reg8_array(struct device *dev, int index, void *buf, size_t count)
{
	DEBUG_ASSERT(index >= 0 && index < IDE_REG_NUM);

	struct ide_driver_state *state = dev->state;

	inprep(state->regs[index], (uint8_t *) buf, count);
}

static void ide_read_reg16_array(struct device *dev, int index, void *buf, size_t count)
{
	DEBUG_ASSERT(index >= 0 && index < IDE_REG_NUM);

	struct ide_driver_state *state = dev->state;

	inpwrep(state->regs[index], (uint16_t *) buf, count);
}

static void ide_read_reg32_array(struct device *dev, int index, void *buf, size_t count)
{
	DEBUG_ASSERT(index >= 0 && index < IDE_REG_NUM);

	struct ide_driver_state *state = dev->state;

	inpdrep(state->regs[index], (uint32_t *) buf, count);
}

static void ide_write_reg8_array(struct device *dev, int index, const void *buf, size_t count)
{
	DEBUG_ASSERT(index >= 0 && index < IDE_REG_NUM);

	struct ide_driver_state *state = dev->state;

	outprep(state->regs[index], (uint8_t *) buf, count);
}

static void ide_write_reg16_array(struct device *dev, int index, const void *buf, size_t count)
{
	DEBUG_ASSERT(index >= 0 && index < IDE_REG_NUM);

	struct ide_driver_state *state = dev->state;

	outpwrep(state->regs[index], (uint16_t *) buf, count);
}

static void ide_write_reg32_array(struct device *dev, int index, const void *buf, size_t count)
{
	DEBUG_ASSERT(index >= 0 && index < IDE_REG_NUM);

	struct ide_driver_state *state = dev->state;

	outpdrep(state->regs[index], (uint32_t *) buf, count);
}

static void ide_write_reg8(struct device *dev, int index, uint8_t value)
{
	DEBUG_ASSERT(index >= 0 && index < IDE_REG_NUM);

	struct ide_driver_state *state = dev->state;

	outp(state->regs[index], value);
}

static void ide_write_reg16(struct device *dev, int index, uint16_t value)
{
	DEBUG_ASSERT(index >= 0 && index < IDE_REG_NUM);

	struct ide_driver_state *state = dev->state;

	outpw(state->regs[index], value);
}

static void ide_write_reg32(struct device *dev, int index, uint32_t value)
{
	DEBUG_ASSERT(index >= 0 && index < IDE_REG_NUM);

	struct ide_driver_state *state = dev->state;

	outpd(state->regs[index], value);
}

static void ide_device_select(struct device *dev, int index)
{
	ide_write_reg8(dev, IDE_REG_DRIVE_HEAD, (index & 1) << 4);
}

static void ide_delay_400ns(struct device *dev)
{
	ide_read_reg8(dev, IDE_REG_ALT_STATUS);
	ide_read_reg8(dev, IDE_REG_ALT_STATUS);
	ide_read_reg8(dev, IDE_REG_ALT_STATUS);
	ide_read_reg8(dev, IDE_REG_ALT_STATUS);
}

static void ide_device_reset(struct device *dev)
{
	struct ide_driver_state *state = dev->state;

	lk_time_t start;
	uint8_t sect_cnt, sect_num;
	int err;

	ide_device_select(dev, 0);
	ide_delay_400ns(dev);
	
	// set bit 2 for at least 4.8us
	ide_write_reg8(dev, IDE_REG_DEVICE_CONTROL, 1<<2);
	
	// delay 5us
	spin(5);
	
	ide_write_reg8(dev, IDE_REG_DEVICE_CONTROL, 0x00);

	err = ide_poll_status(dev, 0, IDE_CTRL_BSY);
	if (err) {
		LTRACEF("Failed while waiting for controller to be ready: %s\n", ide_error_str[err]);
		return;
	}

	// make sure the slave is ready if present
	if (state->type[1] != TYPE_NONE) {
		ide_device_select(dev, 1);
		ide_delay_400ns(dev);
		
		start = current_time();

		do {
			sect_cnt = ide_read_reg8(dev, IDE_REG_SECTOR_COUNT);
			sect_num = ide_read_reg8(dev, IDE_REG_SECTOR_NUM);

			if (sect_cnt == 1 && sect_num == 1) {
				err = ide_poll_status(dev, 0, IDE_CTRL_BSY);
				if (err) {
					LTRACEF("Failed while waiting for slave ready: %s\n", ide_error_str[err]);
					return;
				}

				break;
			}
		} while (TIME_LTE(current_time(), start + 20000));

		err = ide_read_reg8(dev, IDE_REG_ALT_STATUS);
		if (err & IDE_DRV_ERR) {
			err = ide_eval_error(dev);
			LTRACEF("Failed while resetting controller: %s\n", ide_error_str[err]);
			return;
		}
	}
}

static int ide_eval_error(struct device *dev)
{
	int err = 0;
	uint8_t data = 0;
	
	data = ide_read_reg8(dev, IDE_REG_ERROR);
	
	if (data & 0x01) {
		err = IDE_ADDRESSMARK;
	} else if (data & 0x02) {
		err = IDE_CYLINDER0;
	} else if (data & 0x04) {
		err = IDE_INVALIDCOMMAND;
	} else if (data & 0x08) {
		err = IDE_MEDIAREQ;
	} else if (data & 0x10) {
		err = IDE_SECTNOTFOUND;
	} else if (data & 0x20) {
		err = IDE_MEDIACHANGED;
	} else if (data & 0x40) {
		err = IDE_BADDATA;
	} else if (data & 0x80) {
		err = IDE_BADSECTOR;
	} else {
		err = IDE_NOERROR;
	}

	return err;
}

static int ide_poll_status(struct device *dev, uint8_t on_mask, uint8_t off_mask)
{
	int err;
	uint8_t value;
	lk_time_t start = current_time();

	do {
		value = ide_read_reg8(dev, IDE_REG_ALT_STATUS);

		if (value & IDE_DRV_ERR) {
			err = ide_eval_error(dev);
			LTRACEF("Error while polling status: %s\n", ide_error_str[err]);
			return err;
		}

		if ((value & on_mask) == on_mask && (value & off_mask) == 0)
			return IDE_NOERROR; 
	} while (TIME_LTE(current_time(), start + 20000));

	return IDE_TIMEOUT;
}

static void ide_detect_drives(struct device *dev)
{
	struct ide_driver_state *state = dev->state;
	uint8_t sc = 0, sn = 0, st = 0, cl = 0, ch = 0;

	ide_device_select(dev, 0);
	ide_delay_400ns(dev);
	ide_delay_400ns(dev);

	ide_write_reg8(dev, IDE_REG_SECTOR_COUNT, 0x55);
	ide_write_reg8(dev, IDE_REG_SECTOR_NUM, 0xaa);
	ide_write_reg8(dev, IDE_REG_SECTOR_COUNT, 0xaa);
	ide_write_reg8(dev, IDE_REG_SECTOR_NUM, 0x55);
	ide_write_reg8(dev, IDE_REG_SECTOR_COUNT, 0x55);
	ide_write_reg8(dev, IDE_REG_SECTOR_NUM, 0xaa);
	
	sc = ide_read_reg8(dev, IDE_REG_SECTOR_COUNT);
	sn = ide_read_reg8(dev, IDE_REG_SECTOR_NUM);
	
	if (sc == 0x55 && sn == 0xaa) {
		state->type[0] = TYPE_UNKNOWN;
	}
	
	// check for device 1
	ide_device_select(dev, 1);
	ide_delay_400ns(dev);
	
	ide_write_reg8(dev, IDE_REG_SECTOR_COUNT, 0x55);
	ide_write_reg8(dev, IDE_REG_SECTOR_NUM, 0xaa);
	ide_write_reg8(dev, IDE_REG_SECTOR_COUNT, 0xaa);
	ide_write_reg8(dev, IDE_REG_SECTOR_NUM, 0x55);
	ide_write_reg8(dev, IDE_REG_SECTOR_COUNT, 0x55);
	ide_write_reg8(dev, IDE_REG_SECTOR_NUM, 0xaa);
	
	sc = ide_read_reg8(dev, IDE_REG_SECTOR_COUNT);
	sn = ide_read_reg8(dev, IDE_REG_SECTOR_NUM);
	
	if (sc == 0x55 && sn == 0xaa) {
		state->type[1] = TYPE_UNKNOWN;
	}
	
	// now the drives present should be known
	// soft reset now
	ide_device_select(dev, 0);
	ide_delay_400ns(dev);
	ide_device_reset(dev);

	ide_device_select(dev, 0);
	ide_delay_400ns(dev);
	
	sc = ide_read_reg8(dev, IDE_REG_SECTOR_COUNT);
	sn = ide_read_reg8(dev, IDE_REG_SECTOR_NUM);
	if (sc == 0x01 && sn == 0x01) {
		state->type[0] = TYPE_UNKNOWN;

		st = ide_read_reg8(dev, IDE_REG_STATUS);
		cl = ide_read_reg8(dev, IDE_REG_CYLINDER_LOW);
		ch = ide_read_reg8(dev, IDE_REG_CYLINDER_HIGH);

		// PATAPI or SATAPI respectively
		if ((cl == 0x14 && ch == 0xeb) || (cl == 0x69 && ch == 0x96)) {
			state->type[0] = TYPE_IDECDROM;
		} else if (st != 0 && ((cl == 0x00 && ch == 0x00) || (cl == 0x3c && ch == 0xc3))) {
			state->type[0] = TYPE_IDEDISK;
		}
	}

	ide_device_select(dev, 1);
	ide_delay_400ns(dev);
	
	sc = ide_read_reg8(dev, IDE_REG_SECTOR_COUNT);
	sn = ide_read_reg8(dev, IDE_REG_SECTOR_NUM);
	if (sc == 0x01 && sn == 0x01) {
		state->type[1] = TYPE_UNKNOWN;

		st = ide_read_reg8(dev, IDE_REG_STATUS);
		cl = ide_read_reg8(dev, IDE_REG_CYLINDER_LOW);
		ch = ide_read_reg8(dev, IDE_REG_CYLINDER_HIGH);

		// PATAPI or SATAPI respectively
		if ((cl == 0x14 && ch == 0xeb) || (cl == 0x69 && ch == 0x96)) {
			state->type[1] = TYPE_IDECDROM;
		} else if (st != 0 && ((cl == 0x00 && ch == 0x00) || (cl == 0x3c && ch == 0xc3))) {
			state->type[1] = TYPE_IDEDISK;
		}
	}

	LTRACEF("Detected drive 0: %s\n", ide_type_str[state->type[0]]);
	LTRACEF("Detected drive 1: %s\n", ide_type_str[state->type[1]]);

	switch (state->type[0]) {
		case TYPE_IDEDISK:
			ide_detect_ata(dev, 0);
			break;

		default:
			break;
	}

	switch (state->type[1]) {
		case TYPE_IDEDISK:
			ide_detect_ata(dev, 1);
			break;

		default:
			break;
	}
}

static int ide_wait_for_completion(struct device *dev)
{
	struct ide_driver_state *state = dev->state;
	status_t err;

	err = event_wait_timeout(&state->completion, 20000);
	if (err)
		return IDE_TIMEOUT;
	
	return IDE_NOERROR;
}

static status_t ide_detect_ata(struct device *dev, int index)
{
	struct ide_driver_state *state = dev->state;
	status_t res = NO_ERROR;
	uint8_t *info = NULL;
	int err;

	ide_device_select(dev, index);
	ide_delay_400ns(dev);

	err = ide_poll_status(dev, 0, IDE_CTRL_BSY | IDE_DRV_DRQ);
	if (err) {
		LTRACEF("Error while detecting drive %d: %s\n", index, ide_error_str[err]);
		res = ERR_TIMED_OUT;
		goto error;
	}

	ide_device_select(dev, index);
	ide_delay_400ns(dev);

	err = ide_poll_status(dev, 0, IDE_CTRL_BSY | IDE_DRV_DRQ);
	if (err) {
		LTRACEF("Error while detecting drive %d: %s\n", index, ide_error_str[err]);
		res = ERR_TIMED_OUT;
		goto error;
	}
	
	// try to wait for the selected drive to be ready, but don't quit if not
	// since CD-ROMs don't seem to respond to this when they're masters
	ide_poll_status(dev, IDE_DRV_RDY, 0);
	
	// send the "identify device" command
	ide_write_reg8(dev, IDE_REG_COMMAND, ATA_GETDEVINFO);
	ide_delay_400ns(dev);

	err = ide_wait_for_completion(dev);
	if (err) {
		LTRACEF("Error while waiting for command: %s\n", ide_error_str[err]);
		res = ERR_TIMED_OUT;
		goto error;
	}

	info = malloc(512);
	if (!info) {
		res = ERR_NO_MEMORY;
		goto error;
	}

	LTRACEF("Found ATA hard disk on channel %d!\n", index);

	ide_read_reg16_array(dev, IDE_REG_DATA, info, 256);

	state->drive[index].sectors = *((uint32_t *) (info + 120));
	state->drive[index].sector_size = 512;

	LTRACEF("Disk supports %u sectors for a total of %u bytes\n", state->drive[index].sectors,
			state->drive[index].sectors * 512);

error:
	free(info);
	return res;
}

static void ide_lba_setup(struct device *dev, uint32_t addr, int drive)
{
	ide_write_reg8(dev, IDE_REG_DRIVE_HEAD, 0xe0 | ((drive & 0x00000001) << 4) | ((addr >> 24) & 0xf));
	ide_write_reg8(dev, IDE_REG_CYLINDER_LOW, (addr >> 8) & 0xff);
	ide_write_reg8(dev, IDE_REG_CYLINDER_HIGH, (addr >> 16) & 0xff);
	ide_write_reg8(dev, IDE_REG_SECTOR_NUM, addr & 0xff);
	ide_write_reg8(dev, IDE_REG_PRECOMP, 0xff);
}

