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

#include <app.h>

#include <platform.h>
#include <stdio.h>
#include <debug.h>
#include <string.h>
#include <pow2.h>
#include <err.h>
#include <assert.h>
#include <trace.h>

#include <kernel/thread.h>
#include <kernel/mutex.h>

#include <kernel/vm.h>
#include <app/lkboot.h>

#include "lkboot.h"

#if WITH_LIB_MINIP
#include <lib/minip.h>
#define LKBOOT_WITH_SERVER 1
#else
#define LKBOOT_WITH_SERVER 0
#endif
//#define LKBOOT_SERVER_TIMEOUT INFINITE_TIME
#define LKBOOT_SERVER_TIMEOUT 5000

#define LOCAL_TRACE 0

void *lkb_iobuffer = 0;
paddr_t lkb_iobuffer_phys = 0;
size_t lkb_iobuffer_size = 16*1024*1024;

#define STATE_OPEN 0
#define STATE_DATA 1
#define STATE_RESP 2
#define STATE_DONE 3
#define STATE_ERROR 4

typedef struct LKB {
    tcp_socket_t *s;
    int state;
    size_t avail;
} lkb_t;

extern int lkb_handle_command(lkb_t *lkb, const char *cmd, const char *arg, unsigned len, const char **result);
extern status_t do_flash_boot(void);

static int readx(void *s, void *_data, size_t len) {
    char *data = _data;
    while (len > 0) {
        int r = tcp_read(s, data, len);
        if (r <= 0) return -1;
        data += r;
        len -= r;
    }
    return 0;
}

static int lkb_send(lkb_t *lkb, u8 opcode, const void *data, size_t len) {
    msg_hdr_t hdr;

    // once we sent our OKAY or FAIL or errored out, no more writes
    if (lkb->state >= STATE_DONE) return -1;

    switch (opcode) {
    case MSG_OKAY:
    case MSG_FAIL:
        lkb->state = STATE_DONE;
        if (len > 0xFFFF) return -1;
        break;
    case MSG_LOG:
        if (len > 0xFFFF) return -1;
        break;
    case MSG_SEND_DATA:
        if (len > 0x10000) return -1;
        break;
    case MSG_GO_AHEAD:
        if (lkb->state == STATE_OPEN) {
            lkb->state = STATE_DATA;
            break;
        }
        len = 0;
    default:
        lkb->state = STATE_ERROR;
        opcode = MSG_FAIL;
        data = "internal error";
        len = 14;
        break;
    }

    hdr.opcode = opcode;
    hdr.extra = 0;
    hdr.length = (opcode == MSG_SEND_DATA) ? (len - 1) : len;
    if (tcp_write(lkb->s, &hdr, sizeof(hdr)) != sizeof(&hdr)) {
        printf("xmit hdr fail\n");
        lkb->state = STATE_ERROR;
        return -1;
    }
    if (len && (tcp_write(lkb->s, data, len) != (ssize_t)len)) {
        printf("xmit data fail\n");
        lkb->state = STATE_ERROR;
        return -1;
    }
    return 0;
}

#define lkb_okay(lkb) lkb_send(lkb, MSG_OKAY, NULL, 0)
#define lkb_fail(lkb, msg) lkb_send(lkb, MSG_FAIL, msg, strlen(msg))

int lkb_write(lkb_t *lkb, const void *_data, size_t len) {
    const char *data = _data;
    while (len > 0) {
        size_t xfer = (len > 65536) ? 65536 : len;
        if (lkb_send(lkb, MSG_SEND_DATA, data, xfer)) return -1;
        len -= xfer;
        data += xfer;
    }
    return 0;
}

int lkb_read(lkb_t *lkb, void *_data, size_t len) {
    char *data = _data;

    printf("lkb_read %d\n", len);
    if (lkb->state == STATE_RESP) {
        return 0;
    }
    if (lkb->state == STATE_OPEN) {
        if (lkb_send(lkb, MSG_GO_AHEAD, NULL, 0)) return -1;
    }
    while (len > 0) {
        if (lkb->avail == 0) {
            msg_hdr_t hdr;
            if (readx(lkb->s, &hdr, sizeof(hdr))) goto fail;
            if (hdr.opcode == MSG_END_DATA) {
                lkb->state = STATE_RESP;
                return -1;
            }
            if (hdr.opcode != MSG_SEND_DATA) goto fail;
            lkb->avail = ((size_t) hdr.length) + 1;
        }
        if (lkb->avail >= len) {
            if (readx(lkb->s, data, len)) goto fail;
            lkb->avail -= len;
            return 0;
        }
        if (readx(lkb->s, data, lkb->avail)) {
            lkb->state = STATE_ERROR;
            return -1;
        }
        data += lkb->avail;
        len -= lkb->avail;
        lkb->avail = 0;
    }
    return 0;

fail:
    lkb->state = STATE_ERROR;
    return -1;
}

