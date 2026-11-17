/*
 * Copyright (c) 2013 Corey Tabaka
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <arch/x86.h>
#include <assert.h>
#include <dev/block/ide.h>
#include <kernel/mutex.h>
#include <kernel/event.h>
#include <lib/bio.h>
#include <limits.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/reg.h>
#include <lk/trace.h>
#include <malloc.h>
#include <platform.h>
#include <platform/interrupts.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include <lib/cmdline.h>

#define LOCAL_TRACE 0

// how long to wait for a drive's interrupt before polling its status instead
#define IDE_IRQ_TIMEOUT 2000

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
#define ATA_NOP               0x00
#define ATA_ATAPIRESET        0x08
#define ATA_RECALIBRATE       0x10
#define ATA_READMULT_RET      0x20
#define ATA_READMULT          0x21
#define ATA_READECC_RET       0x22
#define ATA_READECC           0x23
#define ATA_WRITEMULT_RET     0x30
#define ATA_WRITEMULT         0x31
#define ATA_WRITEECC_RET      0x32
#define ATA_WRITEECC          0x33
#define ATA_VERIFYMULT_RET    0x40
#define ATA_VERIFYMULT        0x41
#define ATA_FORMATTRACK       0x50
#define ATA_SEEK              0x70
#define ATA_DIAG              0x90
#define ATA_INITPARAMS        0x91
#define ATA_ATAPIPACKET       0xA0
#define ATA_ATAPIIDENTIFY     0xA1
#define ATA_ATAPISERVICE      0xA2
#define ATA_READ_DMA          0xC8
#define ATA_READ_SECTORS_EXT  0x24
#define ATA_READ_DMA_EXT      0x25
#define ATA_WRITE_DMA         0xCA
#define ATA_WRITE_SECTORS_EXT 0x34
#define ATA_WRITE_DMA_EXT     0x35
#define ATA_GETDEVINFO        0xEC
#define ATA_ATAPISETFEAT      0xEF

// error codes
#define IDE_NOERROR        0
#define IDE_ADDRESSMARK    1
#define IDE_CYLINDER0      2
#define IDE_INVALIDCOMMAND 3
#define IDE_MEDIAREQ       4
#define IDE_SECTNOTFOUND   5
#define IDE_MEDIACHANGED   6
#define IDE_BADDATA        7
#define IDE_BADSECTOR      8
#define IDE_TIMEOUT        9
#define IDE_DMAERROR       10

enum {
    IDE_REG_DATA = 0,
    IDE_REG_ERROR = 1,
    IDE_REG_PRECOMP = 1,
    IDE_REG_SECTOR_COUNT = 2,
    IDE_REG_SECTOR_NUM = 3,
    IDE_REG_CYLINDER_LOW = 4,
    IDE_REG_CYLINDER_HIGH = 5,
    IDE_REG_DRIVE_HEAD = 6,
    IDE_REG_STATUS = 7,
    IDE_REG_COMMAND = 7,
    IDE_REG_ALT_STATUS = 8,
    IDE_REG_DEVICE_CONTROL = 8,

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
    "DMA error"};

struct ide_driver_state {
    int irq;
    uint16_t regs[IDE_REG_NUM];

    event_t completion;
    mutex_t lock;
    uint8_t irq_status; // status register as the last interrupt read it
    bool polled;        // the irq never arrived once, so status is polled from then on

    int type[2];
    struct {
        int sectors;
        int sector_size;
        bool lba;   // LBA28 addressing; CHS otherwise
        bool lba48;
        uint16_t cyls, heads, spt; // default CHS geometry from IDENTIFY
    } drive[2];
};

struct ide_device {
    const struct platform_ide_config *config;
    struct ide_driver_state *state;
};

struct ide_bdev {
    bdev_t bdev;
    struct ide_device *dev;
    int drive;
    bool registered;
    char name[16];
};

struct ide_channel_state {
    bool initialized;
    struct platform_ide_config config;
    struct ide_driver_state driver_state;
    struct ide_device dev;
    struct ide_bdev bdev[2];
};

static struct ide_channel_state g_ide_channels[2];

static enum handler_return ide_irq_handler(void *arg);

static status_t ide_init(struct ide_device *dev);
static status_t ide_register_bdev(struct ide_channel_state *chan, int drive);
static ssize_t ide_get_block_size(struct ide_device *dev, int drive);
static ssize_t ide_get_block_count(struct ide_device *dev, int drive);
static ssize_t ide_write(struct ide_device *dev, int drive, off_t offset, const void *buf, size_t count);
static ssize_t ide_read(struct ide_device *dev, int drive, off_t offset, void *buf, size_t count);
static ssize_t ide_bdev_read_block(struct bdev *bdev, void *buf, bnum_t block, uint count);
static ssize_t ide_bdev_write_block(struct bdev *bdev, const void *buf, bnum_t block, uint count);

static uint8_t ide_read_reg8(struct ide_device *dev, int index);
static uint16_t ide_read_reg16(struct ide_device *dev, int index);
static uint32_t ide_read_reg32(struct ide_device *dev, int index);

static void ide_write_reg8(struct ide_device *dev, int index, uint8_t value);
static void ide_write_reg16(struct ide_device *dev, int index, uint16_t value);
static void ide_write_reg32(struct ide_device *dev, int index, uint32_t value);

static void ide_read_reg8_array(struct ide_device *dev, int index, void *buf, size_t count);
static void ide_read_reg16_array(struct ide_device *dev, int index, void *buf, size_t count);
static void ide_read_reg32_array(struct ide_device *dev, int index, void *buf, size_t count);

static void ide_write_reg8_array(struct ide_device *dev, int index, const void *buf, size_t count);
static void ide_write_reg16_array(struct ide_device *dev, int index, const void *buf, size_t count);
static void ide_write_reg32_array(struct ide_device *dev, int index, const void *buf, size_t count);

static void ide_device_select(struct ide_device *dev, int index);
static void ide_device_reset(struct ide_device *dev);
static void ide_delay_400ns(struct ide_device *dev);
static int ide_poll_status(struct ide_device *dev, uint8_t on_mask, uint8_t off_mask);
static int ide_eval_error(struct ide_device *dev);
static void ide_detect_drives(struct ide_device *dev);
static bool ide_probe_registers(struct ide_device *dev);
static int ide_detect_ata(struct ide_device *dev, int index);
static void ide_lba_setup(struct ide_device *dev, uint32_t addr, int drive);
static void ide_lba48_setup(struct ide_device *dev, uint64_t addr, uint16_t count, int drive);

status_t platform_ide_init(const struct platform_ide_config *config) {
    if (!config) {
        return ERR_INVALID_ARGS;
    }

    uint32_t channel = config->channel;
    if (channel >= countof(g_ide_channels)) {
        return ERR_OUT_OF_RANGE;
    }

    struct ide_channel_state *chan = &g_ide_channels[channel];
    if (chan->initialized) {
        return ERR_ALREADY_EXISTS;
    }

    chan->config = *config;
    chan->dev.config = &chan->config;
    chan->dev.state = &chan->driver_state;

    status_t err = ide_init(&chan->dev);
    if (err != NO_ERROR) {
        return err;
    }

    for (int drive = 0; drive < 2; ++drive) {
        err = ide_register_bdev(chan, drive);
        if (err != NO_ERROR && err != ERR_NOT_FOUND) {
            dprintf(INFO, "ide: failed to register channel %u drive %d with bio: %d\n",
                    channel, drive, err);
        }
    }

    chan->initialized = true;
    return NO_ERROR;
}

static status_t ide_init(struct ide_device *dev) {
    if (!dev) {
        return ERR_INVALID_ARGS;
    }

    if (!dev->config) {
        return ERR_NOT_CONFIGURED;
    }

    const struct platform_ide_config *config = dev->config;

    LTRACEF("entry: dev %p, config %p\n", dev, config);

    struct ide_driver_state *state = dev->state;
    if (!state) {
        return ERR_INVALID_ARGS;
    }

    memset(state, 0, sizeof(*state));

    state->irq = config->irq;
    state->regs[IDE_REG_DATA] = config->io_base + 0;
    state->regs[IDE_REG_ERROR] = config->io_base + 1;
    state->regs[IDE_REG_SECTOR_COUNT] = config->io_base + 2;
    state->regs[IDE_REG_SECTOR_NUM] = config->io_base + 3;
    state->regs[IDE_REG_CYLINDER_LOW] = config->io_base + 4;
    state->regs[IDE_REG_CYLINDER_HIGH] = config->io_base + 5;
    state->regs[IDE_REG_DRIVE_HEAD] = config->io_base + 6;
    state->regs[IDE_REG_STATUS] = config->io_base + 7;
    state->regs[IDE_REG_ALT_STATUS] = config->ctrl_base;

    state->type[0] = state->type[1] = TYPE_NONE;

    // Nothing decodes the ports when there is no controller, so the status reads back
    // as all ones; with a controller, a drive holds what is written to its sector count
    // and number registers. Neither means no channel, and its irq is left alone.
    if (ide_read_reg8(dev, IDE_REG_STATUS) == 0xff || !ide_probe_registers(dev)) {
        LTRACEF("no controller at %#x\n", config->io_base);
        return ERR_NOT_FOUND;
    }

    event_init(&state->completion, false, EVENT_FLAG_AUTOUNSIGNAL);
    mutex_init(&state->lock);

    register_int_handler(state->irq, ide_irq_handler, dev);
    unmask_interrupt(state->irq);

    /* enable interrupts */
    ide_write_reg8(dev, IDE_REG_DEVICE_CONTROL, 0);

    /* detect drives */
    ide_detect_drives(dev);

    return NO_ERROR;
}

static status_t ide_register_bdev(struct ide_channel_state *chan, int drive) {
    DEBUG_ASSERT(chan);

    if (drive < 0 || drive > 1) {
        return ERR_OUT_OF_RANGE;
    }

    struct ide_device *dev = &chan->dev;
    struct ide_driver_state *state = dev->state;
    DEBUG_ASSERT(state);

    if (state->type[drive] != TYPE_IDEDISK ||
        state->drive[drive].sector_size == 0 ||
        state->drive[drive].sectors <= 0) {
        return ERR_NOT_FOUND;
    }

    struct ide_bdev *ide_bdev = &chan->bdev[drive];
    if (ide_bdev->registered) {
        return ERR_ALREADY_EXISTS;
    }

    memset(ide_bdev, 0, sizeof(*ide_bdev));

    ide_bdev->dev = dev;
    ide_bdev->drive = drive;
    snprintf(ide_bdev->name, sizeof(ide_bdev->name), "ide%u.%d", chan->config.channel, drive);

    bio_initialize_bdev(&ide_bdev->bdev,
                        ide_bdev->name,
                        state->drive[drive].sector_size,
                        (bnum_t)state->drive[drive].sectors,
                        0,
                        NULL,
                        BIO_FLAGS_NONE);

    ide_bdev->bdev.read_block = ide_bdev_read_block;
    ide_bdev->bdev.write_block = ide_bdev_write_block;

    bio_register_device(&ide_bdev->bdev);
    ide_bdev->registered = true;

    dprintf(INFO, "ide: registered block device '%s'\n", ide_bdev->name);

    return NO_ERROR;
}

