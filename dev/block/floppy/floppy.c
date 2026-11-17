/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

// Read only driver for the PC/AT floppy controller (NEC 765 / Intel 8272A, and the
// 82077AA and its clones, which are register compatible for everything used here).
//
// Data moves by ISA DMA through one bounce buffer allocated at init, a track (one side
// of a cylinder) at most per command. The density of the disk in a drive isn't known
// until it has been read, so the first access after a disk change tries a READ ID at
// each data rate the drive type allows and keeps the first that answers.

#include <dev/block/floppy.h>

#include <arch/x86.h>
#include <dev/bus/isa_dma.h>
#include <kernel/event.h>
#include <kernel/mutex.h>
#include <kernel/spinlock.h>
#include <kernel/thread.h>
#include <kernel/timer.h>
#include <kernel/vm.h>
#include <lib/bio.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/list.h>
#include <lk/trace.h>
#include <platform.h>
#include <platform/interrupts.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOCAL_TRACE 0

#define FLOPPY_SECTOR 512

// register offsets from the controller's base
#define FDC_DOR  2 // digital output
#define FDC_MSR  4 // main status (read)
#define FDC_DATA 5 // command/result FIFO
#define FDC_DIR  7 // digital input (read): bit 7 is the disk change line
#define FDC_CCR  7 // configuration control (write): data rate

#define DOR_NOT_RESET (1u << 2)
#define DOR_DMA_IRQ   (1u << 3)
#define DOR_MOTOR(d)  (1u << (4 + (d)))

#define MSR_RQM (1u << 7) // ready for a byte
#define MSR_DIO (1u << 6) // byte is for the cpu

#define DIR_DISK_CHANGE (1u << 7)

#define CMD_SPECIFY     0x03
#define CMD_RECALIBRATE 0x07
#define CMD_SENSE_INT   0x08
#define CMD_SEEK        0x0f
#define CMD_VERSION     0x10
#define CMD_MFM         0x40
#define CMD_READ_DATA   (0x06 | CMD_MFM)
#define CMD_READ_ID     (0x0a | CMD_MFM)

#define ST0_IC_MASK 0xc0 // interrupt code, 0 is normal termination
#define ST0_SE      0x20 // seek end
#define ST0_EC      0x10 // equipment check: the drive never reached track 0

#define FDC_BYTE_TIMEOUT  100  // ms for the FIFO to take or give a byte
#define FDC_IRQ_TIMEOUT   2000 // ms for a seek or a track's worth of data
#define FDC_MOTOR_SPINUP  500
#define FDC_MOTOR_IDLE    2000 // ms of no use before the motor goes off
#define FDC_HEAD_SETTLE   15
#define FDC_READ_RETRIES  3

// 32KB aligned to 32KB never crosses a 64KB boundary, and holds the largest track (2.88M,
// 36 sectors is 18KB)
#define FDC_BOUNCE_PAGES      8
#define FDC_BOUNCE_ALIGN_LOG2 15

struct floppy_format {
    const char *name;
    uint8_t cyls, heads, spt;
    uint8_t rate; // CCR value: 0 500kbps, 1 300kbps, 2 250kbps, 3 1Mbps
    uint8_t gap3;
    bool double_step; // 40 track disk in an 80 track drive
};

static const struct floppy_format fmt_360k = { "360K", 40, 2, 9, 2, 0x2a, false };
static const struct floppy_format fmt_360k_in_1200k = { "360K", 40, 2, 9, 1, 0x23, true };
static const struct floppy_format fmt_1200k = { "1.2M", 80, 2, 15, 0, 0x1b, false };
static const struct floppy_format fmt_720k = { "720K", 80, 2, 9, 2, 0x1b, false };
static const struct floppy_format fmt_1440k = { "1.44M", 80, 2, 18, 0, 0x1b, false };
static const struct floppy_format fmt_2880k = { "2.88M", 80, 2, 36, 3, 0x1b, false };

// the formats each drive type can read, native first
static const struct floppy_format *const formats_360k[] = { &fmt_360k, NULL };
static const struct floppy_format *const formats_1200k[] = { &fmt_1200k, &fmt_360k_in_1200k, NULL };
static const struct floppy_format *const formats_720k[] = { &fmt_720k, NULL };
static const struct floppy_format *const formats_1440k[] = { &fmt_1440k, &fmt_720k, NULL };
static const struct floppy_format *const formats_2880k[] = { &fmt_2880k, &fmt_1440k, &fmt_720k, NULL };

static const struct floppy_format *const *drive_formats(enum floppy_drive_type type) {
    switch (type) {
        case FLOPPY_TYPE_360K:  return formats_360k;
        case FLOPPY_TYPE_1200K: return formats_1200k;
        case FLOPPY_TYPE_720K:  return formats_720k;
        case FLOPPY_TYPE_1440K: return formats_1440k;
        case FLOPPY_TYPE_2880K: return formats_2880k;
        default:                return NULL;
    }
}

const char *floppy_drive_type_name(enum floppy_drive_type type) {
    switch (type) {
        case FLOPPY_TYPE_NONE:  return "none";
        case FLOPPY_TYPE_360K:  return "5.25\" 360K";
        case FLOPPY_TYPE_1200K: return "5.25\" 1.2M";
        case FLOPPY_TYPE_720K:  return "3.5\" 720K";
        case FLOPPY_TYPE_1440K: return "3.5\" 1.44M";
        case FLOPPY_TYPE_2880K: return "3.5\" 2.88M";
    }
    return "unknown";
}

struct fdc;

struct floppy_drive {
    bdev_t bdev; // first, the bio callbacks cast back to the drive
    struct fdc *fdc;
    uint index;
    enum floppy_drive_type type;
    const struct floppy_format *media; // NULL until a disk has been read
    int cur_cyl;                       // physical cylinder the head is on, -1 unknown
    char name[8];
};

struct fdc {
    uint16_t base;
    unsigned int irq;
    uint dma_channel;

    mutex_t lock; // one command at a time, for both drives
    event_t irq_event;

    spin_lock_t dor_lock;
    uint8_t dor;
    bool busy; // a command is running: the motor timer leaves the motors alone
    timer_t motor_timer;

    paddr_t bounce_pa;
    uint8_t *bounce;

    uint8_t rate; // CCR value last programmed, 0xff for none
    struct floppy_drive drive[2];
};

static struct fdc the_fdc;

static void dor_write(struct fdc *fdc, uint8_t dor) {
    arch_interrupt_saved_state_t state = spin_lock_irqsave(&fdc->dor_lock);
    fdc->dor = dor;
    outp(fdc->base + FDC_DOR, dor);
    spin_unlock_irqrestore(&fdc->dor_lock, state);
}

static enum handler_return fdc_irq(void *arg) {
    struct fdc *fdc = arg;
    event_signal(&fdc->irq_event, false);
    return INT_RESCHEDULE;
}

static bool fdc_wait_msr(struct fdc *fdc, uint8_t want) {
    const lk_time_t start = current_time();
    do {
        if ((inp(fdc->base + FDC_MSR) & (MSR_RQM | MSR_DIO)) == want) {
            return true;
        }
    } while (current_time() - start < FDC_BYTE_TIMEOUT);
    return false;
}

static status_t fdc_send(struct fdc *fdc, const uint8_t *bytes, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (!fdc_wait_msr(fdc, MSR_RQM)) {
            LTRACEF("timeout sending byte %zu of command %#x\n", i, bytes[0]);
            return ERR_TIMED_OUT;
        }
        outp(fdc->base + FDC_DATA, bytes[i]);
    }
    return NO_ERROR;
}

static status_t fdc_result(struct fdc *fdc, uint8_t *bytes, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (!fdc_wait_msr(fdc, MSR_RQM | MSR_DIO)) {
            LTRACEF("timeout reading result byte %zu\n", i);
            return ERR_TIMED_OUT;
        }
        bytes[i] = inp(fdc->base + FDC_DATA);
    }
    return NO_ERROR;
}