static status_t handle_txn(tcp_socket_t *s)
{
    msg_hdr_t hdr;
    lkb_t lkb;
    char cmd[128];
    char *arg;
    int err;
    const char *result;
    unsigned len;

    if (readx(s, &hdr, sizeof(hdr))) goto fail;
    if (hdr.opcode != MSG_CMD) goto fail;
    if (hdr.length > 127) goto fail;
    if (readx(s, cmd, hdr.length)) goto fail;
    cmd[hdr.length] = 0;

    printf("lkboot: recv '%s'\n", cmd);

    if (!(arg = strchr(cmd, ':'))) goto fail;
    *arg++ = 0;
    len = atoul(arg);
    if (!(arg = strchr(arg, ':'))) goto fail;
    arg++;
    
    lkb.s = s;
    lkb.state = STATE_OPEN;
    lkb.avail = 0;
    err = lkb_handle_command(&lkb, cmd, arg, len, &result);
    if (err >= 0) {
        lkb_okay(&lkb);
    } else {
        lkb_fail(&lkb, result);
    }

    return NO_ERROR;

fail:
    tcp_close(s);
    return ERR_IO;
}

static void lkboot_task(const struct app_descriptor *app, void *args)
{
#if LKBOOT_WITH_SERVER
    tcp_socket_t *listen_socket = NULL;
    bool wait_forever = (LKBOOT_SERVER_TIMEOUT == INFINITE_TIME);

    // allocate memory for the server
    if (vmm_alloc_contiguous(vmm_get_kernel_aspace(), "lkboot_iobuf",
        lkb_iobuffer_size, &lkb_iobuffer, log2_uint(lkb_iobuffer_size), 0,
        ARCH_MMU_FLAG_UNCACHED) < 0) {
        goto regboot;
    }
    if (arch_mmu_query((u32) lkb_iobuffer, &lkb_iobuffer_phys, NULL) < 0) {
        goto regboot;
    }
    printf("lkboot: iobuffer %p (phys 0x%lx)\n", lkb_iobuffer, lkb_iobuffer_phys);

serverloop:
    /* open the server's socket */
    if (tcp_open_listen(&listen_socket, 1023) < 0) {
        printf("lkboot: error opening listen socket\n");
        goto regboot;
    }

    /* run the main lkserver loop */
    printf("lkboot: starting network server\n");
    lk_time_t t = current_time();
    for (;;) {
        tcp_socket_t *s;

        /* calculate how much longer we will wait */
        lk_time_t timeout = INFINITE_TIME;
        if (!wait_forever) {
            timeout = current_time() - t;
            if (timeout >= LKBOOT_SERVER_TIMEOUT)
                break;
            timeout = LKBOOT_SERVER_TIMEOUT - timeout;
        }

        /* wait for a new connection */
        if (tcp_accept_timeout(listen_socket, &s, timeout) < 0) {
            continue;
        }

        DEBUG_ASSERT(s);

        /* handle the command and close it */
        handle_txn(s);
        tcp_close(s);

        /* after the first command, stay in the server loop forever */
        if (!wait_forever) {
            wait_forever = true;
            printf("lkboot: handled command, staying in server loop\n");
        }
    }

    tcp_close(listen_socket);

    printf("lkboot: network server timed out\n");
#endif

regboot:
    printf("lkboot: trying to boot from flash...\n");

    status_t err = do_flash_boot();
    TRACEF("do_flash_boot returns %d\n", err);

#if LKBOOT_WITH_SERVER
    if (err < 0) {
        TRACEF("restarting network server\n");
        wait_forever = true;
        goto serverloop;
    }
#endif

    TRACEF("nothing to do, exiting\n");
}

APP_START(lkboot)
    .entry = lkboot_task,
    .flags = 0,
APP_END
