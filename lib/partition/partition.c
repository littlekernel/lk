/*
 * Copyright (c) 2009 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include "lib/partition.h"

#include <lk/debug.h>
#include <stdio.h>
#include <string.h>
#include <lk/compiler.h>
#include <stdlib.h>
#include <arch.h>
#include <lib/bio.h>
#include <assert.h>

struct chs {
    uint8_t c;
    uint8_t h;
    uint8_t s;
};

struct mbr_part {
    uint8_t status;
    struct chs start;
    uint8_t type;
    struct chs end;
    uint32_t lba_start;
    uint32_t lba_length;
};
static_assert(sizeof(struct mbr_part) == 16, "");

static status_t validate_mbr_partition(bdev_t *dev, const struct mbr_part *part) {
    /* check for invalid types */
    if (part->type == 0)
        return -1;
    /* check for invalid status */
    if (part->status != 0x80 && part->status != 0x00)
        return -1;

    /* make sure the range fits within the device */
    if (part->lba_start >= dev->block_count)
        return -1;
    if ((part->lba_start + part->lba_length) > dev->block_count)
        return -1;

    /* that's about all we can do, MBR has no other good way to see if it's valid */

    return 0;
}

int partition_publish(const char *device, off_t offset) {
    int err = 0;
    int count = 0;

    // clear any partitions that may have already existed
    partition_unpublish(device);

    bdev_t *dev = bio_open(device);
    if (!dev) {
        printf("partition_publish: unable to open device\n");
        return -1;
    }

    // get a dma aligned and padded block to read info
    STACKBUF_DMA_ALIGN(buf, dev->block_size);

    /* sniff for MBR partition types */
    do {
        int i;

        err = bio_read(dev, buf, offset, 512);
        if (err < 0)
            goto err;

        /* look for the aa55 tag */
        if (buf[510] != 0x55 || buf[511] != 0xaa)
            break;

        /* see if a partition table makes sense here */
        struct mbr_part part[4];
        memcpy(part, buf + 446, sizeof(part));

#if LK_DEBUGLEVEL >= INFO
        dprintf(INFO, "mbr partition table dump:\n");
        for (i=0; i < 4; i++) {
            dprintf(INFO, "\t%i: status 0x%hhx, type 0x%hhx, start 0x%x, len 0x%x\n", i, part[i].status, part[i].type, part[i].lba_start, part[i].lba_length);
        }
#endif

        /* validate each of the partition entries */
        for (i=0; i < 4; i++) {
            if (validate_mbr_partition(dev, &part[i]) >= 0) {
                // publish it
                char subdevice[128];

                sprintf(subdevice, "%sp%d", device, i);

                err = bio_publish_subdevice(device, subdevice, part[i].lba_start, part[i].lba_length);
                if (err < 0) {
                    dprintf(INFO, "error publishing subdevice '%s'\n", subdevice);
                    continue;
                }
                count++;
            }
        }
    } while (0);

    bio_close(dev);

err:
    return (err < 0) ? err : count;
}

int partition_unpublish(const char *device) {
    int i;
    int count;
    bdev_t *dev;
    char devname[512];

    count = 0;
    for (i=0; i < 16; i++) {
        sprintf(devname, "%sp%d", device, i);

        dev = bio_open(devname);
        if (!dev)
            continue;

        bio_unregister_device(dev);
        bio_close(dev);
        count++;
    }

    return count;
}

