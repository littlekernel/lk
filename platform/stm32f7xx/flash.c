/*
 * Copyright (c) 2015 Travis Geiselbrecht
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
#include <string.h>
#include <stdlib.h>
#include <trace.h>
#include <pow2.h>
#include <arch/ops.h>
#include <lib/bio.h>
#include <platform/stm32.h>

#define LOCAL_TRACE 0

/* device parameters */
#define MAX_GEOMETRY_COUNT 3
#define PROGRAM_SIZE 4

#define _32K  (32 * 1024U)
#define _128K (128 * 1024U)
#define _256K (256 * 1024U)

#define ERASE_RANGE0_START  (0)
#define ERASE_RANGE0_END    (_32K * 4)
#define ERASE_RANGE1_START  ERASE_RANGE0_END
#define ERASE_RANGE1_END    (ERASE_RANGE1_START + _128K)
#define ERASE_RANGE2_START  ERASE_RANGE1_END
#define ERASE_RANGE2_END    (flash.size)

struct stm32_flash {
    bdev_t bdev;
    off_t size;

    bio_erase_geometry_info_t geometry[MAX_GEOMETRY_COUNT];
} flash;

static ssize_t stm32_flash_bdev_read(struct bdev *, void *buf, off_t offset, size_t len);
static ssize_t stm32_flash_bdev_read_block(struct bdev *, void *buf, bnum_t block, uint count);
static ssize_t stm32_flash_bdev_write(struct bdev *bdev, const void *buf, off_t offset, size_t len);
static ssize_t stm32_flash_bdev_write_block(struct bdev *, const void *buf, bnum_t block, uint count);
static ssize_t stm32_flash_bdev_erase(struct bdev *, off_t offset, size_t len);
static int stm32_flash_ioctl(struct bdev *, int request, void *argp);

void stm32_flash_early_init(void)
{
    /* Enable FLASH clock  */
    __HAL_RCC_ETH_CLK_ENABLE();
}

void stm32_flash_init(void)
{
    // XXX detect here
    flash.size = 1024*1024;

    flash.geometry[0].start = ERASE_RANGE0_START;
    flash.geometry[0].size = ERASE_RANGE0_END - ERASE_RANGE0_START;
    flash.geometry[0].erase_size = _32K;
    flash.geometry[0].erase_size = log2_uint(_32K);

    flash.geometry[1].start = ERASE_RANGE1_START;
    flash.geometry[1].size = ERASE_RANGE1_END - ERASE_RANGE1_START;
    flash.geometry[1].erase_size = _128K;
    flash.geometry[1].erase_size = log2_uint(_128K);

    flash.geometry[2].start = ERASE_RANGE2_START;
    flash.geometry[2].size = ERASE_RANGE2_END - ERASE_RANGE2_START;
    flash.geometry[2].erase_size = _256K;
    flash.geometry[2].erase_size = log2_uint(_256K);

    /* construct the block device */
    bio_initialize_bdev(&flash.bdev, "flash0",
                        PROGRAM_SIZE, flash.size / PROGRAM_SIZE,
                        3, flash.geometry, BIO_FLAGS_NONE);

    /* we erase to 0xff */
    flash.bdev.erase_byte = 0xff;

    /* override our block device hooks */
    flash.bdev.read = &stm32_flash_bdev_read;
    flash.bdev.read_block = &stm32_flash_bdev_read_block;
    //flash.bdev.write = &stm32_flash_bdev_write;
    flash.bdev.write_block = &stm32_flash_bdev_write_block;
    flash.bdev.erase = &stm32_flash_bdev_erase;
    flash.bdev.ioctl = &stm32_flash_ioctl;

    bio_register_device(&flash.bdev);
}

static ssize_t stm32_flash_bdev_read(struct bdev *bdev, void *buf, off_t offset, size_t len)
{
    LTRACEF("dev %p, buf %p, offset 0x%llx, len 0x%zx\n", bdev, buf, offset, len);

    memcpy(buf, (uint8_t *)FLASHAXI_BASE + offset, len);

    return len;
}

static ssize_t stm32_flash_bdev_read_block(struct bdev *bdev, void *buf, bnum_t block, uint count)
{
    LTRACEF("dev %p, buf %p, block 0x%x, count %u\n", bdev, buf, block, count);

    memcpy(buf, (uint8_t *)FLASHAXI_BASE + block * bdev->block_size, count * bdev->block_size);

    return count * bdev->block_size;
}

static ssize_t stm32_flash_bdev_write_block(struct bdev *bdev, const void *buf, bnum_t block, uint count)
{
    LTRACEF("dev %p, buf %p, block 0x%x, count %u\n", bdev, buf, block, count);

    HAL_FLASH_Unlock();

    ssize_t written_bytes = count * bdev->block_size;
    const uint32_t *buf32 = (const uint32_t *)buf;
    while (count > 0) {
        if (HAL_FLASH_Program(TYPEPROGRAM_WORD, FLASHAXI_BASE + block * bdev->block_size, *buf32) != HAL_OK) {
            written_bytes = ERR_IO;
            break;
        }

        buf32++;
        block++;
        count--;
    }

    HAL_FLASH_Lock();

    return written_bytes;
}

static status_t offset_to_sector(off_t offset, uint32_t *sector, off_t *sector_offset, off_t *next_offset)
{
    if (offset < 0) {
        return -1;
    } else if (offset < ERASE_RANGE0_END) {
        *sector = (offset - ERASE_RANGE0_START) / _32K;
        *sector_offset = ROUNDDOWN(offset - ERASE_RANGE0_START, _32K) + ERASE_RANGE0_START;
        *next_offset = *sector_offset + _32K;
    } else if (offset < ERASE_RANGE1_END) {
        *sector = (offset - ERASE_RANGE1_START) / _128K + 4;
        *sector_offset = ROUNDDOWN(offset - ERASE_RANGE1_START, _128K) + ERASE_RANGE1_START;
        *next_offset = *sector_offset + _128K;
    } else if (offset < ERASE_RANGE2_END) {
        *sector = (offset - ERASE_RANGE2_START) / _256K + 5;
        *sector_offset = ROUNDDOWN(offset - ERASE_RANGE2_START, _256K) + ERASE_RANGE2_START;
        *next_offset = *sector_offset + _256K;
    } else {
        return -1;
    }

    DEBUG_ASSERT(*sector < FLASH_SECTOR_TOTAL);

    LTRACEF("offset 0x%llx, sector %u, sector_offset 0x%llx, next_offset 0x%llx\n", offset, *sector, *sector_offset, *next_offset);

    return NO_ERROR;
}

static ssize_t stm32_flash_bdev_erase(struct bdev *bdev, off_t offset, size_t len)
{
    LTRACEF("dev %p, offset 0x%llx, len 0x%zx\n", bdev, offset, len);

    ssize_t total_erased = 0;

    HAL_FLASH_Unlock();

    while (len > 0) {
        uint32_t sector = 0;
        off_t sector_offset = 0;
        off_t next_offset = 0;

        if (offset_to_sector(offset, &sector, &sector_offset, &next_offset) < 0)
            return ERR_INVALID_ARGS;

        FLASH_EraseInitTypeDef erase;
        erase.TypeErase = FLASH_TYPEERASE_SECTORS;
        erase.Sector = sector;
        erase.NbSectors = 1;
        erase.VoltageRange = FLASH_VOLTAGE_RANGE_3; // XXX

        LTRACEF("erase params: sector %u, num_sectors %u, next_offset 0x%llx\n", erase.Sector, erase.NbSectors, next_offset);

        if (1) {
            uint32_t sector_error;
            HAL_StatusTypeDef err = HAL_FLASHEx_Erase(&erase, &sector_error);
            if (err != HAL_OK) {
                TRACEF("error starting erase operation, sector error %u\n", sector_error);
                total_erased = ERR_IO;
                break;
            }

            err = FLASH_WaitForLastOperation(HAL_MAX_DELAY);
            if (err != HAL_OK) {
                TRACEF("error waiting for erase operation to end, hal error %u\n", HAL_FLASH_GetError());
                total_erased = ERR_IO;
                break;
            }

            // invalidate the cache on this region
            arch_invalidate_cache_range(FLASHAXI_BASE + sector_offset, next_offset - sector_offset);
        }

        // move to the next erase boundary
        total_erased += next_offset - sector_offset;
        off_t erased_bytes = next_offset - offset;
        if (erased_bytes >= len)
            break;
        len -= erased_bytes;
        offset = next_offset;
    }

    HAL_FLASH_Lock();

    return total_erased;
}

static int stm32_flash_ioctl(struct bdev *bdev, int request, void *argp)
{
    LTRACEF("dev %p, request %d, argp %p\n", bdev, request, argp);

    int ret = ERR_NOT_SUPPORTED;
    switch (request) {
        case BIO_IOCTL_GET_MAP_ADDR:
        case BIO_IOCTL_GET_MEM_MAP:
            /* we're already mapped */
            if (argp)
                *(void **)argp = (void *)FLASHAXI_BASE;
            break;
        case BIO_IOCTL_PUT_MEM_MAP:
            break;
    }

    return ret;
}

