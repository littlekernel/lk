/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

// Checks block devices stamped by scripts/mkpattern-image.py, where every 512 byte
// block names itself. Any registered device whose block 0 carries the stamp is read at
// addresses chosen to straddle the usual track, head and transfer size boundaries, and
// every block that comes back must be the one asked for. On a writable device a few
// blocks are rewritten, read back and restored. With no stamped device the test passes
// without doing anything, so it is harmless in a normal boot test run.

#include <lib/bio.h>
#include <lib/unittest.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PATTERN_BLOCK     512
#define PATTERN_MAX_DEVS  8
#define PATTERN_RUN       300 // more than one 256 sector ATA command

static const char pattern_magic[8] = { 'L', 'K', 'P', 'A', 'T', 'T', 'R', 'N' };

static uint32_t get_le32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

// expected contents of block; inverted on blocks this test has rewritten
static void pattern_fill(uint8_t *buf, uint32_t block, uint32_t total, bool inverted) {
    memcpy(buf, pattern_magic, sizeof(pattern_magic));
    for (int i = 0; i < 4; i++) {
        buf[8 + i] = (uint8_t)(block >> (i * 8));
        buf[12 + i] = (uint8_t)(total >> (i * 8));
    }
    for (uint i = 16; i < PATTERN_BLOCK; i++) {
        const uint8_t v = (uint8_t)(block * 7 + i);
        buf[i] = inverted ? (uint8_t)~v : v;
    }
}

// true if buf holds the expected block, else describes the mismatch
static bool pattern_matches(const uint8_t *buf, uint32_t block, uint32_t total, bool inverted,
                            uint8_t *scratch) {
    pattern_fill(scratch, block, total, inverted);
    if (memcmp(buf, scratch, PATTERN_BLOCK) == 0) {
        return true;
    }
    if (memcmp(buf, pattern_magic, sizeof(pattern_magic)) == 0) {
        unittest_printf("block %u read back as block %u\n", block, get_le32(buf + 8));
    } else {
        unittest_printf("block %u has no pattern stamp\n", block);
    }
    return false;
}

struct pattern_devs {
    uint count;
    char names[PATTERN_MAX_DEVS][32];
};

static bool collect_device(void *cookie, bdev_t *dev) {
    struct pattern_devs *devs = cookie;
    if (dev->block_size == PATTERN_BLOCK && devs->count < PATTERN_MAX_DEVS) {
        strlcpy(devs->names[devs->count++], dev->name, sizeof(devs->names[0]));
    }
    return true;
}

// image_total is the image's own block count from the stamp; the device may expose
// fewer (a CHS disk rounds down to whole cylinders)
static bool read_check(bdev_t *dev, uint8_t *buf, uint8_t *scratch, uint32_t image_total,
                       uint32_t block, uint count, bool inverted) {
    const uint32_t total = dev->block_count;
    if (block >= total) {
        return true;
    }
    count = MIN(count, total - block);
    if (bio_read_block(dev, buf, block, count) != (ssize_t)(count * PATTERN_BLOCK)) {
        unittest_printf("%s: read of %u blocks at %u failed\n", dev->name, count, block);
        return false;
    }
    for (uint i = 0; i < count; i++) {
        if (!pattern_matches(buf + i * PATTERN_BLOCK, block + i, image_total, inverted, scratch)) {
            unittest_printf("%s: in a read of %u blocks at %u\n", dev->name, count, block);
            return false;
        }
    }
    return true;
}