static ssize_t ide_bdev_read_block(struct bdev *bdev, void *buf, bnum_t block, uint count) {
    struct ide_bdev *ide_bdev = (struct ide_bdev *)bdev;
    uint trim_count = bio_trim_block_range(bdev, block, count);
    if (trim_count == 0) {
        return 0;
    }

    ssize_t sectors = ide_read(ide_bdev->dev, ide_bdev->drive, (off_t)block, buf, trim_count);
    if (sectors < 0) {
        return sectors;
    }

    return sectors * (ssize_t)bdev->block_size;
}

static ssize_t ide_bdev_write_block(struct bdev *bdev, const void *buf, bnum_t block, uint count) {
    struct ide_bdev *ide_bdev = (struct ide_bdev *)bdev;
    uint trim_count = bio_trim_block_range(bdev, block, count);
    if (trim_count == 0) {
        return 0;
    }

    ssize_t sectors = ide_write(ide_bdev->dev, ide_bdev->drive, (off_t)block, buf, trim_count);
    if (sectors < 0) {
        return sectors;
    }

    return sectors * (ssize_t)bdev->block_size;
}

// Reading the status register is what deasserts the drive's interrupt. The waiter
// looks at the status it recorded, errors included.
static enum handler_return ide_irq_handler(void *arg) {
    struct ide_device *dev = arg;
    struct ide_driver_state *state = dev->state;

