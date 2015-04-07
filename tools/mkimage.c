/*
 * Copyright (c) 2014 Brian Swetland
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "bootimage.h"

static const char *outname = "boot.img";

static struct {
	const char *cmd;
	unsigned kind;
	unsigned type;
} types[] = {
	{ "lk", KIND_FILE, TYPE_LK },
	{ "fpga", KIND_FILE, TYPE_FPGA_IMAGE },
	{ "linux", KIND_FILE, TYPE_LINUX_KERNEL },
	{ "initrd", KIND_FILE, TYPE_LINUX_INITRD },
	{ "devicetree", KIND_FILE, TYPE_DEVICE_TREE },
	{ "sysparams", KIND_FILE, TYPE_SYSPARAMS },
	{ "board", KIND_BOARD, 0 },
	{ "build", KIND_BUILD, 0 },
	{ NULL, 0 },
};

void usage(const char *binary) {
	unsigned n;
	fprintf(stderr, "usage:\n");
	fprintf(stderr, "%s [-h] [-o <output file] section:file ...\n\n", binary);

	fprintf(stderr, "Supported section types:\n");
	for (n = 0; types[n].cmd != NULL; n++) {
		if (types[n].kind == KIND_FILE) {
			fprintf(stderr, "\t%s\n", types[n].cmd);
		}
	}

	fprintf(stderr, "\nSupported string types:\n");
	for (n = 0; types[n].cmd != NULL; n++) {
		if (types[n].kind != KIND_FILE) {
			fprintf(stderr, "\t%s\n", types[n].cmd);
		}
	}
}

int process(bootimage *img, char *cmd, char *arg) {
	unsigned n;

	for (n = 0; types[n].cmd != NULL; n++) {
		if (strcmp(cmd, types[n].cmd)) {
			continue;
		}
		if (types[n].kind == KIND_FILE) {
			if (bootimage_add_file(img, types[n].type, arg) == NULL) {
				return -1;
			}
		} else {
			if (bootimage_add_string(img, types[n].kind, arg) == NULL) {
				return -1;
			}
		}
		return 0;
	}

	fprintf(stderr, "unknown command '%s'\n", cmd);
	return -1;
}

int main(int argc, char **argv) {
	const char *binary = argv[0];
	bootimage *img;
	int fd;
	int count = 0;

	img = bootimage_init();

	while (argc > 1) {
		char *cmd = argv[1];
		char *arg = strchr(cmd, ':');
		argc--;
		argv++;

		if (!strcmp(cmd, "-h") || !strcmp(cmd, "--help")) {
			usage(binary);
			return 1;
		} else if (!strcmp(cmd, "-o")) {
			outname = argv[1];
			argc--;
			argv++;
		} else {
			if (arg == NULL) {
				fprintf(stderr, "error: invalid argument '%s'\n", cmd);
				return 1;
			}

			*arg++ = 0;

			if (process(img, cmd, arg)) {
				return 1;
			}
			count++;
		}
	}

	if (count == 0) {
		fprintf(stderr, "no sections to process\n");
		return 1;
	}

	bootimage_done(img);

	if ((fd = open(outname, O_CREAT|O_TRUNC|O_WRONLY, 0644)) < 0) {
		fprintf(stderr, "error: cannot open '%s' for writing\n", outname);
		return 1;
	}
	if (bootimage_write(img, fd)) {
		fprintf(stderr, "error: failed to write '%s'\n", outname);
		unlink(outname);
		return 1;
	}
	close(fd);
	return 0;
}

// vim: set noexpandtab:
