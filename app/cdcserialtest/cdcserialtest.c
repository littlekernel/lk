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

#include <app.h>
#include <err.h>
#include <dev/usb/class/cdcserial.h>
#include <kernel/thread.h>

static uint8_t rxbuf[64];

static void cdctest_init(const struct app_descriptor *app)
{
}

// Read bytes from CDC Serial and write them back to the stream.
static void cdctest_entry(const struct app_descriptor *app, void *args)
{
    while (true) {
        ssize_t bytes = cdcserial_read(sizeof(rxbuf), rxbuf);
        if (bytes == ERR_NOT_READY) {
            // USB is not ready yet.
            thread_sleep(100);
            continue;
        } else if (bytes < 0) {
            printf("Error reading bytes from CDC Serial: %ld\n", bytes);
            break;
        }

        cdcserial_write(bytes, rxbuf);
    }
}

APP_START(usbtest)
 .init = cdctest_init,
 .entry = cdctest_entry,
APP_END
