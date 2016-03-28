/*
 * Copyright 2016 Google Inc. All Rights Reserved.
 * Author: gkalsi@google.com (Gurjant Kalsi)
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

#include <app/moot/usbboot.h>

#include <app/moot/stubs.h>
#include <dev/udc.h>
#include <dev/usb.h>
#include <dev/usbc.h>
#include <err.h>
#include <kernel/event.h>
#include <kernel/thread.h>
#include <lib/bio.h>
#include <lib/buildsig.h>
#include <lib/version.h>
#include <platform.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <trace.h>

#define LOCAL_TRACE 0
#define COMMAND_MAGIC (0x4d4f4f54)  // MOOT
#define RESP_MAGIC    (0x52455350)  // RESP

#define USB_XFER_SIZE (512)
#define W(w) (w & 0xff), (w >> 8)
#define W3(w) (w & 0xff), ((w >> 8) & 0xff), ((w >> 16) & 0xff)

// How long should we wait for activity on USB before we continue to boot?
#define USB_BOOT_TIMEOUT (3000)
#define USB_READ_TIMEOUT (10000)

static const uint8_t if_descriptor[] = {
    0x09,           /* length */
    INTERFACE,      /* type */
    0x01,           /* interface num */
    0x00,           /* alternates */
    0x02,           /* endpoint count */
    0xff,           /* interface class */
    0x01,           /* interface subclass */
    0x00,           /* interface protocol */
    0x00,           /* string index */

    /* endpoint 1 IN */
    0x07,           /* length */
    ENDPOINT,       /* type */
    0x1 | 0x80,     /* address: 1 IN */
    0x02,           /* type: bulk */
    W(64),          /* max packet size: 64 */
    00,             /* interval */

    /* endpoint 1 OUT */
    0x07,           /* length */
    ENDPOINT,       /* type */
    0x1,            /* address: 1 OUT */
    0x02,           /* type: bulk */
    W(64),          /* max packet size: 64 */
    00,             /* interval */
};

// Everything is okay.
#define USB_RESP_NO_ERROR             (0x00)
#define USB_RESP_XMIT_READY           (0x01)
#define USB_RESP_RECV_READY           (0x02)

// Malformed reqeust
#define USB_RESP_BAD_DATA_LEN         (0xAAA0)
#define USB_RESP_BAD_MAGIC            (0xAAA1)
#define USB_RESP_UNKNOWN_COMMAND      (0xAAA2)

// Device Side System Errors
#define USB_RESP_SYS_IMAGE_TOO_BIG    (0xFFF1)
#define USB_RESP_ERR_OPEN_SYS_FLASH   (0xFFF2)
#define USB_RESP_ERR_ERASE_SYS_FLASH  (0xFFF3)
#define USB_RESP_ERR_WRITE_SYS_FLASH  (0xFFF4)
#define USB_RESP_CANT_FIND_BUILDSIG   (0xFFF5)
#define USB_RESP_USB_READ_ERROR       (0xFFF6)

/* Bootloader commands */
#define USB_CMD_FLASH    (0x01)
#define USB_CMD_BOOT     (0x02)
#define USB_CMD_DEVINFO  (0x03)

typedef struct cmd_header {
    uint32_t magic;
    uint32_t cmd;
    uint32_t arg;
} cmd_header_t;

typedef struct cmd_response {
    uint32_t magic;
    uint32_t code;
    uint32_t arg;
} cmd_response_t;

// USB Functions
static void usb_xmit(void *data, size_t len);
static status_t usb_recv(void *data, size_t len, lk_time_t timeout,
                         size_t *actual);
static status_t usb_xmit_cplt_cb(ep_t endpoint, usbc_transfer_t *t);
static status_t usb_recv_cplt_cb(ep_t endpoint, usbc_transfer_t *t);
static status_t usb_register_cb(
    void *cookie, usb_callback_op_t op, const union usb_callback_args *args);

static event_t txevt = EVENT_INITIAL_VALUE(txevt, 0, EVENT_FLAG_AUTOUNSIGNAL);
static event_t rxevt = EVENT_INITIAL_VALUE(rxevt, 0, EVENT_FLAG_AUTOUNSIGNAL);
static volatile bool usb_online = false;

