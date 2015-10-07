/*
 * Copyright (c) 2009-2014 Travis Geiselbrecht
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
#include <assert.h>
#include <debug.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lib/console.h>
#include <lib/bio.h>
#include <lib/partition.h>
#include <platform.h>

#if WITH_LIB_CKSUM
#include <lib/cksum.h>
#endif

#if defined(WITH_LIB_CONSOLE)

#if LK_DEBUGLEVEL > 0
static int cmd_bio(int argc, const cmd_args *argv);
static int bio_test_device(bdev_t* device);

STATIC_COMMAND_START
STATIC_COMMAND("bio", "block io debug commands", &cmd_bio)
STATIC_COMMAND_END(bio);

static int cmd_bio(int argc, const cmd_args *argv)
{
    int rc = 0;

    if (argc < 2) {
notenoughargs:
        printf("not enough arguments:\n");
usage:
        printf("%s list\n", argv[0].str);
        printf("%s read <device> <address> <offset> <len>\n", argv[0].str);
        printf("%s write <device> <address> <offset> <len>\n", argv[0].str);
        printf("%s dump <device> <offset> <len>\n", argv[0].str);
        printf("%s erase <device> <offset> <len>\n", argv[0].str);
        printf("%s ioctl <device> <request> <arg>\n", argv[0].str);
        printf("%s remove <device>\n", argv[0].str);
        printf("%s test <device>\n", argv[0].str);
#if WITH_LIB_PARTITION
        printf("%s partscan <device> [offset]\n", argv[0].str);
#endif
#if WITH_LIB_CKSUM
        printf("%s crc32 <device> <offset> <len> [repeat]\n", argv[0].str);
#endif
        return -1;
    }

    if (!strcmp(argv[1].str, "list")) {
        bio_dump_devices();
    } else if (!strcmp(argv[1].str, "read")) {
        if (argc < 6) goto notenoughargs;

        addr_t address = argv[3].u;
        off_t offset = argv[4].u; // XXX use long
        size_t len = argv[5].u;

        bdev_t *dev = bio_open(argv[2].str);
        if (!dev) {
            printf("error opening block device\n");
            return -1;
        }

        lk_time_t t = current_time();
        ssize_t err = bio_read(dev, (void *)address, offset, len);
        t = current_time() - t;
        dprintf(INFO, "bio_read returns %d, took %u msecs (%d bytes/sec)\n", (int)err, (uint)t, (uint32_t)((uint64_t)err * 1000 / t));

        bio_close(dev);

        rc = err;
    } else if (!strcmp(argv[1].str, "write")) {
        if (argc < 6) goto notenoughargs;

        addr_t address = argv[3].u;
        off_t offset = argv[4].u; // XXX use long
        size_t len = argv[5].u;

        bdev_t *dev = bio_open(argv[2].str);
        if (!dev) {
            printf("error opening block device\n");
            return -1;
        }

        lk_time_t t = current_time();
        ssize_t err = bio_write(dev, (void *)address, offset, len);
        t = current_time() - t;
        dprintf(INFO, "bio_write returns %d, took %u msecs (%d bytes/sec)\n", (int)err, (uint)t, (uint32_t)((uint64_t)err * 1000 / t));

        bio_close(dev);

        rc = err;
    } else if (!strcmp(argv[1].str, "dump")) {
        if (argc < 5) {
            printf("not enough arguments:\n");
            goto usage;
        }

        off_t offset = argv[3].u; // XXX use long
        size_t len = argv[4].u;

        bdev_t *dev = bio_open(argv[2].str);
        if (!dev) {
            printf("error opening block device\n");
            return -1;
        }

        uint8_t buf[256];
        ssize_t err = 0;
        while (len > 0) {
            size_t  amt = MIN(sizeof(buf), len);
            ssize_t err = bio_read(dev, buf, offset, amt);

            if (err < 0) {
                dprintf(ALWAYS, "read error %s %zu@%zu (err %d)\n",
                        argv[2].str, amt, (size_t)offset, (int)err);
                break;
            }

            DEBUG_ASSERT((size_t)err <= amt);
            hexdump8_ex(buf, err, offset);

            if ((size_t)err != amt) {
                dprintf(ALWAYS, "short read from %s @%zu (wanted %zu, got %zu)\n",
                        argv[2].str, (size_t)offset, amt, (size_t)err);
                break;
            }

            offset += amt;
            len    -= amt;
        }

        bio_close(dev);
        rc = err;
    } else if (!strcmp(argv[1].str, "erase")) {
        if (argc < 5) goto notenoughargs;

        off_t offset = argv[3].u; // XXX use long
        size_t len = argv[4].u;

        bdev_t *dev = bio_open(argv[2].str);
        if (!dev) {
            printf("error opening block device\n");
            return -1;
        }

        lk_time_t t = current_time();
        ssize_t err = bio_erase(dev, offset, len);
        t = current_time() - t;
        dprintf(INFO, "bio_erase returns %d, took %u msecs (%d bytes/sec)\n", (int)err, (uint)t, (uint32_t)((uint64_t)err * 1000 / t));

        bio_close(dev);

        rc = err;
    } else if (!strcmp(argv[1].str, "ioctl")) {
        if (argc < 4) goto notenoughargs;

        int request = argv[3].u;
        int arg = (argc == 5) ? argv[4].u : 0;

        bdev_t *dev = bio_open(argv[2].str);
        if (!dev) {
            printf("error opening block device\n");
            return -1;
        }

        int err = bio_ioctl(dev, request, (void *)arg);
        dprintf(INFO, "bio_ioctl returns %d\n", err);

        bio_close(dev);

        rc = err;
    } else if (!strcmp(argv[1].str, "remove")) {
        if (argc < 3) goto notenoughargs;

        bdev_t *dev = bio_open(argv[2].str);
        if (!dev) {
            printf("error opening block device\n");
            return -1;
        }

        bio_unregister_device(dev);
        bio_close(dev);
    } else if (!strcmp(argv[1].str, "test")) {
        if (argc < 3) goto notenoughargs;

        bdev_t *dev = bio_open(argv[2].str);
        if (!dev) {
            printf("error opening block device\n");
            return -1;
        }

        int err = bio_test_device(dev);
        bio_close(dev);

        rc = err;
#if WITH_LIB_PARTITION
    } else if (!strcmp(argv[1].str, "partscan")) {
        if (argc < 3) goto notenoughargs;

        off_t offset = 0;
        if (argc > 3)
            offset = argv[3].u;

        rc = partition_publish(argv[2].str, offset);
        dprintf(INFO, "partition_publish returns %d\n", rc);
#endif
#if WITH_LIB_CKSUM
    } else if (!strcmp(argv[1].str, "crc32")) {
        if (argc < 5) goto notenoughargs;

        off_t offset = argv[3].u; // XXX use long
        size_t len = argv[4].u;

        bdev_t *dev = bio_open(argv[2].str);
        if (!dev) {
            printf("error opening block device\n");
            return -1;
        }

        void *buf = malloc(dev->block_size);

        bool repeat = false;
        if (argc >= 6 && !strcmp(argv[5].str, "repeat")) {
            repeat = true;
        }

        do {
            ulong crc = 0;
            off_t pos = offset;
            while (pos < offset + len) {
                ssize_t err = bio_read(dev, buf, pos, MIN(len - (pos - offset), dev->block_size));

                if (err <= 0) {
                    printf("error reading at offset 0x%llx\n", offset + pos);
                    break;
                }

                crc = crc32(crc, buf, err);

                pos += err;
            }

            printf("crc 0x%08lx\n", crc);
        } while (repeat);

        bio_close(dev);
        free(buf);

#endif
    } else {
        printf("unrecognized subcommand\n");
        goto usage;
    }

    return rc;
}

#endif

#endif

// Returns the number of blocks that do not match the reference pattern.
static bool is_valid_block(bdev_t *device, bnum_t block_num, uint8_t* pattern,
                           size_t pattern_length)
{
    uint8_t *block_contents = malloc(device->block_size);
    device->read_block(device, block_contents, block_num, 1);
    for (size_t i = 0; i < device->block_size; i++) {
        if (block_contents[i] != pattern[i % pattern_length]) {
            free(block_contents);
            block_contents = NULL;
            return false;
        }
    }

    free(block_contents);
    block_contents = NULL;

    return true;
}

static size_t erase_test(bdev_t *device)
{
    printf("erasing flash...\n");
    uint8_t empty_flash[1] = { 0xff };

    bio_erase(device, 0, device->total_size);

    printf("validating erase...\n");
    size_t num_invalid_blocks = 0;
    for (bnum_t bnum = 0; bnum < device->block_count; bnum++) {
        if (!is_valid_block(device, bnum, empty_flash, sizeof(empty_flash))) {
            num_invalid_blocks++;
        }
    }
    return num_invalid_blocks;
}

// returns the number of blocks where the write was not successful.
static size_t write_test(bdev_t *device)
{
    uint8_t *test_buffer = malloc(device->block_size);

    for (bnum_t bnum = 0; bnum < device->block_count; bnum++) {
        memset(test_buffer, (uint8_t)bnum, device->block_size);
        bio_write_block(device, test_buffer, bnum, 1);
    }

    size_t num_errors = 0;
    uint8_t expected_pattern[1];
    for (bnum_t bnum = 0; bnum < device->block_count; bnum++) {
        expected_pattern[0] = (uint8_t)bnum;
        if (!is_valid_block(device, bnum, expected_pattern, sizeof(expected_pattern))) {
            num_errors++;
        }
    }

    free(test_buffer);
    test_buffer = NULL;

    return num_errors;
}

static int bio_test_device(bdev_t* device)
{
    size_t num_errors = erase_test(device);
    printf("discovered %u error(s) while testing erase.\n", num_errors);
    if (num_errors) {
        // No point in continuing the tests if we couldn't erase the device.
        printf("not continuing to test writes.\n");
        return -1;
    }

    num_errors = write_test(device);
    printf("Discovered %u error(s) while testing write.\n", num_errors);
    if (num_errors) {
        return -1;
    }

    return 0;
}

// vim: set ts=4 sw=4 noexpandtab:

