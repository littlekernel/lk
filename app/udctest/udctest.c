/*
 * Copyright (c) 2015 Brian Swetland
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
#include <debug.h>
#include <string.h>
#include <stdlib.h>
#include <printf.h>
#include <dev/udc.h>

udc_request_t *txreq;
udc_request_t *rxreq;
udc_endpoint_t *txept;
udc_endpoint_t *rxept;

static char rxbuf[4096];

static void rx_complete(udc_request_t *req, unsigned actual, int status)
{
    //printf("rx done %d %d\n", actual, status);
    if (status == 0) {
        udc_request_queue(rxept, rxreq);
    }
}

static void tx_complete(udc_request_t *req, unsigned actual, int status)
{
    //printf("tx done %d %d\n", actual, status);
    if (status == 0) {
        udc_request_queue(txept, txreq);
    }
}

static void udctest_notify(udc_gadget_t *gadget, unsigned event)
{
    printf("event %d\n", event);
    if (event == UDC_EVENT_ONLINE) {
        udc_request_queue(rxept, rxreq);
        udc_request_queue(txept, txreq);
    }
}

static udc_device_t udctest_device = {
    .vendor_id = 0x18d1,
    .product_id = 0xdb01,
    .version_id = 0x0100,
    .manufacturer = "Frobozz Magic USB Device Company",
    .product = "Frobozzco USB Device",
    .serialno = "00000005",
};

static udc_endpoint_t *udctest_endpoints[2];

static udc_gadget_t udctest_gadget = {
    .notify = udctest_notify,
    .ifc_class = 0xFF,
    .ifc_subclass = 0x42,
    .ifc_protocol = 0x01,
    .ifc_endpoints = 2,
    .ifc_string = "string",
    .ept = udctest_endpoints,
};

static void udctest_init(const struct app_descriptor *app)
{
    printf("usbtest_init()\n");
    udc_init(&udctest_device);
    udctest_endpoints[0] = txept = udc_endpoint_alloc(UDC_BULK_IN, 512);
    udctest_endpoints[1] = rxept = udc_endpoint_alloc(UDC_BULK_OUT, 512);
    txreq = udc_request_alloc();
    rxreq = udc_request_alloc();
    rxreq->buffer = rxbuf;
    rxreq->length = sizeof(rxbuf);
    rxreq->complete = rx_complete;
    txreq->buffer = rxbuf;
    txreq->length = sizeof(rxbuf);
    txreq->complete = tx_complete;
    udc_register_gadget(&udctest_gadget);
}

static void udctest_entry(const struct app_descriptor *app, void *args)
{
    printf("udctest_entry()\n");
    udc_start();
}

APP_START(usbtest)
.init = udctest_init,
 .entry = udctest_entry,
  APP_END


