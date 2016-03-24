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

#include "devicetree.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

void error(const char *msg) {
	printf("error: %s\n", msg);
};

void indent(u32 n) {
	while (n-- > 0)
		putchar(' ');
}

void hexdump(u8 *data, u32 count) {
	while (count-- > 0)
		printf("%02x ", *data++);
}

int loadfile(const char *fn, dt_slice_t *out) {
	void *data = NULL;
	off_t end;	
	int fd;
	if ((fd = open(fn, O_RDONLY)) < 0)
		return -1;
	if ((end = lseek(fd, 0, SEEK_END)) == -1)
		goto oops;
	if (lseek(fd, 0, SEEK_SET))
		goto oops;
	if ((data = malloc(end + 1)) == NULL)
		goto oops;
	if ((read(fd, data, end) != end))
		goto oops;
	out->data = data;
	out->size = end;
	close(fd);
	return 0;
oops:
	free(data);
	close(fd);
	return -1;
}

static int _depth = 0;

int node_cb(int depth, const char *name, void *cookie) {
	_depth = depth;
	indent(depth*2);
	printf("node: '%s'\n", name);
	return 0;
}

int prop_cb(const char *name, u8 *data, u32 size, void *cookie) {
	indent(_depth * 2 + 2);
	printf("prop '%s' sz=%d\n", name, size);
	indent(_depth * 2 + 2);
	printf("data ");
	hexdump(data, size);
	printf("\n");
	return 0;
}

int main(int argc, char **argv) {
	devicetree_t dt;
	dt_slice_t s;

	dt.error = error;

	if (argc != 2)
		return -1;
	if (loadfile(argv[1], &s))
		return -1;
	if (dt_init(&dt, s.data, s.size))
		return -1;

	printf("magic %x\n", dt.hdr.magic);
	printf("size %d\n", dt.hdr.size);
	printf("off_struct %d (%d)\n", dt.hdr.off_struct, dt.hdr.sz_struct);
	printf("off_strings %d (%d)\n", dt.hdr.off_strings, dt.hdr.sz_strings);
	printf("version %d (min %d)\n", dt.hdr.version, dt.hdr.version_compat);

	dt_walk(&dt, node_cb, prop_cb, NULL);
	return 0;
}

