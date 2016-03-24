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

#include "lkboot.h"

#include <app.h>

#include <platform.h>
#include <stdio.h>
#include <debug.h>
#include <string.h>
#include <pow2.h>
#include <err.h>
#include <assert.h>
#include <trace.h>

#include <lib/sysparam.h>

#include <kernel/thread.h>
#include <kernel/mutex.h>

#include <kernel/vm.h>
#include <app/lkboot.h>

#if WITH_LIB_MINIP
#include <lib/minip.h>
#endif

#ifndef LKBOOT_WITH_SERVER
#define LKBOOT_WITH_SERVER 1
#endif
#ifndef LKBOOT_AUTOBOOT
#define LKBOOT_AUTOBOOT 1
#endif
#ifndef LKBOOT_AUTOBOOT_TIMEOUT
#define LKBOOT_AUTOBOOT_TIMEOUT 5000
#endif

#define LOCAL_TRACE 0

#define STATE_OPEN 0
#define STATE_DATA 1
#define STATE_RESP 2
#define STATE_DONE 3
#define STATE_ERROR 4

typedef struct LKB {
    lkb_read_hook *read;
    lkb_write_hook *write;

    void *cookie;

    int state;
    size_t avail;
} lkb_t;

lkb_t *lkboot_create_lkb(void *cookie, lkb_read_hook *read, lkb_write_hook *write) {
    lkb_t *lkb = malloc(sizeof(lkb_t));
    if (!lkb)
        return NULL;

    lkb->cookie = cookie;
    lkb->state = STATE_OPEN;
    lkb->avail = 0;
    lkb->read = read;
    lkb->write = write;

    return lkb;
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
    if (lkb->write(lkb->cookie, &hdr, sizeof(hdr)) != sizeof(&hdr)) {
        printf("xmit hdr fail\n");
        lkb->state = STATE_ERROR;
        return -1;
    }
    if (len && (lkb->write(lkb->cookie, data, len) != (ssize_t)len)) {
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

    if (lkb->state == STATE_RESP) {
        return 0;
    }
    if (lkb->state == STATE_OPEN) {
        if (lkb_send(lkb, MSG_GO_AHEAD, NULL, 0)) return -1;
    }
    while (len > 0) {
        if (lkb->avail == 0) {
            msg_hdr_t hdr;
            if (lkb->read(lkb->cookie, &hdr, sizeof(hdr))) goto fail;
            if (hdr.opcode == MSG_END_DATA) {
                lkb->state = STATE_RESP;
                return -1;
            }
            if (hdr.opcode != MSG_SEND_DATA) goto fail;
            lkb->avail = ((size_t) hdr.length) + 1;
        }
        if (lkb->avail >= len) {
            if (lkb->read(lkb->cookie, data, len)) goto fail;
            lkb->avail -= len;
            return 0;
        }
        if (lkb->read(lkb->cookie, data, lkb->avail)) {
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

status_t lkboot_process_command(lkb_t *lkb)
{
    msg_hdr_t hdr;
    char cmd[128];
    char *arg;
    int err;
    const char *result;
    unsigned len;

    if (lkb->read(lkb->cookie, &hdr, sizeof(hdr))) goto fail;
    if (hdr.opcode != MSG_CMD) goto fail;
    if (hdr.length > 127) goto fail;
    if (lkb->read(lkb->cookie, cmd, hdr.length)) goto fail;
    cmd[hdr.length] = 0;

    TRACEF("recv '%s'\n", cmd);

    if (!(arg = strchr(cmd, ':'))) goto fail;
    *arg++ = 0;
    len = atoul(arg);
    if (!(arg = strchr(arg, ':'))) goto fail;
    arg++;

    err = lkb_handle_command(lkb, cmd, arg, len, &result);
    if (err >= 0) {
        lkb_okay(lkb);
    } else {
        lkb_fail(lkb, result);
    }

    TRACEF("command handled with success\n");
    return NO_ERROR;

fail:
    TRACEF("command failed\n");
    return ERR_IO;
}

static status_t lkboot_server(lk_time_t timeout)
{
    lkboot_dcc_init();

#if WITH_LIB_MINIP
    /* open the server's socket */
    tcp_socket_t *listen_socket = NULL;
    if (tcp_open_listen(&listen_socket, 1023) < 0) {
        printf("lkboot: error opening listen socket\n");
        return ERR_NO_MEMORY;
    }
#endif

    /* run the main lkserver loop */
    printf("lkboot: starting server\n");
    lk_time_t t = current_time(); /* remember when we started */
    for (;;) {
        bool handled_command = false;

        lkb_t *lkb;

#if WITH_LIB_MINIP
        /* wait for a new connection */
        lk_time_t sock_timeout = 100;
        tcp_socket_t *s;
        if (tcp_accept_timeout(listen_socket, &s, sock_timeout) >= 0) {
            DEBUG_ASSERT(s);

            /* handle the command and close it */
            lkb = lkboot_tcp_opened(s);
            lkboot_process_command(lkb);
            free(lkb);
            tcp_close(s);
            handled_command = true;
        }
#endif

        /* check if anything is coming in on dcc */
        lkb = lkboot_check_dcc_open();
        if (lkb) {
            lkboot_process_command(lkb);
            free(lkb);
            handled_command = true;
        }

        /* after the first command, stay in the server loop forever */
        if (handled_command && timeout != INFINITE_TIME) {
            timeout = INFINITE_TIME;
            printf("lkboot: handled command, staying in server loop\n");
        }

        /* see if we need to drop out and try to direct boot */
        if (timeout != INFINITE_TIME && (current_time() - t >= timeout)) {
            break;
        }
    }

#if WITH_LIB_MINIP
    tcp_close(listen_socket);
#endif

    printf("lkboot: server timed out\n");

    return ERR_TIMED_OUT;
}

/* platform code can override this to conditionally abort autobooting from flash */
__WEAK bool platform_abort_autoboot(void)
{
    return false;
}

static void lkboot_task(const struct app_descriptor *app, void *args)
{
    /* read a few sysparams to decide if we're going to autoboot */
    uint8_t autoboot = 1;
    sysparam_read("lkboot.autoboot", &autoboot, sizeof(autoboot));

    /* let platform code have a shot at disabling the autoboot behavior */
    if (platform_abort_autoboot())
        autoboot = 0;

#if !LKBOOT_AUTOBOOT
    autoboot = 0;
#endif

    /* if we're going to autoobot, read the timeout value */
    lk_time_t autoboot_timeout;
    if (!autoboot) {
        autoboot_timeout = INFINITE_TIME;
    } else {
        autoboot_timeout = LKBOOT_AUTOBOOT_TIMEOUT;
        sysparam_read("lkboot.autoboot_timeout", &autoboot_timeout, sizeof(autoboot_timeout));
    }

    TRACEF("autoboot %u autoboot_timeout %u\n", autoboot, (uint)autoboot_timeout);

#if LKBOOT_WITH_SERVER
    lkboot_server(autoboot_timeout);
#else
    if (autoboot_timeout != INFINITE_TIME) {
        TRACEF("waiting for %u milliseconds before autobooting\n", (uint)autoboot_timeout);
        thread_sleep(autoboot_timeout);
    }
#endif

    if (autoboot_timeout != INFINITE_TIME) {
        TRACEF("trying to boot from flash...\n");
        status_t err = do_flash_boot();
        TRACEF("do_flash_boot returns %d\n", err);
    }

#if LKBOOT_WITH_SERVER
    TRACEF("restarting server\n");
    lkboot_server(INFINITE_TIME);
#endif

    TRACEF("nothing to do, exiting\n");
}

APP_START(lkboot)
    .entry = lkboot_task,
    .flags = 0,
APP_END