// Command processor that handles USB boot commands.
static bool handle_usb_cmd(
    const void *data, const size_t n, cmd_response_t *resp
) {
    static uint8_t buffer[4096];

    DEBUG_ASSERT(resp);

    resp->magic = RESP_MAGIC;
    resp->code = USB_RESP_NO_ERROR;
    resp->arg = 0;

    // Make sure we have enough data.
    if (n < sizeof(cmd_header_t)) {
        resp->code = USB_RESP_BAD_DATA_LEN;
        return false;
    }

    cmd_header_t *header = (cmd_header_t *)data;
    if (header->magic != COMMAND_MAGIC) {
        resp->code = USB_RESP_BAD_MAGIC;
        return false;
    }

    ssize_t image_length;
    const lk_version_t *version;
    status_t st;
    switch (header->cmd) {
        case USB_CMD_FLASH:
            image_length = header->arg;
            if (image_length > (ssize_t)moot_system_info.system_len) {
                resp->code = USB_RESP_SYS_IMAGE_TOO_BIG;
                break;
            }

            // Make space on the flash for the data.
            bdev_t *dev = bio_open(moot_system_info.system_flash_name);
            if (!dev) {
                resp->code = USB_RESP_ERR_OPEN_SYS_FLASH;
                break;
            }

            ssize_t n_bytes_erased =
                bio_erase(dev, moot_system_info.system_offset, image_length);
            if (n_bytes_erased < image_length) {
                resp->code = USB_RESP_ERR_ERASE_SYS_FLASH;
                goto close_and_exit;
            }

            // Signal to the host to start sending the image over.
            resp->code = USB_RESP_RECV_READY;
            resp->arg = 0;
            usb_xmit((void *)resp, sizeof(*resp));

            off_t addr = moot_system_info.system_offset;
            while (image_length > 0) {
                ssize_t xfer = (image_length > (ssize_t)sizeof(buffer)) ?
                               (ssize_t)sizeof(buffer) : image_length;

                size_t bytes_received;
                st = usb_recv(buffer, xfer, USB_READ_TIMEOUT, &bytes_received);
                if (st != NO_ERROR) {
                    resp->code = USB_RESP_USB_READ_ERROR;
                    goto close_and_exit;
                }

                if (xfer != (ssize_t)bytes_received) {
                    resp->code = USB_RESP_BAD_DATA_LEN;
                    goto close_and_exit;
                }

                ssize_t written = bio_write(dev, buffer, addr, bytes_received);
                if (written != (ssize_t)bytes_received) {
                    resp->code = USB_RESP_ERR_WRITE_SYS_FLASH;
                    goto close_and_exit;
                }

                addr += written;
                image_length -= written;
            }

            resp->code = USB_RESP_NO_ERROR;

close_and_exit:
            bio_close(dev);
            break;
        case USB_CMD_BOOT:
            resp->code = USB_RESP_NO_ERROR;
            resp->arg = 0;
            return true;
            break;
        case USB_CMD_DEVINFO:
            st = buildsig_search(
                     (void *)moot_system_info.sys_base_addr,
                     DEFAULT_BUILDSIG_SEARCH_LEN,
                     1024*1024,
                     &version
                 );
            if (st != NO_ERROR) {
                resp->code = USB_RESP_CANT_FIND_BUILDSIG;
                break;
            }

            size_t buflen = sizeof(buffer);
            snprintf((char *)buffer, buflen, "%s\n%s\n%s\n%s\n%s",
                     version->arch, version->platform, version->target,
                     version->project, version->buildid);

            resp->code = USB_RESP_XMIT_READY;
            resp->arg = strnlen((char *)buffer, buflen);
            usb_xmit((void *)resp, sizeof(*resp));
            usb_xmit((void *)buffer, resp->arg);

            resp->arg = 0;
            resp->code = USB_RESP_NO_ERROR;
            break;
        default:
            resp->arg = 0;
            resp->code = USB_RESP_UNKNOWN_COMMAND;
            break;
    }

finish:
    return false;
}

void init_usb_boot(void)
{
    usb_register_callback(&usb_register_cb, NULL);
}

void append_usb_interfaces(void)
{
    usb_append_interface_lowspeed(if_descriptor, sizeof(if_descriptor));
    usb_append_interface_highspeed(if_descriptor, sizeof(if_descriptor));
}

void attempt_usb_boot(void)
{
    static uint8_t buf[USB_XFER_SIZE];

    lk_time_t start = current_time();
    lk_time_t timeout = USB_BOOT_TIMEOUT;
    size_t bytes_received;

    while (current_time() - start < timeout) {
        if (!usb_online) {
            thread_yield();
            continue;
        }

        status_t r = usb_recv(buf, USB_XFER_SIZE, timeout, &bytes_received);
        if (r == ERR_TIMED_OUT) {
            goto finish;
        } else if (r == NO_ERROR) {
            // Somebody tried to talk to us over USB, they own the boot now.
            cmd_response_t response;
            bool should_boot = handle_usb_cmd(buf, bytes_received, &response);
            usb_xmit((void *)&response, sizeof(response));
            timeout = INFINITE_TIME;
            if (should_boot) {
                goto finish;
            }
        }
    }

finish:
    return;
}

static status_t usb_register_cb(
    void *cookie,
    usb_callback_op_t op,
    const union usb_callback_args *args
)
{
    if (op == USB_CB_ONLINE) {
        usbc_setup_endpoint(1, USB_IN, 0x40, USB_BULK);
        usbc_setup_endpoint(1, USB_OUT, 0x40, USB_BULK);
        usb_online = true;
    }
    return NO_ERROR;
}

static status_t usb_xmit_cplt_cb(ep_t endpoint, usbc_transfer_t *t)
{
    event_signal(&txevt, false);
    return 0;
}

static status_t usb_recv_cplt_cb(ep_t endpoint, usbc_transfer_t *t)
{
    event_signal(&rxevt, false);
    return 0;
}

static void usb_xmit(void *data, size_t len)
{
    usbc_transfer_t transfer = {
        .callback = &usb_xmit_cplt_cb,
        .result   = 0,
        .buf      = data,
        .buflen   = len,
        .bufpos   = 0,
        .extra    = 0,
    };

    usbc_queue_tx(1, &transfer);
    event_wait(&txevt);
}

static status_t usb_recv(
    void *data, size_t len, lk_time_t timeout, size_t *actual
) {
    usbc_transfer_t transfer = {
        .callback = &usb_recv_cplt_cb,
        .result = 0,
        .buf = data,
        .buflen = len,
        .bufpos = 0,
        .extra = 0,
    };

    usbc_queue_rx(1, &transfer);
    status_t res = event_wait_timeout(&rxevt, timeout);

    if (res != NO_ERROR) {
        return res;
    }

    *actual = transfer.bufpos;
    return NO_ERROR;
}