static status_t fdc_wait_irq(struct fdc *fdc) {
    return event_wait_timeout(&fdc->irq_event, FDC_IRQ_TIMEOUT);
}

static status_t fdc_sense_int(struct fdc *fdc, uint8_t *st0, uint8_t *cyl) {
    const uint8_t cmd = CMD_SENSE_INT;
    uint8_t res[2];
    status_t err = fdc_send(fdc, &cmd, 1);
    if (err == NO_ERROR) {
        err = fdc_result(fdc, res, 2);
    }
    if (err == NO_ERROR) {
        *st0 = res[0];
        *cyl = res[1];
    }
    return err;
}

static void fdc_set_rate(struct fdc *fdc, uint8_t rate) {
    if (fdc->rate != rate) {
        outp(fdc->base + FDC_CCR, rate);
        fdc->rate = rate;
    }
}

// Pulse reset, collect the four drives' reset interrupts and set the step and head load
// timings. Also the way out of a command that will never finish (no disk, no index).
static status_t fdc_reset(struct fdc *fdc) {
    event_unsignal(&fdc->irq_event);
    const uint8_t motors = fdc->dor & (DOR_MOTOR(0) | DOR_MOTOR(1));
    dor_write(fdc, 0);
    spin(20);
    dor_write(fdc, DOR_NOT_RESET | DOR_DMA_IRQ | motors);

    status_t err = fdc_wait_irq(fdc);
    if (err != NO_ERROR) {
        return err;
    }
    for (int i = 0; i < 4; i++) {
        uint8_t st0, cyl;
        fdc_sense_int(fdc, &st0, &cyl);
    }

    fdc->rate = 0xff;
    fdc_set_rate(fdc, 0);

    // step rate 3ms and head unload 240ms at 500kbps, head load 4ms, DMA mode
    const uint8_t specify[3] = { CMD_SPECIFY, 0xdf, 0x02 };
    err = fdc_send(fdc, specify, sizeof(specify));

    fdc->drive[0].cur_cyl = fdc->drive[1].cur_cyl = -1;
    return err;
}

static enum handler_return motor_off(timer_t *timer, lk_time_t now, void *arg) {
    struct fdc *fdc = arg;
    arch_interrupt_saved_state_t state = spin_lock_irqsave(&fdc->dor_lock);
    if (!fdc->busy) {
        fdc->dor &= ~(DOR_MOTOR(0) | DOR_MOTOR(1));
        outp(fdc->base + FDC_DOR, fdc->dor);
    }
    spin_unlock_irqrestore(&fdc->dor_lock, state);
    return INT_NO_RESCHEDULE;
}

// called with the lock held, at the start of every operation on a drive
static void fdc_begin(struct floppy_drive *drv) {
    struct fdc *fdc = drv->fdc;

    timer_cancel(&fdc->motor_timer);

    arch_interrupt_saved_state_t state = spin_lock_irqsave(&fdc->dor_lock);
    fdc->busy = true;
    const bool spinning = fdc->dor & DOR_MOTOR(drv->index);
    fdc->dor = (fdc->dor & ~0x03) | DOR_MOTOR(drv->index) | drv->index;
    outp(fdc->base + FDC_DOR, fdc->dor);
    spin_unlock_irqrestore(&fdc->dor_lock, state);

    if (!spinning) {
        thread_sleep(FDC_MOTOR_SPINUP);
    }
}

static void fdc_end(struct floppy_drive *drv) {
    struct fdc *fdc = drv->fdc;
    arch_interrupt_saved_state_t state = spin_lock_irqsave(&fdc->dor_lock);
    fdc->busy = false;
    spin_unlock_irqrestore(&fdc->dor_lock, state);
    timer_set_oneshot(&fdc->motor_timer, FDC_MOTOR_IDLE, motor_off, fdc);
}

static status_t fdc_recalibrate(struct floppy_drive *drv) {
    struct fdc *fdc = drv->fdc;

    // one recalibrate steps at most 77 times, short of 80 track drives
    for (int tries = 0; tries < 2; tries++) {
        event_unsignal(&fdc->irq_event);
        const uint8_t cmd[2] = { CMD_RECALIBRATE, (uint8_t)drv->index };
        status_t err = fdc_send(fdc, cmd, sizeof(cmd));
        if (err == NO_ERROR) {
            err = fdc_wait_irq(fdc);
        }
        uint8_t st0 = 0, cyl = 0xff;
        if (err == NO_ERROR) {
            err = fdc_sense_int(fdc, &st0, &cyl);
        }
        if (err != NO_ERROR) {
            return err;
        }
        if ((st0 & ST0_SE) && !(st0 & ST0_EC) && cyl == 0) {
            drv->cur_cyl = 0;
            return NO_ERROR;
        }
    }
    drv->cur_cyl = -1;
    return ERR_IO;
}

static status_t fdc_seek(struct floppy_drive *drv, int pcyl, int head) {
    struct fdc *fdc = drv->fdc;

    if (drv->cur_cyl < 0) {
        status_t err = fdc_recalibrate(drv);
        if (err != NO_ERROR) {
            return err;
        }
    }
    if (drv->cur_cyl == pcyl) {
        return NO_ERROR;
    }

    event_unsignal(&fdc->irq_event);
    const uint8_t cmd[3] = { CMD_SEEK, (uint8_t)((head << 2) | drv->index), (uint8_t)pcyl };
    status_t err = fdc_send(fdc, cmd, sizeof(cmd));
    if (err == NO_ERROR) {
        err = fdc_wait_irq(fdc);
    }
    uint8_t st0 = 0, cyl = 0;
    if (err == NO_ERROR) {
        err = fdc_sense_int(fdc, &st0, &cyl);
    }
    if (err != NO_ERROR || !(st0 & ST0_SE) || cyl != pcyl) {
        drv->cur_cyl = -1;
        return (err != NO_ERROR) ? err : ERR_IO;
    }
    drv->cur_cyl = pcyl;
    thread_sleep(FDC_HEAD_SETTLE);
    return NO_ERROR;
}

// a command whose end is reported by an interrupt and seven result bytes
static status_t fdc_run_rw(struct fdc *fdc, const uint8_t *cmd, size_t len, uint8_t res[7]) {
    event_unsignal(&fdc->irq_event);
    status_t err = fdc_send(fdc, cmd, len);
    if (err == NO_ERROR) {
        err = fdc_wait_irq(fdc);
    }
    if (err == NO_ERROR) {
        err = fdc_result(fdc, res, 7);
    }
    if (err != NO_ERROR) {
        // a command that never ends (no disk, no index pulses) only stops on a reset
        fdc_reset(fdc);
        return err;
    }
    return (res[0] & ST0_IC_MASK) ? ERR_IO : NO_ERROR;
}

static status_t fdc_read_id(struct floppy_drive *drv, int head) {
    const uint8_t cmd[2] = { CMD_READ_ID, (uint8_t)((head << 2) | drv->index) };
    uint8_t res[7];
    return fdc_run_rw(drv->fdc, cmd, sizeof(cmd), res);
}

static void set_media(struct floppy_drive *drv, const struct floppy_format *fmt) {
    drv->media = fmt;
    // the size follows the disk; bio reads these for every request
    drv->bdev.block_count = (bnum_t)fmt->cyls * fmt->heads * fmt->spt;
    drv->bdev.total_size = (off_t)drv->bdev.block_count * FLOPPY_SECTOR;
}

// Work out which format the disk is by reading a sector id at each rate the drive could
// use. Stepping off track 0 and back clears the change line when a disk is present.
static status_t detect_media(struct floppy_drive *drv) {
    struct fdc *fdc = drv->fdc;

    drv->media = NULL;
    status_t err = fdc_recalibrate(drv);
    if (err == NO_ERROR) {
        err = fdc_seek(drv, 1, 0);
    }
    if (err == NO_ERROR) {
        err = fdc_seek(drv, 0, 0);
    }
    if (err != NO_ERROR) {
        return err;
    }
    if (drv->type != FLOPPY_TYPE_360K && (inp(fdc->base + FDC_DIR) & DIR_DISK_CHANGE)) {
        return ERR_NOT_READY; // no disk
    }

    for (const struct floppy_format *const *fmt = drive_formats(drv->type); *fmt; fmt++) {
        fdc_set_rate(fdc, (*fmt)->rate);
        if (fdc_read_id(drv, 0) == NO_ERROR) {
            set_media(drv, *fmt);
            dprintf(INFO, "%s: %s disk\n", drv->name, (*fmt)->name);
            return NO_ERROR;
        }
    }
    return ERR_NOT_READY;
}

// read count sectors of one track side into the bounce buffer
static status_t read_track(struct floppy_drive *drv, uint cyl, uint head, uint sector, uint count) {
    struct fdc *fdc = drv->fdc;
    const struct floppy_format *fmt = drv->media;

    status_t err = fdc_seek(drv, fmt->double_step ? cyl * 2 : cyl, head);
    if (err != NO_ERROR) {
        return err;
    }
    fdc_set_rate(fdc, fmt->rate);

    err = isa_dma_start(fdc->dma_channel, fdc->bounce_pa, count * FLOPPY_SECTOR, ISA_DMA_TO_MEMORY);
    if (err != NO_ERROR) {
        return err;
    }

    const uint8_t cmd[9] = {
        CMD_READ_DATA, (uint8_t)((head << 2) | drv->index), (uint8_t)cyl, (uint8_t)head,
        (uint8_t)sector, 2 /* 512 bytes */, (uint8_t)(sector + count - 1) /* last sector */,
        fmt->gap3, 0xff,
    };
    uint8_t res[7];
    err = fdc_run_rw(fdc, cmd, sizeof(cmd), res);
    isa_dma_stop(fdc->dma_channel);
    if (err != NO_ERROR) {
        LTRACEF("%s: read c%u h%u s%u n%u failed: %d, st %#x %#x %#x\n", drv->name, cyl, head,
                sector, count, err, res[0], res[1], res[2]);
    }
    return err;
}

static ssize_t floppy_read_block(bdev_t *bdev, void *_buf, bnum_t block, uint count) {
    struct floppy_drive *drv = (struct floppy_drive *)bdev;
    struct fdc *fdc = drv->fdc;
    uint8_t *buf = _buf;
    ssize_t ret = 0;

    mutex_acquire(&fdc->lock);
    fdc_begin(drv);

    if (!drv->media || (drv->type != FLOPPY_TYPE_360K && (inp(fdc->base + FDC_DIR) & DIR_DISK_CHANGE))) {
        status_t err = detect_media(drv);
        if (err != NO_ERROR) {
            ret = err;
            goto out;
        }
    }

    count = bio_trim_block_range(bdev, block, count);
    while (count > 0) {
        const struct floppy_format *fmt = drv->media;
        const uint track_blocks = fmt->spt;
        const uint cyl = block / (track_blocks * fmt->heads);
        const uint head = (block / track_blocks) % fmt->heads;
        const uint sector = block % track_blocks + 1;
        const uint n = MIN(count, track_blocks - sector + 1);

        status_t err = ERR_IO;
        for (int tries = 0; tries < FDC_READ_RETRIES && err != NO_ERROR; tries++) {
            if (tries > 0) {
                drv->cur_cyl = -1; // recalibrate before the retry
            }
            err = read_track(drv, cyl, head, sector, n);
        }
        if (err != NO_ERROR) {
            dprintf(INFO, "%s: read of block %u failed: %d\n", drv->name, block, err);
            ret = (ret > 0) ? ret : err;
            goto out;
        }

        memcpy(buf, fdc->bounce, n * FLOPPY_SECTOR);
        buf += n * FLOPPY_SECTOR;
        block += n;
        count -= n;
        ret += n * FLOPPY_SECTOR;
    }

out:
    fdc_end(drv);
    mutex_release(&fdc->lock);
    return ret;
}