    state->irq_status = ide_read_reg8(dev, IDE_REG_STATUS);
    event_signal(&state->completion, false);

    return INT_RESCHEDULE;
}

static ssize_t ide_get_block_size(struct ide_device *dev, int drive) {
    DEBUG_ASSERT(dev);
    DEBUG_ASSERT(dev->state);
    DEBUG_ASSERT(drive >= 0 && drive < 2);

    struct ide_driver_state *state = dev->state;
    return state->drive[drive].sector_size;
}

static ssize_t ide_get_block_count(struct ide_device *dev, int drive) {
    DEBUG_ASSERT(dev);
    DEBUG_ASSERT(dev->state);
    DEBUG_ASSERT(drive >= 0 && drive < 2);

    struct ide_driver_state *state = dev->state;
    return state->drive[drive].sectors;
}

// Wait for the drive's interrupt at the end of a sector (or of the command), then check
// the status it left. If the irq never arrives (a board with IRQ14/15 not wired, or set
// to something else) but the drive is plainly done, the channel switches to polling.
static int ide_wait_irq(struct ide_device *dev, bool want_drq) {
    struct ide_driver_state *state = dev->state;
    uint8_t status;

    if (!state->polled && event_wait_timeout(&state->completion, IDE_IRQ_TIMEOUT) == NO_ERROR) {
        status = state->irq_status;
    } else {
        int err = ide_poll_status(dev, 0, IDE_CTRL_BSY);
        if (err != IDE_NOERROR) {
            return err;
        }
        status = ide_read_reg8(dev, IDE_REG_STATUS);
        if (!state->polled && event_wait_timeout(&state->completion, 0) == NO_ERROR) {
            // only slow (a drive spinning up), the irq did arrive
            status = state->irq_status;
        } else if (!state->polled) {
            dprintf(INFO, "ide%u: no interrupt on vector %#x, polling from now on\n",
                    dev->config->channel, dev->config->irq);
            state->polled = true;
        }
    }

    if (status & (IDE_DRV_ERR | IDE_DRV_WRTFLT)) {
        return (status & IDE_DRV_ERR) ? ide_eval_error(dev) : IDE_BADDATA;
    }
    if (want_drq && !(status & IDE_DRV_DRQ)) {
        // the interrupt can come a little ahead of DRQ on some drives
        return ide_poll_status(dev, IDE_DRV_DRQ, IDE_CTRL_BSY);
    }
    return IDE_NOERROR;
}

static void ide_chs_setup(struct ide_device *dev, uint32_t addr, int drive) {
    struct ide_driver_state *state = dev->state;
    const uint32_t spt = state->drive[drive].spt;
    const uint32_t heads = state->drive[drive].heads;

    const uint32_t cyl = addr / (spt * heads);
    const uint32_t head = (addr / spt) % heads;
    const uint32_t sector = addr % spt + 1;

    ide_write_reg8(dev, IDE_REG_DRIVE_HEAD, 0xa0 | ((drive & 1) << 4) | (head & 0xf));
    ide_write_reg8(dev, IDE_REG_CYLINDER_LOW, cyl & 0xff);
    ide_write_reg8(dev, IDE_REG_CYLINDER_HIGH, (cyl >> 8) & 0xff);
    ide_write_reg8(dev, IDE_REG_SECTOR_NUM, sector);
    ide_write_reg8(dev, IDE_REG_PRECOMP, 0xff);
}

