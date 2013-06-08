/*
 * Copyright (c) 2009 Travis Geiselbrecht
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
#include <debug.h>
#include <stdlib.h>
#include <string.h>
#include <lib/console.h>
#include <lib/bio.h>
#include <lib/partition.h>
#include <platform.h>

#if defined(WITH_LIB_CONSOLE)

#if LK_DEBUGLEVEL > 0
static int cmd_bio(int argc, const cmd_args *argv);

STATIC_COMMAND_START
STATIC_COMMAND("bio", "block io debug commands", &cmd_bio)
STATIC_COMMAND_END(bio);

static int cmd_bio(int argc, const cmd_args *argv)
{
	int rc = 0;

	if (argc < 2) {
		printf("not enough arguments:\n");
usage:
		printf("%s list\n", argv[0].str);
		printf("%s read <device> <address> <offset> <len>\n", argv[0].str);
		printf("%s write <device> <address> <offset> <len>\n", argv[0].str);
		printf("%s erase <device> <offset> <len>\n", argv[0].str);
		printf("%s ioctl <device> <request> <arg>\n", argv[0].str);
		printf("%s remove <device>\n", argv[0].str);
#if WITH_LIB_PARTITION
		printf("%s partscan <device> [offset]\n", argv[0].str);
#endif
		return -1;
	}

	if (!strcmp(argv[1].str, "list")) {
		bio_dump_devices();
	} else if (!strcmp(argv[1].str, "read")) {
		if (argc < 6) {
			printf("not enough arguments:\n");
			goto usage;
		}

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
		if (argc < 6) {
			printf("not enough arguments:\n");
			goto usage;
		}

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
	} else if (!strcmp(argv[1].str, "erase")) {
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

		lk_time_t t = current_time();
		ssize_t err = bio_erase(dev, offset, len);
		t = current_time() - t;
		dprintf(INFO, "bio_erase returns %d, took %u msecs (%d bytes/sec)\n", (int)err, (uint)t, (uint32_t)((uint64_t)err * 1000 / t));

		bio_close(dev);

		rc = err;
	} else if (!strcmp(argv[1].str, "ioctl")) {
		if (argc < 4) {
			printf("not enough arguments:\n");
			goto usage;
		}

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
		if (argc < 3) {
			printf("not enough arguments:\n");
			goto usage;
		}

		bdev_t *dev = bio_open(argv[2].str);
		if (!dev) {
			printf("error opening block device\n");
			return -1;
		}

		bio_unregister_device(dev);
		bio_close(dev);
#if WITH_LIB_PARTITION
	} else if (!strcmp(argv[1].str, "partscan")) {
		if (argc < 3) {
			printf("not enough arguments:\n");
			goto usage;
		}

		off_t offset = 0;
		if (argc > 3)
			offset = argv[3].u;

		rc = partition_publish(argv[2].str, offset);
		dprintf(INFO, "partition_publish returns %d\n", rc);
#endif
	} else {
		printf("unrecognized subcommand\n");
		goto usage;
	}

	return rc;
}

#endif

#endif

