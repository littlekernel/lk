/*
 * Copyright (c) 2014 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <app.h>
#include <lk/err.h>
#include <lk/debug.h>
#include <lk/trace.h>
#include <stdio.h>
#include <stdlib.h>
#include <lk/compiler.h>
#include <kernel/thread.h>
#include <lib/minip.h>
#include <lib/tftp.h>
#include <lib/cksum.h>
#include <platform.h>

#define LOCAL_TRACE 1

static int telnet_worker(void *arg) {
    tcp_socket_t *s = static_cast<tcp_socket_t *>(arg);

    char buf[128];
    for (;;) {
        ssize_t err = tcp_read(s, buf, sizeof(buf));
        if (err < 0) {
            printf("TELENT: error from read, exiting\n");
            return err;
        }

        hexdump8(buf, err);
    }

    return 0;
}

static void telnetd_entry(const struct app_descriptor *app, void *args) {
    printf("TELNET: waiting for network configuration\n");
    minip_wait_for_configured(INFINITE_TIME);
    printf("TELNET: starting telnet server\n");

    // starting telnet stack
    tcp_socket_t *listen_socket;
    status_t err = tcp_open_listen(&listen_socket, 23);
    if (err < 0) {
        printf("tcp_open_listen returns %d\n", err);
    }

    for (;;) {
        tcp_socket_t *accept_socket;

        err = tcp_accept(listen_socket, &accept_socket);
        LTRACEF("tcp_accept returns returns %d, handle %p\n", err, accept_socket);
        if (err < 0) {
            TRACEF("error accepting socket, retrying\n");
            continue;
        }

        printf("TELNET: starting worker\n");
        thread_detach_and_resume(thread_create("chargen_worker", &telnet_worker, accept_socket, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE));
    }
}

APP_START(inetsrv)
.init = nullptr,
.entry = telnetd_entry,
.flags = 0,
.stack_size = 0,
APP_END