static bool fdc_present(struct fdc *fdc) {
    if (inp(fdc->base + FDC_MSR) == 0xff) {
        return false;
    }
    // a 765/8272 answers VERSION as an invalid command (0x80), an 82077 with 0x90
    const uint8_t cmd = CMD_VERSION;
    uint8_t version;
    if (fdc_send(fdc, &cmd, 1) != NO_ERROR || fdc_result(fdc, &version, 1) != NO_ERROR) {
        return false;
    }
    dprintf(INFO, "floppy: controller at %#x, %s\n", fdc->base,
            version == 0x90 ? "82077 or later" : "765/8272");
    return true;
}

status_t floppy_init(const struct floppy_config *config) {
    struct fdc *fdc = &the_fdc;

    if (fdc->base) {
        return ERR_ALREADY_EXISTS;
    }
    if (config->drive_type[0] == FLOPPY_TYPE_NONE && config->drive_type[1] == FLOPPY_TYPE_NONE) {
        return ERR_NOT_FOUND;
    }

    memset(fdc, 0, sizeof(*fdc));
    fdc->base = config->io_base;
    fdc->irq = config->irq;
    fdc->dma_channel = config->dma_channel;
    mutex_init(&fdc->lock);
    event_init(&fdc->irq_event, false, EVENT_FLAG_AUTOUNSIGNAL);
    spin_lock_init(&fdc->dor_lock);
    timer_initialize(&fdc->motor_timer);

    if (!fdc_present(fdc)) {
        fdc->base = 0;
        return ERR_NOT_FOUND;
    }

    struct list_node pages = LIST_INITIAL_VALUE(pages);
    if (pmm_alloc_contiguous(FDC_BOUNCE_PAGES, FDC_BOUNCE_ALIGN_LOG2, &fdc->bounce_pa, &pages) !=
        FDC_BOUNCE_PAGES) {
        fdc->base = 0;
        return ERR_NO_MEMORY;
    }
    fdc->bounce = paddr_to_kvaddr(fdc->bounce_pa);
    if (!fdc->bounce || !isa_dma_range_ok(fdc->bounce_pa, FDC_BOUNCE_PAGES * PAGE_SIZE)) {
        dprintf(INFO, "floppy: bounce buffer at %#lx is out of ISA DMA reach\n",
                (ulong)fdc->bounce_pa);
        pmm_free(&pages);
        fdc->base = 0;
        return ERR_NO_MEMORY;
    }

    register_int_handler(fdc->irq, fdc_irq, fdc);
    unmask_interrupt(fdc->irq);

    if (fdc_reset(fdc) != NO_ERROR) {
        dprintf(INFO, "floppy: controller at %#x did not come out of reset\n", fdc->base);
        mask_interrupt(fdc->irq);
        return ERR_NOT_FOUND;
    }

    for (uint i = 0; i < 2; i++) {
        struct floppy_drive *drv = &fdc->drive[i];
        drv->fdc = fdc;
        drv->index = i;
        drv->type = config->drive_type[i];
        drv->cur_cyl = -1;
        const struct floppy_format *const *formats = drive_formats(drv->type);
        if (!formats) {
            continue;
        }

        snprintf(drv->name, sizeof(drv->name), "fd%u", i);
        const struct floppy_format *native = formats[0];
        bio_initialize_bdev(&drv->bdev, drv->name, FLOPPY_SECTOR,
                            (bnum_t)native->cyls * native->heads * native->spt, 0, NULL,
                            BIO_FLAGS_NONE);
        drv->bdev.read_block = floppy_read_block;
        bio_register_device(&drv->bdev);
        dprintf(INFO, "floppy: %s is a %s drive\n", drv->name, floppy_drive_type_name(drv->type));
    }

    return NO_ERROR;
}
