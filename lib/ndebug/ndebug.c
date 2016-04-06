#include <lib/ndebug.h>

#include <dev/udc.h>
#include <dev/usb.h>
#include <dev/usbc.h>
#include <kernel/event.h>
#include <string.h>
#include <err.h>
#include <assert.h>

#define W(w) (w & 0xff), (w >> 8)
#define W3(w) (w & 0xff), ((w >> 8) & 0xff), ((w >> 16) & 0xff)

#define IFACE_SUBCLASS_IDX (0x06)
#define IFACE_PROTOCOL_IDX (0x07)
#define IFACE_IN_ADDR_IDX  (0x0B)
#define IFACE_OUT_ADDR_IDX (0x12)

#define CH_TO_ADDR(CH) ((CH) + 1)
#define CHECK_CHANNEL(CH) \
		do { DEBUG_ASSERT((CH) < NDEBUG_CHANNEL_COUNT); } while (false);

#define SYS_EP_ADDR (CH_TO_ADDR(NDEBUG_CHANNEL_SYS))
#define USR_EP_ADDR (CH_TO_ADDR(NDEBUG_CHANNEL_USR))

#define NDEBUG_SUBCLASS (0x02)

#define NDEBUG_PROTOCOL_LK_SYSTEM (0x01)
#define NDEBUG_PROTOCOL_SERIAL_PIPE (0x02)

static event_t rx_event[NDEBUG_CHANNEL_COUNT];
static event_t tx_event[NDEBUG_CHANNEL_COUNT];

static const uint8_t bulk_pair_descriptor_template[] = {
    0x09,           /* length */
    INTERFACE,      /* type */
    0x00,           /* interface num - Unused, patched in by dev/usb */
    0x00,           /* alternates */
    0x02,           /* endpoint count */
    0xff,           /* interface class - User Deficned */
    0x00,           /* interface subclass - Patched by client */
    0x00,           /* interface protocol - Patched by client */
    0x00,           /* string index */

    /* endpoint 1 IN */
    0x07,           /* length */
    ENDPOINT,       /* type */
    0x80,           /* address - Patched by Client */
    0x02,           /* type: bulk */
    W(64),          /* max packet size: 64 */
    00,             /* interval - unused for bulk */

    /* endpoint 1 OUT */
    0x07,           /* length */
    ENDPOINT,       /* type */
    0x00,           /* address - Patched by client */
    0x02,           /* type: bulk */
    W(64),          /* max packet size: 64 */
    00,             /* interval - unused for bulk */
};

static void init_channel(uint8_t subclass, uint8_t protocol, uint8_t ep_addr)
{
	uint8_t desc[sizeof(bulk_pair_descriptor_template)];

	// Make a copy of the template descriptor and fill in the missing fields.
	memcpy(desc, bulk_pair_descriptor_template, 
		   sizeof(bulk_pair_descriptor_template));

	desc[IFACE_SUBCLASS_IDX] = subclass;
	desc[IFACE_PROTOCOL_IDX] = protocol;
	desc[IFACE_IN_ADDR_IDX] = ep_addr | 0x80;
	desc[IFACE_OUT_ADDR_IDX] = ep_addr;

	// Append the interfaces.
	usb_append_interface_lowspeed(desc, sizeof(desc));
    usb_append_interface_highspeed(desc, sizeof(desc));
}

static void setup_usb_endpoint(uint8_t ep_num)
{
	usbc_setup_endpoint(ep_num, USB_IN, 0x40, USB_BULK);
	usbc_setup_endpoint(ep_num, USB_OUT, 0x40, USB_BULK);
}

static status_t ndebug_register_cb(
    void *cookie,
    usb_callback_op_t op,
    const union usb_callback_args *args
)
{
    if (op == USB_CB_ONLINE) {
        for (channel_t ch = 0; ch < NDEBUG_CHANNEL_COUNT; ++ch) {
    		setup_usb_endpoint(CH_TO_ADDR(ch));
    	}
    }
    return NO_ERROR;
}

void ndebug_init(void)
{
	for (channel_t ch = 0; ch < NDEBUG_CHANNEL_COUNT; ++ch) {
		event_init(&rx_event[ch], 0, EVENT_FLAG_AUTOUNSIGNAL);
		event_init(&tx_event[ch], 0, EVENT_FLAG_AUTOUNSIGNAL);
    }

    init_channel(NDEBUG_SUBCLASS, NDEBUG_PROTOCOL_LK_SYSTEM, SYS_EP_ADDR);
	init_channel(NDEBUG_SUBCLASS, NDEBUG_PROTOCOL_SERIAL_PIPE, USR_EP_ADDR);

    usb_register_callback(&ndebug_register_cb, NULL);
}

static status_t usb_xmit_cplt_cb(ep_t endpoint, usbc_transfer_t *t)
{
	uint32_t channel = (uint32_t)t->extra;
	CHECK_CHANNEL(channel);

    event_signal(&tx_event[channel], false);
    return 0;
}

static status_t usb_recv_cplt_cb(ep_t endpoint, usbc_transfer_t *t)
{
	uint32_t channel = (uint32_t)t->extra;
	CHECK_CHANNEL(channel);

    event_signal(&rx_event[channel], false);
    return 0;
}

ssize_t ndebug_read(const channel_t ch, const size_t n, uint8_t *buf)
{
	CHECK_CHANNEL(ch);

	usbc_transfer_t transfer = {
	    .callback = &usb_recv_cplt_cb,
	    .result = 0,
	    .buf = buf,
	    .buflen = n,
	    .bufpos = 0,
	    .extra = (void *)ch,
	};

    usbc_queue_rx(CH_TO_ADDR(ch), &transfer);
    status_t res = event_wait(&rx_event[ch]);

    if (res != NO_ERROR) {
    	return res;
    }

    return transfer.bufpos;
}

ssize_t ndebug_write(const channel_t ch, const size_t n, uint8_t *buf)
{
	CHECK_CHANNEL(ch);

	usbc_transfer_t transfer = {
	    .callback = &usb_xmit_cplt_cb,
	    .result = 0,
	    .buf = buf,
	    .buflen = n,
	    .bufpos = 0,
	    .extra = (void *)ch,
	};

    usbc_queue_tx(CH_TO_ADDR(ch), &transfer);
    status_t res = event_wait(&tx_event[ch]);

    if (res != NO_ERROR) {
    	return res;
    }

    return n;
}