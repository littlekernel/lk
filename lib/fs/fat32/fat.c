/*
 * Copyright (c) 2015 Steve White
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

#include <err.h>
#include <lib/bio.h>
#include <lib/fs.h>
#include <trace.h>
#include <debug.h>
#include <malloc.h>
#include <string.h>
#include <endian.h>

#include "fat32_priv.h"
#include "fat_fs.h"

void fat32_dump(fat_fs_t *fat)
{
    printf("bytes_per_sector=%i\n", fat->bytes_per_sector);
    printf("sectors_per_cluster=%i\n", fat->sectors_per_cluster);
    printf("bytes_per_cluster=%i\n", fat->bytes_per_cluster);
    printf("reserved_sectors=%i\n", fat->reserved_sectors);
    printf("fat_bits=%i\n", fat->fat_bits);
    printf("fat_count=%i\n", fat->fat_count);
    printf("sectors_per_fat=%i\n", fat->sectors_per_fat);
    printf("total_sectors=%i\n", fat->total_sectors);
    printf("active_fat=%i\n", fat->active_fat);
    printf("data_start=%i\n", fat->data_start);
    printf("total_clusters=%i\n", fat->total_clusters);
    printf("root_cluster=%i\n", fat->root_cluster);
    printf("root_entries=%i\n", fat->root_entries);
    printf("root_start=%i\n", fat->root_start);
}

status_t fat32_mount(bdev_t *dev, fscookie **cookie)
{
    status_t result = NO_ERROR;

    if (!dev)
        return ERR_NOT_VALID;

    uint8_t *bs = malloc(512);
    int err = bio_read(dev, bs, 1024, 512);
    if (err < 0) {
        result = ERR_GENERIC;
        goto end;
    }

    if (((bs[0x1fe] != 0x55) || (bs[0x1ff] != 0xaa)) && (bs[0x15] == 0xf8)) {
        printf("missing boot signature\n");
        result = ERR_NOT_VALID;
        goto end;
    }

    fat_fs_t *fat = malloc(sizeof(fat_fs_t));
    fat->lba_start = 1024;
    fat->dev = dev;

    fat->bytes_per_sector = fat_read16(bs,0xb);
    if ((fat->bytes_per_sector != 0x200) && (fat->bytes_per_sector != 0x400) && (fat->bytes_per_sector != 0x800)) {
        printf("unsupported sector size (%x)\n", fat->bytes_per_sector);
        result = ERR_NOT_VALID;
        goto end;
    }

    fat->sectors_per_cluster = bs[0xd];
    switch (fat->sectors_per_cluster) {
        case 1:
        case 2:
        case 4:
        case 8:
        case 0x10:
        case 0x20:
        case 0x40:
        case 0x80:
            break;
        default:
            printf("unsupported sectors/cluster (%x)\n", fat->sectors_per_cluster);
            result = ERR_NOT_VALID;
            goto end;
    }

    fat->reserved_sectors = fat_read16(bs, 0xe);
    fat->fat_count = bs[0x10];

    if ((fat->fat_count == 0) || (fat->fat_count > 8)) {
        printf("unreasonable FAT count (%x)\n", fat->fat_count);
        result = ERR_NOT_VALID;
        goto end;
    }

    if (bs[0x15] != 0xf8) {
        printf("unsupported media descriptor byte (%x)\n", bs[0x15]);
        result = ERR_NOT_VALID;
        goto end;
    }

    fat->sectors_per_fat = fat_read16(bs, 0x16);
    if (fat->sectors_per_fat == 0) {
        fat->fat_bits = 32;
        fat->sectors_per_fat = fat_read32(bs,0x24);
        fat->total_sectors = fat_read32(bs,0x20);
        fat->active_fat = (bs[0x28] & 0x80) ? 0 : (bs[0x28] & 0xf);
        fat->data_start = fat->reserved_sectors + (fat->fat_count * fat->sectors_per_fat);
        fat->total_clusters = (fat->total_sectors - fat->data_start) / fat->sectors_per_cluster;

        // In FAT32, root directory appears in data area on given cluster and can be a cluster chain.
        fat->root_cluster = fat_read32(bs,0x2c);
        fat->root_start = 0;
        if (fat->root_cluster >= fat->total_clusters) {
            printf("root cluster too large (%x > %x)\n", fat->root_cluster, fat->total_clusters);
            result = ERR_NOT_VALID;
            goto end;
        }
        fat->root_entries = 0;
    } else {
        if (fat->fat_count != 2) {
            printf("illegal FAT count (%x)\n", fat->fat_count);
            result = ERR_NOT_VALID;
            goto end;
        }

        // On a FAT 12 or FAT 16 volumes the root directory is at a fixed position immediately after the File Allocation Tables
        fat->root_cluster = 0;
        fat->root_entries = fat_read16(bs,0x11);
        if (fat->root_entries % (fat->bytes_per_sector / 0x20)) {
            printf("illegal number of root entries (%x)\n", fat->root_entries);
            result = ERR_NOT_VALID;
            goto end;
        }

        fat->total_sectors = fat_read16(bs,0x13);
        if (fat->total_sectors == 0) {
            fat->total_sectors = fat_read32(bs,0x20);
        }

        fat->root_start = fat->reserved_sectors + fat->fat_count * fat->sectors_per_fat;
        fat->data_start = fat->root_start + fat->root_entries * 0x20 / fat->bytes_per_sector;
        fat->total_clusters = (fat->total_sectors - fat->data_start) / fat->sectors_per_cluster;

        if (fat->total_clusters < 0xff2) {
            printf("small FAT12, not supported\n");
            result = ERR_NOT_VALID;
            goto end;
        }
        fat->fat_bits = 16;
    }

    fat->bytes_per_cluster = fat->sectors_per_cluster * fat->bytes_per_sector;
    fat->cache = bcache_create(fat->dev, fat->bytes_per_sector, 4);

    *cookie = (fscookie *)fat;
end:
    free(bs);
    return result;
}

status_t fat32_unmount(fscookie *cookie)
{
    fat_fs_t *fat = (fat_fs_t *)cookie;
    bcache_destroy(fat->cache);
    free(fat);
    return NO_ERROR;
}

static const struct fs_api fat32_api = {
    .mount = fat32_mount,
    .unmount = fat32_unmount,
    .open = fat32_open_file,
    .stat = fat32_stat_file,
    .read = fat32_read_file,
    .close = fat32_close_file,
};

STATIC_FS_IMPL(fat32, &fat32_api);