static bool check_device(bdev_t *dev, uint8_t *buf, uint8_t *scratch, uint32_t image_total) {
    BEGIN_TEST;

    const uint32_t total = dev->block_count;
    unittest_printf("bio_pattern: checking %s, %u of the image's %u blocks\n", dev->name, total,
                    image_total);

    // Single blocks around the boundaries of floppy (9, 15, 18 sectors per track, 2
    // heads) and disk (17 or 63 sectors, 16 heads) geometries, the 8 bit sector count
    // and the 28 bit LBA's top nibble, then the end of the device.
    static const uint32_t probes[] = {
        0, 1, 8, 9, 14, 15, 16, 17, 18, 19, 35, 36, 37, 62, 63, 64, 126, 127, 255, 256, 257,
        271, 272, 1007, 1008, 1009, 16127, 16128, 16129, 65535, 65536,
    };
    for (uint i = 0; i < countof(probes); i++) {
        EXPECT_TRUE(read_check(dev, buf, scratch, image_total, probes[i], 1, false), "");
    }
    EXPECT_TRUE(read_check(dev, buf, scratch, image_total, total - 1, 1, false), "");

    // multi block reads that cross a track and a command boundary
    EXPECT_TRUE(read_check(dev, buf, scratch, image_total, 0, PATTERN_RUN, false), "");
    EXPECT_TRUE(read_check(dev, buf, scratch, image_total, 250, PATTERN_RUN, false), "");
    if (total > PATTERN_RUN) {
        EXPECT_TRUE(read_check(dev, buf, scratch, image_total, total - PATTERN_RUN, PATTERN_RUN, false), "");
    }

    // rewrite a run straddling a 256 block command boundary, then put it back
    const uint32_t wblock = (total > 300) ? 250 : 0;
    const uint wcount = MIN(12u, total - wblock);
    for (uint i = 0; i < wcount; i++) {
        pattern_fill(buf + i * PATTERN_BLOCK, wblock + i, image_total, true);
    }
    ssize_t err = bio_write_block(dev, buf, wblock, wcount);
    if (err == ERR_NOT_SUPPORTED) {
        unittest_printf("bio_pattern: %s is read only, skipping the write check\n", dev->name);
        EXPECT_TRUE(read_check(dev, buf, scratch, image_total, wblock, wcount, false), "");
    } else {
        EXPECT_EQ((ssize_t)(wcount * PATTERN_BLOCK), err, "write");
        EXPECT_TRUE(read_check(dev, buf, scratch, image_total, wblock, wcount, true), "");
        for (uint i = 0; i < wcount; i++) {
            pattern_fill(buf + i * PATTERN_BLOCK, wblock + i, image_total, false);
        }
        EXPECT_EQ((ssize_t)(wcount * PATTERN_BLOCK), bio_write_block(dev, buf, wblock, wcount),
                  "restore");
        EXPECT_TRUE(read_check(dev, buf, scratch, image_total, wblock - (wblock ? 1 : 0), wcount + 2, false), "");
    }

    END_TEST;
}

static bool pattern_devices(void) {
    BEGIN_TEST;

    // only the small buffers until a stamped device turns up, so boards with little RAM
    // and no such device are unaffected
    struct pattern_devs *devs = calloc(1, sizeof(*devs));
    uint8_t *scratch = malloc(PATTERN_BLOCK);
    uint8_t *buf = NULL;
    ASSERT_NONNULL(devs, "");
    ASSERT_NONNULL(scratch, "");

    // names first: no i/o is allowed while bio's device list is held
    bio_iter_devices(&collect_device, devs);

    uint checked = 0;
    for (uint i = 0; i < devs->count; i++) {
        bdev_t *dev = bio_open(devs->names[i]);
        if (!dev) {
            continue;
        }
        // any stamp counts, even one naming the wrong block: that is a failure to report,
        // not a device to skip
        if (bio_read_block(dev, scratch, 0, 1) == PATTERN_BLOCK &&
            memcmp(scratch, pattern_magic, sizeof(pattern_magic)) == 0 &&
            get_le32(scratch + 12) >= dev->block_count) {
            const uint32_t image_total = get_le32(scratch + 12);
            if (!buf) {
                buf = malloc(PATTERN_RUN * PATTERN_BLOCK);
            }
            EXPECT_NONNULL(buf, "");
            if (buf) {
                EXPECT_TRUE(check_device(dev, buf, scratch, image_total), dev->name);
            }
            checked++;
        }
        bio_close(dev);
    }
    if (checked == 0) {
        unittest_printf("bio_pattern: no pattern stamped device, nothing to check\n");
    }

    free(buf);
    free(scratch);
    free(devs);
    END_TEST;
}

BEGIN_TEST_CASE(bio_pattern)
RUN_TEST(pattern_devices)
END_TEST_CASE(bio_pattern)