// PIO data transfer of count sectors. Each sector of a read is announced by an interrupt
// with DRQ set; a write supplies the first sector on DRQ alone, and the interrupt after
// each sector asks for the next one or ends the command.
static ssize_t ide_transfer(struct ide_device *dev, int drive, off_t offset, void *buf,
                            size_t count, bool write) {
    DEBUG_ASSERT(dev);
    DEBUG_ASSERT(dev->state);

    struct ide_driver_state *state = dev->state;

    if (drive < 0 || drive > 1) {
        return ERR_OUT_OF_RANGE;
    }
    if (offset < 0) {
        return ERR_INVALID_ARGS;
    }
    if (state->type[drive] != TYPE_IDEDISK) {
        return ERR_NOT_FOUND;
    }
    if (count == 0) {
        return 0;
    }

    uint64_t max_blocks = (uint64_t)ide_get_block_count(dev, drive);
    uint64_t start = (uint64_t)offset;
    if (start >= max_blocks) {
        return 0;
    }
    uint64_t available = max_blocks - start;
    if ((uint64_t)count > available) {
        count = (size_t)available;
    }

    mutex_acquire(&state->lock);

    uint16_t *ubuf = buf;
    size_t sectors = count;
    ssize_t ret;
    int err;

    ide_device_select(dev, drive);
    ide_delay_400ns(dev);

    while (sectors > 0) {
        const size_t do_sectors = MIN(sectors, 256u);

        err = ide_poll_status(dev, 0, IDE_CTRL_BSY | IDE_DRV_DRQ);
        if (err) {
            goto error;
        }

        if (state->drive[drive].lba48) {
            ide_lba48_setup(dev, (uint64_t)offset, (uint16_t)do_sectors, drive);
        } else {
            if (state->drive[drive].lba) {
                ide_lba_setup(dev, offset, drive);
            } else {
                ide_chs_setup(dev, offset, drive);
            }
            ide_write_reg8(dev, IDE_REG_SECTOR_COUNT, do_sectors & 0xff); // 0 means 256
        }

        err = ide_poll_status(dev, IDE_DRV_RDY, 0);
        if (err) {
            goto error;
        }

        uint8_t cmd;
        if (write) {
            cmd = state->drive[drive].lba48 ? ATA_WRITE_SECTORS_EXT : ATA_WRITEMULT_RET;
        } else {
            cmd = state->drive[drive].lba48 ? ATA_READ_SECTORS_EXT : ATA_READMULT_RET;
        }
        event_unsignal(&state->completion);
        ide_write_reg8(dev, IDE_REG_COMMAND, cmd);
        ide_delay_400ns(dev);

        for (size_t i = 0; i < do_sectors; i++) {
            if (write) {
                err = (i == 0) ? ide_poll_status(dev, IDE_DRV_DRQ, IDE_CTRL_BSY)
                               : ide_wait_irq(dev, true);
                if (err) {
                    goto error;
                }
                ide_write_reg16_array(dev, IDE_REG_DATA, ubuf, 256);
            } else {
                err = ide_wait_irq(dev, true);
                if (err) {
                    goto error;
                }
                ide_read_reg16_array(dev, IDE_REG_DATA, ubuf, 256);
            }
            ubuf += 256;
        }

        if (write) {
            err = ide_wait_irq(dev, false);
            if (err) {
                goto error;
            }
        }

        sectors -= do_sectors;
        offset += (off_t)do_sectors;
    }

    ret = (ssize_t)count;
    mutex_release(&state->lock);
    return ret;

error:
    dprintf(INFO, "ide: %s of drive %d at sector %lld failed: %s\n", write ? "write" : "read",
            drive, (long long)offset, ide_error_str[err]);
    ret = (err == IDE_TIMEOUT) ? ERR_TIMED_OUT : ERR_IO;
    mutex_release(&state->lock);
    return ret;
}

static ssize_t ide_write(struct ide_device *dev, int drive, off_t offset, const void *buf, size_t count) {
    return ide_transfer(dev, drive, offset, (void *)buf, count, true);
}

static ssize_t ide_read(struct ide_device *dev, int drive, off_t offset, void *buf, size_t count) {
    return ide_transfer(dev, drive, offset, buf, count, false);
}

static uint8_t ide_read_reg8(struct ide_device *dev, int index) {
    DEBUG_ASSERT(index >= 0 && index < IDE_REG_NUM);

    struct ide_driver_state *state = dev->state;

    return inp(state->regs[index]);
}

static uint16_t ide_read_reg16(struct ide_device *dev, int index) {
    DEBUG_ASSERT(index >= 0 && index < IDE_REG_NUM);

    struct ide_driver_state *state = dev->state;

    return inpw(state->regs[index]);
}

static uint32_t ide_read_reg32(struct ide_device *dev, int index) {
    DEBUG_ASSERT(index >= 0 && index < IDE_REG_NUM);

    struct ide_driver_state *state = dev->state;

    return inpd(state->regs[index]);
}

static void ide_read_reg8_array(struct ide_device *dev, int index, void *buf, size_t count) {
    DEBUG_ASSERT(index >= 0 && index < IDE_REG_NUM);

    struct ide_driver_state *state = dev->state;

    inprep(state->regs[index], (uint8_t *)buf, count);
}

static void ide_read_reg16_array(struct ide_device *dev, int index, void *buf, size_t count) {
    DEBUG_ASSERT(index >= 0 && index < IDE_REG_NUM);

    struct ide_driver_state *state = dev->state;

    inpwrep(state->regs[index], (uint16_t *)buf, count);
}

static void ide_read_reg32_array(struct ide_device *dev, int index, void *buf, size_t count) {
    DEBUG_ASSERT(index >= 0 && index < IDE_REG_NUM);

    struct ide_driver_state *state = dev->state;

    inpdrep(state->regs[index], (uint32_t *)buf, count);
}

static void ide_write_reg8_array(struct ide_device *dev, int index, const void *buf, size_t count) {
    DEBUG_ASSERT(index >= 0 && index < IDE_REG_NUM);

    struct ide_driver_state *state = dev->state;

    outprep(state->regs[index], (uint8_t *)buf, count);
}

static void ide_write_reg16_array(struct ide_device *dev, int index, const void *buf, size_t count) {
    DEBUG_ASSERT(index >= 0 && index < IDE_REG_NUM);

    struct ide_driver_state *state = dev->state;

    outpwrep(state->regs[index], (uint16_t *)buf, count);
}

static void ide_write_reg32_array(struct ide_device *dev, int index, const void *buf, size_t count) {
    DEBUG_ASSERT(index >= 0 && index < IDE_REG_NUM);

    struct ide_driver_state *state = dev->state;

    outpdrep(state->regs[index], (uint32_t *)buf, count);
}

static void ide_write_reg8(struct ide_device *dev, int index, uint8_t value) {
    DEBUG_ASSERT(index >= 0 && index < IDE_REG_NUM);

    struct ide_driver_state *state = dev->state;

    outp(state->regs[index], value);
}

static void ide_write_reg16(struct ide_device *dev, int index, uint16_t value) {
    DEBUG_ASSERT(index >= 0 && index < IDE_REG_NUM);

    struct ide_driver_state *state = dev->state;

    outpw(state->regs[index], value);
}

static void ide_write_reg32(struct ide_device *dev, int index, uint32_t value) {
    DEBUG_ASSERT(index >= 0 && index < IDE_REG_NUM);

    struct ide_driver_state *state = dev->state;

    outpd(state->regs[index], value);
}

// bits 7 and 5 are obsolete, but drives of the CHS era expect them set
static void ide_device_select(struct ide_device *dev, int index) {
    ide_write_reg8(dev, IDE_REG_DRIVE_HEAD, 0xa0 | ((index & 1) << 4));
}

static void ide_delay_400ns(struct ide_device *dev) {
    ide_read_reg8(dev, IDE_REG_ALT_STATUS);
    ide_read_reg8(dev, IDE_REG_ALT_STATUS);
    ide_read_reg8(dev, IDE_REG_ALT_STATUS);
    ide_read_reg8(dev, IDE_REG_ALT_STATUS);
}

static void ide_device_reset(struct ide_device *dev) {
    struct ide_driver_state *state = dev->state;

    lk_time_t start;
    uint8_t sect_cnt, sect_num;
    int err;

    ide_device_select(dev, 0);
    ide_delay_400ns(dev);

    // set bit 2 for at least 4.8us
    ide_write_reg8(dev, IDE_REG_DEVICE_CONTROL, 1 << 2);

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

static int ide_eval_error(struct ide_device *dev) {
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

static int ide_poll_status(struct ide_device *dev, uint8_t on_mask, uint8_t off_mask) {
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

        if ((value & on_mask) == on_mask && (value & off_mask) == 0) {
            return IDE_NOERROR;
        }
    } while (TIME_LTE(current_time(), start + 20000));

    return IDE_TIMEOUT;
}

// true if either drive position holds a pattern written to its sector count and number
// registers, which only a drive does
static bool ide_probe_registers(struct ide_device *dev) {
    for (int index = 0; index < 2; index++) {
        ide_device_select(dev, index);
        ide_delay_400ns(dev);

        ide_write_reg8(dev, IDE_REG_SECTOR_COUNT, 0x55);
        ide_write_reg8(dev, IDE_REG_SECTOR_NUM, 0xaa);
        if (ide_read_reg8(dev, IDE_REG_SECTOR_COUNT) == 0x55 &&
            ide_read_reg8(dev, IDE_REG_SECTOR_NUM) == 0xaa) {
            return true;
        }
    }
    return false;
}

static void ide_detect_drives(struct ide_device *dev) {
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

        LTRACEF("sc %hhu sn %hhu st %hhu cl %hhu ch %hhu\n", sc, sn, st, cl, ch);

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

        LTRACEF("sc %hhu sn %hhu st %hhu cl %hhu ch %hhu\n", sc, sn, st, cl, ch);

        // PATAPI or SATAPI respectively
        if ((cl == 0x14 && ch == 0xeb) || (cl == 0x69 && ch == 0x96)) {
            state->type[1] = TYPE_IDECDROM;
        } else if (st != 0 && ((cl == 0x00 && ch == 0x00) || (cl == 0x3c && ch == 0xc3))) {
            state->type[1] = TYPE_IDEDISK;
        }
    }

    dprintf(INFO, "ide%u: drive 0: %s, drive 1: %s\n", dev->config->channel,
            ide_type_str[state->type[0]], ide_type_str[state->type[1]]);

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


static status_t ide_detect_ata(struct ide_device *dev, int index) {
    struct ide_driver_state *state = dev->state;
    status_t res = NO_ERROR;
    uint16_t *info = NULL;
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
    event_unsignal(&state->completion);
    ide_write_reg8(dev, IDE_REG_COMMAND, ATA_GETDEVINFO);
    ide_delay_400ns(dev);

    err = ide_wait_irq(dev, true);
    if (err) {
        LTRACEF("Error while waiting for command: %s\n", ide_error_str[err]);
        res = ERR_TIMED_OUT;
        goto error;
    }

    info = calloc(1, 512);
    if (!info) {
        res = ERR_NO_MEMORY;
        goto error;
    }

    LTRACEF("Found ATA hard disk on channel %d!\n", index);

    ide_read_reg16_array(dev, IDE_REG_DATA, info, 256);

    // word 49 bit 9: LBA supported. Drives without it (and some early ones that set it
    // but report no LBA capacity) are addressed by cylinder/head/sector.
    const bool force_chs = cmdline_is_present("ide.force_chs");
    const uint32_t lba28_sectors = (uint32_t)info[60] | ((uint32_t)info[61] << 16);
    const bool supports_lba = !force_chs && (info[49] & (1u << 9)) && lba28_sectors > 0;
    const bool supports_lba48 = supports_lba && (info[83] & (1u << 10)) != 0;

    uint64_t sectors;
    if (supports_lba48) {
        sectors = ((uint64_t)info[100]) | ((uint64_t)info[101] << 16) |
                  ((uint64_t)info[102] << 32) | ((uint64_t)info[103] << 48);
    } else if (supports_lba) {
        sectors = lba28_sectors;
    } else {
        const uint16_t cyls = info[1];
        const uint16_t heads = info[3];
        const uint16_t spt = info[6];
        if (!cyls || !heads || heads > 16 || !spt || spt > 255) {
            dprintf(INFO, "ide: disk %d has no LBA and no usable geometry (%u/%u/%u)\n", index,
                    cyls, heads, spt);
            res = ERR_NOT_CONFIGURED;
            goto error;
        }
        state->drive[index].cyls = cyls;
        state->drive[index].heads = heads;
        state->drive[index].spt = spt;
        sectors = (uint64_t)cyls * heads * spt;

        // make the drive's translation match the geometry the addresses are computed with
        ide_device_select(dev, index);
        ide_delay_400ns(dev);
        ide_write_reg8(dev, IDE_REG_SECTOR_COUNT, spt & 0xff);
        ide_write_reg8(dev, IDE_REG_DRIVE_HEAD, 0xa0 | ((index & 1) << 4) | ((heads - 1) & 0xf));
        event_unsignal(&state->completion);
        ide_write_reg8(dev, IDE_REG_COMMAND, ATA_INITPARAMS);
        ide_delay_400ns(dev);
        err = ide_wait_irq(dev, false);
        if (err) {
            dprintf(INFO, "ide: disk %d rejected its own geometry: %s\n", index, ide_error_str[err]);
            res = ERR_IO;
            goto error;
        }
    }

    state->drive[index].lba = supports_lba;
    state->drive[index].lba48 = supports_lba48;
    state->drive[index].sector_size = 512;
    if (sectors > INT_MAX) {
        dprintf(INFO, "ide: disk %d capacity exceeds 32-bit sector tracking, clamping at %d sectors\n",
                index, INT_MAX);
        sectors = INT_MAX;
    }
    state->drive[index].sectors = (int)sectors;

    if (supports_lba) {
        dprintf(INFO, "ide%u: disk %d: %s, %u sectors (%llu MB)\n", dev->config->channel, index,
                supports_lba48 ? "LBA48" : "LBA28", state->drive[index].sectors,
                (unsigned long long)sectors / 2048);
    } else {
        dprintf(INFO, "ide%u: disk %d: CHS %u/%u/%u%s, %u sectors (%llu MB)\n", dev->config->channel, index,
                state->drive[index].cyls, state->drive[index].heads, state->drive[index].spt,
                force_chs ? " (ide.force_chs)" : "", state->drive[index].sectors,
                (unsigned long long)sectors / 2048);
    }

    res = NO_ERROR;

error:
    free(info);
    return res;
}

static void ide_lba48_setup(struct ide_device *dev, uint64_t addr, uint16_t count, int drive) {
    ide_write_reg8(dev, IDE_REG_DRIVE_HEAD, 0xe0 | ((drive & 0x00000001) << 4));

    ide_write_reg8(dev, IDE_REG_PRECOMP, 0x00);
    ide_write_reg8(dev, IDE_REG_SECTOR_COUNT, (count >> 8) & 0xff);
    ide_write_reg8(dev, IDE_REG_SECTOR_NUM, (addr >> 24) & 0xff);
    ide_write_reg8(dev, IDE_REG_CYLINDER_LOW, (addr >> 32) & 0xff);
    ide_write_reg8(dev, IDE_REG_CYLINDER_HIGH, (addr >> 40) & 0xff);

    ide_write_reg8(dev, IDE_REG_PRECOMP, 0x00);
    ide_write_reg8(dev, IDE_REG_SECTOR_COUNT, count & 0xff);
    ide_write_reg8(dev, IDE_REG_SECTOR_NUM, addr & 0xff);
    ide_write_reg8(dev, IDE_REG_CYLINDER_LOW, (addr >> 8) & 0xff);
    ide_write_reg8(dev, IDE_REG_CYLINDER_HIGH, (addr >> 16) & 0xff);
}

static void ide_lba_setup(struct ide_device *dev, uint32_t addr, int drive) {
    ide_write_reg8(dev, IDE_REG_DRIVE_HEAD, 0xe0 | ((drive & 0x00000001) << 4) | ((addr >> 24) & 0xf));
    ide_write_reg8(dev, IDE_REG_CYLINDER_LOW, (addr >> 8) & 0xff);
    ide_write_reg8(dev, IDE_REG_CYLINDER_HIGH, (addr >> 16) & 0xff);
    ide_write_reg8(dev, IDE_REG_SECTOR_NUM, addr & 0xff);
    ide_write_reg8(dev, IDE_REG_PRECOMP, 0xff);
}
