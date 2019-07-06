/*
 * Copyright 2016 Google Inc. All Rights Reserved.
 * Author: gkalsi@google.com (Gurjant Kalsi)
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <app.h>
#include <assert.h>
#include <lk/err.h>
#include <dev/usb/class/cdcserial.h>
#include <kernel/thread.h>

static uint8_t rxbuf[64];
static cdcserial_channel_t *cdc_chan;

void cdctest_setup(cdcserial_channel_t *chan) {
    cdc_chan = chan;
}

static void cdctest_init(const struct app_descriptor *app) {
}

// Read bytes from CDC Serial and write them back to the stream.
static void cdctest_entry(const struct app_descriptor *app, void *args) {
    assert(cdc_chan != NULL);
    while (true) {
        ssize_t bytes = cdcserial_read(cdc_chan, sizeof(rxbuf), rxbuf);
        if (bytes == ERR_NOT_READY) {
            // USB is not ready yet.
            thread_sleep(100);
            continue;
        } else if (bytes < 0) {
            printf("Error reading bytes from CDC Serial: %ld\n", bytes);
            break;
        }

        cdcserial_write(cdc_chan, bytes, rxbuf);
    }
}

APP_START(usbtest)
.init = cdctest_init,
.entry = cdctest_entry,
APP_END
