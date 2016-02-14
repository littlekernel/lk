// Copyright (C) 2015 Playground Global LLC.  All rights reserved.

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
#include <kernel/thread.h>
#include <stm32f4xx_flash.h>

#define LOCAL_TRACE 0

#define SECTORS 12

static u32 sectors[SECTORS + 1] = {
    0x08000000,
    0x08004000,
    0x08008000,
    0x0800C000,
    0x08010000,
    0x08020000,
    0x08040000,
    0x08060000,
    0x08080000,
    0x080A0000,
    0x080C0000,
    0x080E0000,
    0x08100000,
};


typedef struct intflash_s {
    bool initialized;
    bdev_t bdev;
    uint32_t start;
} intflash_t;

static intflash_t sg_flash = { 0 };
static ssize_t stmflash_bdev_read(struct bdev *, void *buf, off_t offset, size_t len);
static ssize_t stmflash_bdev_read_block(struct bdev *, void *buf, bnum_t block, uint count);
static ssize_t stmflash_bdev_write(struct bdev *, const void *buf, off_t offset, size_t len);
static ssize_t stmflash_bdev_write_block(struct bdev *, const void *buf, bnum_t block, uint count);
static ssize_t stmflash_bdev_erase(struct bdev *, off_t offset, size_t len);
static int stmflash_ioctl(struct bdev *, int request, void *argp);

status_t stmflash_init(uint32_t start, uint32_t length)
{
    memset(&sg_flash, 0, sizeof(intflash_t));
    sg_flash.start  = start;
    /* construct the block device */
    bio_initialize_bdev(&sg_flash.bdev,
                        "flash0",
                        1,
                        length,
                        0,
                        NULL,
                        BIO_FLAGS_NONE);

    /* override our block device hooks */
    sg_flash.bdev.read        = &stmflash_bdev_read;
    sg_flash.bdev.read_block  = &stmflash_bdev_read_block;
    sg_flash.bdev.write       = &stmflash_bdev_write;
    sg_flash.bdev.write_block = &stmflash_bdev_write_block;
    sg_flash.bdev.erase       = &stmflash_bdev_erase;
    sg_flash.bdev.ioctl       = &stmflash_ioctl;
    bio_register_device(&sg_flash.bdev);
    sg_flash.initialized = true;
    return NO_ERROR;
}

// bio layer hooks
static ssize_t stmflash_bdev_read(struct bdev *bdev, void *buf, off_t offset, size_t len)
{
    uint32_t startAddress = sg_flash.start;
    LTRACEF("dev %p, buf %p, offset 0x%llx, len 0x%zx\n", bdev, buf, offset, len);
    len = bio_trim_range(bdev, offset, len);
    if (0 == len) {
        return 0;
    }
    startAddress += offset;
    memcpy(buf, (uint32_t *)(startAddress), len);
    return len;
}

static ssize_t stmflash_bdev_read_block(struct bdev *bdev, void *buf, bnum_t block, uint count)
{
    LTRACEF("dev %p, buf %p, block 0x%x, count %u\n",bdev, buf, block, count);
    return 0;
}

static ssize_t stmflash_bdev_write(struct bdev *bdev, const void *buf, off_t offset, size_t len)
{
    uint32_t i, start_address;
    LTRACEF("dev %p, buf %p, offset 0x%llx, len 0x%zx\n",bdev, buf, offset, len);
    len = bio_trim_range(bdev, offset, len);
    if (0 == len) {
        return 0;
    }
    start_address = sg_flash.start+offset;
    FLASH_Unlock();
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
                    FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);
    const uint32_t *_buf = buf;
    for (i = 0; i < len / 4; i++) {
        if (FLASH_COMPLETE == FLASH_ProgramWord(start_address,_buf[i])) {
            start_address += 4;
        } else {
            len = 0;
            break;
        }
    }
    FLASH_Lock();
    return len;
}

static ssize_t stmflash_bdev_write_block(struct bdev *bdev, const void *_buf, bnum_t block, uint count)
{
    LTRACEF("dev %p, buf %p, block 0x%x, count %u\n",bdev, _buf, block, count);
    count = bio_trim_block_range(bdev, block, count);
    return 0;
}

static ssize_t stmflash_bdev_erase(struct bdev *bdev, off_t offset, size_t len)
{
    uint8_t  n;
    LTRACEF("dev %p, offset 0x%llx, len 0x%zx\n",bdev, offset, len);
    len = bio_trim_range(bdev, offset, len);
    if (0 == len) {
        return 0;
    }
    for (n = 0; n < SECTORS; n++) {
        if (sectors[n] == sg_flash.start+offset) {
            break;
        }
    }
    if (SECTORS == n) {
        return 0;
    }
    FLASH_Unlock();
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
                    FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR|FLASH_FLAG_PGSERR);
    for (;;) {
        if (FLASH_EraseSector(n<<3, VoltageRange_3) != FLASH_COMPLETE) {
            FLASH_Lock();
            return 0;
        }
        n++;
        if (SECTORS == n) {
            break;
        }
        if ((sectors[n] - (sg_flash.start+offset)) >= len) {
            break;
        }
    }
    FLASH_Lock();
    return len;
}

static int stmflash_ioctl(struct bdev *bdev, int request, void *argp)
{
    LTRACEF("dev %p, request %d, argp %p\n",bdev, request, argp);
    return ERR_NOT_SUPPORTED;
}
