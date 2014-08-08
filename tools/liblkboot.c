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
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>

#include "network.h"
#include "../app/lkboot/lkboot.h"

static int readx(int s, void *_data, int len) {
	char *data = _data;
	int r;
	while (len > 0) {
		r = read(s, data, len);
		if (r == 0) {
			fprintf(stderr, "error: eof during socket read\n");
			return -1;
		}
		if (r < 0) {
			if (errno == EINTR) continue;
			fprintf(stderr, "error: %s during socket read\n", strerror(errno));
			return -1;
		}
		data += r;
		len -= r;
	}
	return 0;
}

static int upload(int s, int txfd, int txlen) {
	char buf[65536];
	msg_hdr_t hdr;

	while (txlen > 0) {
		int xfer = (txlen > 65536) ? 65536 : txlen;
		if (readx(txfd, buf, xfer)) {
			fprintf(stderr, "error: reading from file\n");
			return -1;
		}
		hdr.opcode = MSG_SEND_DATA;
		hdr.extra = 0;
		hdr.length = xfer - 1;
		if (write(s, &hdr, sizeof(hdr)) != sizeof(hdr)) {
			fprintf(stderr, "error: writing socket\n");
			return -1;
		}
		if (write(s, buf, xfer) != xfer) {
			fprintf(stderr, "error: writing socket\n");
			return -1;
		}
		txlen -= xfer;
	}
	hdr.opcode = MSG_END_DATA;
	hdr.extra = 0;
	hdr.length = 0;
	if (write(s, &hdr, sizeof(hdr)) != sizeof(hdr)) {
		fprintf(stderr, "error: writing socket\n");
		return -1;
	}
	return 0;
}

#define REPLYMAX (1 * 1024 * 1024)
static unsigned char replybuf[REPLYMAX];
static unsigned replylen = 0;

unsigned lkboot_get_reply(void **ptr) {
	*ptr = replybuf;
	return replylen;
}

int lkboot_txn(const char *host, const char *_cmd, int txfd, const char *args) {
	msg_hdr_t hdr;
	in_addr_t addr;
	char cmd[128];
	char tmp[65536];
	off_t txlen = 0;
	int len;
	int s;

	if (txfd != -1) {
		txlen = lseek(txfd, 0, SEEK_END);
		if (txlen > (512*1024*1024)) {
			fprintf(stderr, "error: file too large\n");
			return -1;
		}
		lseek(txfd, 0, SEEK_SET);
	}

	len = snprintf(cmd, 128, "%s:%d:%s", _cmd, (int) txlen, args);
	if (len > 127) {
		fprintf(stderr, "error: command too large\n");
		return -1;
	}

	addr = lookup_hostname(host);
	if (addr == 0) {
		fprintf(stderr, "error: cannot find host '%s'\n", host);
		return -1;
	}
	if ((s = tcp_connect(addr, 1023)) < 0) {
		fprintf(stderr, "error: cannot connect to host '%s'\n", host);
		return -1;
	}

	hdr.opcode = MSG_CMD;
	hdr.extra = 0;
	hdr.length = len;
	if (write(s, &hdr, sizeof(hdr)) != sizeof(hdr)) goto iofail;
	if (write(s, cmd, len) != len) goto iofail;

	for (;;) {	
		if (readx(s, &hdr, sizeof(hdr))) goto iofail;
		switch (hdr.opcode) {
		case MSG_GO_AHEAD:
			if (upload(s, txfd, txlen)) goto fail;
			break;
		case MSG_OKAY:
			close(s);
			return 0;
		case MSG_FAIL:
			len = (hdr.length > 127) ? 127 : hdr.length;
			if (readx(s, cmd, len)) {
				cmd[0] = 0;
			} else {
				cmd[len] = 0;
			}
			fprintf(stderr,"error: remote failure: %s\n", cmd);
			goto fail;
		case MSG_SEND_DATA:
			len = hdr.length + 1;
			if (readx(s, tmp, len)) goto iofail;
			if (len > (REPLYMAX - replylen)) {
				fprintf(stderr, "error: too much reply data\n");
				goto fail;
			}
			memcpy(replybuf + replylen, tmp, len);
			replylen += len;
			break;
		default:
			fprintf(stderr, "error: unknown opcode %d\n", hdr.opcode);
			goto fail;
		}
	}

iofail:
	fprintf(stderr, "error: socket io\n");
fail:
	close(s);
	return -1;
}

// vim: noexpandtab
