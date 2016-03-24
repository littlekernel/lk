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
#include "../app/lkboot/lkboot_protocol.h"

static int readx(int s, void *_data, int len)
{
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

static int upload(int s, int txfd, size_t txlen, int do_endian_swap)
{
    int err = 0;
    msg_hdr_t hdr;

    char *buf = malloc(txlen);
    if (!buf)
        return -1;

    if (readx(txfd, buf, txlen)) {
        fprintf(stderr, "error: reading from file\n");
        err = -1;
        goto done;
    }

    /* 4 byte swap data if requested */
    if (do_endian_swap) {
        size_t i;
        for (i = 0; i < txlen; i += 4) {
            char temp = buf[i];
            buf[i] = buf[i + 3];
            buf[i + 3] = temp;

            temp = buf[i + 1];
            buf[i + 1] = buf[i + 2];
            buf[i + 2] = temp;
        }
    }

    size_t pos = 0;
    while (pos < txlen) {
        size_t xfer = (txlen - pos > 65536) ? 65536 : txlen - pos;

        hdr.opcode = MSG_SEND_DATA;
        hdr.extra = 0;
        hdr.length = xfer - 1;
        if (write(s, &hdr, sizeof(hdr)) != sizeof(hdr)) {
            fprintf(stderr, "error: writing socket\n");
            err = -1;
            goto done;
        }
        if (write(s, buf + pos, xfer) != xfer) {
            fprintf(stderr, "error: writing socket\n");
            err = -1;
            goto done;
        }
        pos += xfer;
    }

    hdr.opcode = MSG_END_DATA;
    hdr.extra = 0;
    hdr.length = 0;
    if (write(s, &hdr, sizeof(hdr)) != sizeof(hdr)) {
        fprintf(stderr, "error: writing socket\n");
        err = -1;
        goto done;
    }

done:
    free(buf);

    return err;
}

static off_t trim_fpga_image(int fd, off_t len)
{
    /* fd should be at start of bitfile, seek until the
     * ff ff ff ff aa 99 55 66 pattern is found and subtract
     * the number of bytes read until pattern found.
     */
    const unsigned char pat[] = { 0xff, 0xff, 0xff, 0xff, 0xaa, 0x99, 0x55, 0x66 };
    unsigned char buf[sizeof(pat)];

    memset(buf, 0, sizeof(buf));

    off_t i;
    for (i = 0; i < len; i++) {
        memmove(buf, buf + 1, sizeof(buf) - 1);
        if (read(fd, &buf[sizeof(buf) - 1], 1) < 1) {
            return -1;
        }

        /* look for pattern */
        if (memcmp(pat, buf, sizeof(pat)) == 0) {
            /* found it, rewind the fd and return the truncated length */
            lseek(fd, -sizeof(pat), SEEK_CUR);
            return len - (i + 1 - sizeof(pat));
        }
    }

    return -1;
}

#define DCC_SUBPROCESS "zynq-dcc"

static int start_dcc_subprocess(int *fd_in, int *fd_out)
{
    int outpipe[2];
    if (pipe(outpipe) != 0)
        return -1;

    int inpipe[2];
    if (pipe(inpipe) != 0)
        return -1;

    *fd_in = inpipe[0];
    *fd_out = outpipe[1];

    pid_t pid = fork();
    if (pid == 0) {
        /* we are the child */
        close(STDIN_FILENO);
        close(STDOUT_FILENO);

        dup2(outpipe[0], STDIN_FILENO);
        close(outpipe[1]);

        dup2(inpipe[1], STDOUT_FILENO);
        close(inpipe[0]);

        fprintf(stderr, "in the child\n");

        execlp(DCC_SUBPROCESS, DCC_SUBPROCESS, NULL);
        fprintf(stderr, "after exec, didn't work!\n");
    } else {
        fprintf(stderr, "in parent, pid %u\n", pid);

        close(outpipe[0]);
        close(inpipe[1]);
    }

    return 0;
}

#define REPLYMAX (9 * 1024 * 1024)
static unsigned char replybuf[REPLYMAX];
static unsigned replylen = 0;

unsigned lkboot_get_reply(void **ptr)
{
    *ptr = replybuf;
    return replylen;
}

int lkboot_txn(const char *host, const char *_cmd, int txfd, const char *args)
{
    msg_hdr_t hdr;
    char cmd[128];
    char tmp[65536];
    off_t txlen = 0;
    int do_endian_swap = 0;
    int once = 1;
    int len;
    int fd_in, fd_out;
    int ret = 0;

    if (txfd != -1) {
        txlen = lseek(txfd, 0, SEEK_END);
        if (txlen > (512*1024*1024)) {
            fprintf(stderr, "error: file too large\n");
            return -1;
        }
        lseek(txfd, 0, SEEK_SET);
    }

    if (!strcmp(_cmd, "fpga")) {
        /* if we were asked to send an fpga image, try to find the sync words and
         * trim all the data before it
         */
        txlen = trim_fpga_image(txfd, txlen);
        if (txlen < 0) {
            fprintf(stderr, "error: fpga image doesn't contain sync pattern\n");
            return -1;
        }

        /* it'll need a 4 byte endian swap as well */
        do_endian_swap = 1;
    }

    len = snprintf(cmd, 128, "%s:%d:%s", _cmd, (int) txlen, args);
    if (len > 127) {
        fprintf(stderr, "error: command too large\n");
        return -1;
    }

    /* if host is -, use stdin/stdout */
    if (!strcmp(host, "-")) {
        fprintf(stderr, "using stdin/stdout for io\n");
        fd_in = STDIN_FILENO;
        fd_out = STDOUT_FILENO;
    } else if (!strcasecmp(host, "jtag")) {
        fprintf(stderr, "using zynq-dcc utility for io\n");
        if (start_dcc_subprocess(&fd_in, &fd_out) < 0) {
            fprintf(stderr, "error starting jtag subprocess, is it in your path?\n");
            return -1;
        }
    } else {
        in_addr_t addr = lookup_hostname(host);
        if (addr == 0) {
            fprintf(stderr, "error: cannot find host '%s'\n", host);
            return -1;
        }
        while ((fd_in = tcp_connect(addr, 1023)) < 0) {
            if (once) {
                fprintf(stderr, "error: cannot connect to host '%s'. retrying...\n", host);
                once = 0;
            }
            usleep(100000);
        }
        fd_out = fd_in;
    }

    hdr.opcode = MSG_CMD;
    hdr.extra = 0;
    hdr.length = len;
    if (write(fd_out, &hdr, sizeof(hdr)) != sizeof(hdr)) goto iofail;
    if (write(fd_out, cmd, len) != len) goto iofail;

    for (;;) {
        if (readx(fd_in, &hdr, sizeof(hdr))) goto iofail;
        switch (hdr.opcode) {
            case MSG_GO_AHEAD:
                if (upload(fd_out, txfd, txlen, do_endian_swap)) {
                    ret = -1;
                    goto out;
                }
                break;
            case MSG_OKAY:
                ret = 0;
                goto out;
            case MSG_FAIL:
                len = (hdr.length > 127) ? 127 : hdr.length;
                if (readx(fd_in, cmd, len)) {
                    cmd[0] = 0;
                } else {
                    cmd[len] = 0;
                }
                fprintf(stderr,"error: remote failure: %s\n", cmd);
                ret = -1;
                goto out;
            case MSG_SEND_DATA:
                len = hdr.length + 1;
                if (readx(fd_in, tmp, len)) goto iofail;
                if (len > (REPLYMAX - replylen)) {
                    fprintf(stderr, "error: too much reply data\n");
                    ret = -1;
                    goto out;
                }
                memcpy(replybuf + replylen, tmp, len);
                replylen += len;
                break;
            default:
                fprintf(stderr, "error: unknown opcode %d\n", hdr.opcode);
                ret = -1;
                goto out;
        }
    }

iofail:
    fprintf(stderr, "error: socket io\n");
    ret = -1;

out:
    close(fd_in);
    if (fd_out != fd_in)
        close(fd_out);
    return ret;
}
