#include <app.h>
#include <debug.h>
#include <err.h>
#include <string.h>
#include <stdlib.h>
#include <dev/usb.h>
#include <dev/usbc.h>
#include <kernel/debug.h>
#include <kernel/thread.h>
#include <kernel/event.h>

#define LOCAL_TRACE 1

extern void usbtest_usb_setup(void);

static status_t rx_callback(ep_t endpoint, struct usbc_transfer *transfer);
static usbc_transfer_t rx;
static uint8_t rxbuf[4096];
static volatile bool rxqueued;

static status_t tx_callback(ep_t endpoint, struct usbc_transfer *transfer);
static usbc_transfer_t tx;
static uint8_t txbuf[4095];
static volatile bool txqueued;

static event_t testevent;

/* RX */
static void queue_rx_transfer(void)
{
    rx.callback = rx_callback;
    rx.result = 0;
    rx.buf = rxbuf;
    rx.buflen = sizeof(rxbuf);
    rx.bufpos = 0;
    rx.extra = NULL;

    memset(rxbuf, 0x99, sizeof(rxbuf));

    rxqueued = true;
    usbc_queue_rx(1, &rx);
}

static status_t rx_callback(ep_t endpoint, struct usbc_transfer *transfer)
{
    LTRACEF("ep %u, transfer %p\n", endpoint, transfer);

    rxqueued = false;
    event_signal(&testevent, false);

    return NO_ERROR;
}

/* TX */
static void queue_tx_transfer(void)
{
    tx.callback = tx_callback;
    tx.result = 0;
    tx.buf = txbuf;
    tx.buflen = sizeof(txbuf);
    tx.bufpos = 0;
    tx.extra = NULL;

    for (uint i = 0; i < sizeof(txbuf); i++)
        txbuf[i] = i * 3;

    txqueued = true;
    usbc_queue_tx(1, &tx);
}

static status_t tx_callback(ep_t endpoint, struct usbc_transfer *transfer)
{
    LTRACEF("ep %u, transfer %p\n", endpoint, transfer);

    txqueued = false;
    event_signal(&testevent, false);

    return NO_ERROR;
}

static void usbtest_init(const struct app_descriptor *app)
{
    LTRACE_ENTRY;
    event_init(&testevent, false, EVENT_FLAG_AUTOUNSIGNAL);
    usbtest_usb_setup();
    LTRACE_EXIT;
}

static void usbtest_entry(const struct app_descriptor *app, void *args)
{
    LTRACE_ENTRY;

    TRACEF("starting usb stack\n");
    usb_start();

    // XXX get callback from stack
    thread_sleep(2000);

    TRACEF("queuing transfers\n");
    queue_rx_transfer();
    queue_tx_transfer();

    while (event_wait(&testevent) == NO_ERROR) {
        if (!rxqueued) {
            /* dump the state of the transfer */
            LTRACEF("rx transfer completed\n");
            usbc_dump_transfer(&rx);
            hexdump8(rx.buf, MIN(128, rx.bufpos));

            queue_rx_transfer();
        }
        if (!txqueued) {
            /* dump the state of the transfer */
            LTRACEF("tx transfer completed\n");
            usbc_dump_transfer(&tx);

            queue_tx_transfer();
        }
    }

    LTRACE_EXIT;
}

APP_START(usbtest)
    .init = usbtest_init,
    .entry = usbtest_entry,
APP_END


