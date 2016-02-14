/*
 * Copyright (c) 2015 Brian Swetland
 * Copyright (c) 2008 Google, Inc.
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

#include <string.h>
#include <stdlib.h>
#include <printf.h>
#include <assert.h>
#include <debug.h>
#include <reg.h>
#include <arch/arm/cm.h>
#include <kernel/thread.h>
#include <kernel/spinlock.h>

#include <platform/lpc43xx-usb.h>
static_assert(sizeof(usb_dqh_t) == 64);
static_assert(sizeof(usb_dtd_t) == 32);

#include <dev/udc.h>

#include "udc-common.h"

#define F_LL_INIT   1
#define F_UDC_INIT  2

// NOTE: I cheat a bit with the locking because this is a UP Cortex-M
// NOTE: device.  I use spinlocks for code that might be called from
// NOTE: userspace or irq context, but for the irq-only code I don't
// NOTE: bother with locking because it's impossible for it to execute
// NOTE: while the lock is held from userspace.

typedef struct {
    u32 base;
    spin_lock_t lock;

    usb_dqh_t *qh;
    usb_dtd_t *dtd_freelist;

    udc_endpoint_t *ep0in;
    udc_endpoint_t *ep0out;
    udc_request_t *ep0req;
    u8 txd[8];
    u8 rxd[8];

    udc_endpoint_t *ept_list;
    uint8_t online;
    uint8_t highspeed;
    uint8_t config_value;
    uint8_t flags;

    udc_device_t *device;
    udc_gadget_t *gadget;

    uint32_t ept_alloc_table;
} usb_t;

static usb_t USB;

typedef struct usb_request {
    udc_request_t req;
    struct usb_request *next;
    usb_dtd_t *dtd;
} usb_request_t;

struct udc_endpoint {
    udc_endpoint_t *next;
    usb_dqh_t *head;
    usb_request_t *req;
    usb_request_t *last;
    usb_t *usb;
    uint32_t bit;
    uint16_t maxpkt;
    uint8_t num;
    uint8_t in;
};

// ---- endpoint management

#if 1
#define DBG(x...) do {} while(0)
#else
#define DBG(x...) dprintf(INFO, x)
#endif

static udc_endpoint_t *_udc_endpoint_alloc(usb_t *usb,
        unsigned num, unsigned in, unsigned max_pkt)
{
    udc_endpoint_t *ept;
    unsigned cfg;

    ept = malloc(sizeof(*ept));
    ept->maxpkt = max_pkt;
    ept->num = num;
    ept->in = !!in;
    ept->req = 0;
    ept->last = 0;
    ept->usb = usb;

    cfg = DQH_CFG_MAXPKT(max_pkt) | DQH_CFG_ZLT;

    if (ept->in) {
        ept->bit = EPT_TX(ept->num);
    } else {
        ept->bit = EPT_RX(ept->num);
        if (num == 0) {
            cfg |= DQH_CFG_IOS;
        }
    }

    ept->head = usb->qh + (num * 2) + (ept->in);
    ept->head->config = cfg;
    ept->next = usb->ept_list;
    usb->ept_list = ept;

    DBG("ept%d %s @%p/%p max=%d bit=%x\n",
        num, in ? "in":"out", ept, ept->head, max_pkt, ept->bit);

    return ept;
}

udc_endpoint_t *udc_endpoint_alloc(unsigned type, unsigned maxpkt)
{
    udc_endpoint_t *ept;
    unsigned n;
    unsigned in = !!(type & 0x80);

    if (!(USB.flags & F_UDC_INIT)) {
        panic("udc_init() must be called before udc_endpoint_alloc()\n");
    }

    for (n = 1; n < 6; n++) {
        unsigned bit = in ? EPT_TX(n) : EPT_RX(n);
        if (USB.ept_alloc_table & bit) {
            continue;
        }
        if ((ept = _udc_endpoint_alloc(&USB, n, in, maxpkt))) {
            USB.ept_alloc_table |= bit;
        }
        return ept;
    }
    return 0;
}

void udc_endpoint_free(struct udc_endpoint *ept)
{
    // todo
}

static void handle_ept_complete(struct udc_endpoint *ept);

static void endpoint_flush(usb_t *usb, udc_endpoint_t *ept)
{
    if (ept->req) {
        // flush outstanding transfers
        writel(ept->bit, usb->base + USB_ENDPTFLUSH);
        while (readl(usb->base + USB_ENDPTFLUSH)) ;
        while (ept->req) {
            handle_ept_complete(ept);
        }
    }
}

static void endpoint_reset(usb_t *usb, udc_endpoint_t *ept)
{
    unsigned n = readl(usb->base + USB_ENDPTCTRL(ept->num));
    n |= ept->in ? EPCTRL_TXR : EPCTRL_RXR;
    writel(n, usb->base + USB_ENDPTCTRL(ept->num));
}

static void endpoint_enable(usb_t *usb, udc_endpoint_t *ept, unsigned yes)
{
    unsigned n = readl(usb->base + USB_ENDPTCTRL(ept->num));

    if (yes) {
        if (ept->in) {
            n |= (EPCTRL_TXE | EPCTRL_TXR | EPCTRL_TX_BULK);
        } else {
            n |= (EPCTRL_RXE | EPCTRL_RXR | EPCTRL_RX_BULK);
        }

        if (ept->num != 0) {
            // todo: support non-max-sized packet sizes
            if (usb->highspeed) {
                ept->head->config = DQH_CFG_MAXPKT(512) | DQH_CFG_ZLT;
            } else {
                ept->head->config = DQH_CFG_MAXPKT(64) | DQH_CFG_ZLT;
            }
        }
    }
    writel(n, usb->base + USB_ENDPTCTRL(ept->num));
}

// ---- request management

udc_request_t *udc_request_alloc(void)
{
    spin_lock_saved_state_t state;
    usb_request_t *req;
    if ((req = malloc(sizeof(*req))) == NULL) {
        return NULL;
    }

    spin_lock_irqsave(&USB.lock, state);
    if (USB.dtd_freelist == NULL) {
        spin_unlock_irqrestore(&USB.lock, state);
        free(req);
        return NULL;
    } else {
        req->dtd = USB.dtd_freelist;
        USB.dtd_freelist = req->dtd->next;
        spin_unlock_irqrestore(&USB.lock, state);

        req->req.buffer = 0;
        req->req.length = 0;
        return &req->req;
    }
}

void udc_request_free(struct udc_request *req)
{
    // todo: check if active?
    free(req);
}

int udc_request_queue(udc_endpoint_t *ept, struct udc_request *_req)
{
    spin_lock_saved_state_t state;
    usb_request_t *req = (usb_request_t *) _req;
    usb_dtd_t *dtd = req->dtd;
    unsigned phys = (unsigned) req->req.buffer;
    int ret = 0;

    dtd->next_dtd = 1; // terminate bit
    dtd->config = DTD_LEN(req->req.length) | DTD_IOC | DTD_ACTIVE;
    dtd->bptr0 = phys;
    phys &= 0xfffff000;
    dtd->bptr1 = phys + 0x1000;
    dtd->bptr2 = phys + 0x2000;
    dtd->bptr3 = phys + 0x3000;
    dtd->bptr4 = phys + 0x4000;

    req->next = 0;
    spin_lock_irqsave(&ept->usb->lock, state);
    if (!USB.online && ept->num) {
        ret = -1;
    } else if (ept->req) {
        // already a transfer in flight, add us to the list
        // we'll get queue'd by the irq handler when it's our turn
        ept->last->next = req;
    } else {
        ept->head->next_dtd = (unsigned) dtd;
        ept->head->dtd_config = 0;
        DSB;
        writel(ept->bit, ept->usb->base + USB_ENDPTPRIME);
        ept->req = req;
    }
    ept->last = req;
    spin_unlock_irqrestore(&ept->usb->lock, state);

    DBG("ept%d %s queue req=%p\n", ept->num, ept->in ? "in" : "out", req);
    return ret;
}

static void handle_ept_complete(struct udc_endpoint *ept)
{
    usb_request_t *req;
    usb_dtd_t *dtd;
    unsigned actual;
    int status;

    DBG("ept%d %s complete req=%p\n",
        ept->num, ept->in ? "in" : "out", ept->req);

    if ((req = ept->req)) {
        if (req->next) {
            // queue next req to hw
            ept->head->next_dtd = (unsigned) req->next->dtd;
            ept->head->dtd_config = 0;
            DSB;
            writel(ept->bit, ept->usb->base + USB_ENDPTPRIME);
            ept->req = req->next;
        } else {
            ept->req = 0;
            ept->last = 0;
        }
        dtd = req->dtd;
        if (dtd->config & 0xff) {
            actual = 0;
            status = -1;
            dprintf(INFO, "EP%d/%s FAIL nfo=%x pg0=%x\n",
                    ept->num, ept->in ? "in" : "out", dtd->config, dtd->bptr0);
        } else {
            actual = req->req.length - ((dtd->config >> 16) & 0x7fff);
            status = 0;
        }
        if (req->req.complete) {
            req->req.complete(&req->req, actual, status);
        }
    }
}

static void setup_ack(usb_t *usb)
{
    usb->ep0req->complete = 0;
    usb->ep0req->length = 0;
    udc_request_queue(usb->ep0in, usb->ep0req);
}

static void ep0in_complete(struct udc_request *req, unsigned actual, int status)
{
    usb_t *usb = (usb_t *) req->context;
    DBG("ep0in_complete %p %d %d\n", req, actual, status);
    if (status == 0) {
        req->length = 0;
        req->complete = 0;
        udc_request_queue(usb->ep0out, req);
    }
}

static void setup_tx(usb_t *usb, void *buf, unsigned len)
{
    DBG("setup_tx %p %d\n", buf, len);
    usb->ep0req->buffer = buf;
    usb->ep0req->complete = ep0in_complete;
    usb->ep0req->length = len;
    udc_request_queue(usb->ep0in, usb->ep0req);
}

static void notify_gadgets(udc_gadget_t *gadget, unsigned event)
{
    while (gadget) {
        if (gadget->notify) {
            gadget->notify(gadget, event);
        }
        gadget = gadget->next;
    }
}

#define SETUP(type,request) (((type) << 8) | (request))

static void handle_setup(usb_t *usb)
{
    union setup_packet s;

    // setup procedure, per databook
    // a. clear setup status by writing and waiting for 0 (1-2uS)
    writel(1, usb->base + USB_ENDPTSETUPSTAT);
    while (readl(usb->base + USB_ENDPTSETUPSTAT) & 1) ;
    do {
        // b. write 1 to tripwire
        writel(CMD_RUN | CMD_SUTW, usb->base + USB_CMD);
        // c. extract setup data
        s.w0 = usb->qh[0].setup0;
        s.w1 = usb->qh[0].setup1;
        // d. if tripwire clear, retry
    } while ((readl(usb->base + USB_CMD) & CMD_SUTW) == 0);
    // e. clear tripwire
    writel(CMD_RUN, usb->base + USB_CMD);
    // flush any pending io from previous setup transactions
    usb->ep0in->req = 0;
    usb->ep0out->req = 0;
    // f. process packet
    // g. ensure setup status is 0

    DBG("setup 0x%02x 0x%02x %d %d %d\n",
        s.type, s.request, s.value, s.index, s.length);

    switch (SETUP(s.type,s.request)) {
        case SETUP(DEVICE_READ, GET_STATUS): {
            static unsigned zero = 0;
            if (s.length == 2) {
                setup_tx(usb, &zero, 2);
                return;
            }
            break;
        }
        case SETUP(DEVICE_READ, GET_DESCRIPTOR): {
            struct udc_descriptor *desc = udc_descriptor_find(s.value);
            if (desc) {
                unsigned len = desc->len;
                if (len > s.length) len = s.length;
                setup_tx(usb, desc->data, len);
                return;
            }
            break;
        }
        case SETUP(DEVICE_READ, GET_CONFIGURATION):
            if ((s.value == 0) && (s.index == 0) && (s.length == 1)) {
                setup_tx(usb, &usb->config_value, 1);
                return;
            }
            break;
        case SETUP(DEVICE_WRITE, SET_CONFIGURATION):
            if (s.value == 1) {
                struct udc_endpoint *ept;
                /* enable endpoints */
                for (ept = usb->ept_list; ept; ept = ept->next) {
                    if (ept->num != 0) {
                        endpoint_enable(usb, ept, 1);
                    }
                }
                usb->config_value = 1;
                notify_gadgets(usb->gadget, UDC_EVENT_ONLINE);
            } else {
                writel(0, usb->base + USB_ENDPTCTRL(1));
                usb->config_value = 0;
                notify_gadgets(usb->gadget, UDC_EVENT_OFFLINE);
            }
            setup_ack(usb);
            usb->online = s.value ? 1 : 0;
            return;
        case SETUP(DEVICE_WRITE, SET_ADDRESS):
            // write address delayed (will take effect after the next IN txn)
            writel(((s.value & 0x7F) << 25) | (1 << 24), usb->base + USB_DEVICEADDR);
            setup_ack(usb);
            return;
        case SETUP(INTERFACE_WRITE, SET_INTERFACE):
            goto stall;
        case SETUP(ENDPOINT_WRITE, CLEAR_FEATURE): {
            udc_endpoint_t *ept;
            unsigned num = s.index & 15;
            unsigned in = !!(s.index & 0x80);

            if ((s.value != 0) || (s.length != 0)) {
                break;
            }
            DBG("clr feat %d %d\n", num, in);
            for (ept = usb->ept_list; ept; ept = ept->next) {
                if ((ept->num == num) && (ept->in == in)) {
                    endpoint_flush(usb, ept);
                    // todo: if callback requeues this could be ugly...
                    endpoint_reset(usb, ept);
                    setup_ack(usb);
                    return;
                }
            }
            break;
        }
    }

    dprintf(INFO, "udc: stall %02x %02x %04x %04x %04x\n",
            s.type, s.request, s.value, s.index, s.length);

stall:
    writel(EPCTRL_RXS | EPCTRL_TXS, usb->base + USB_ENDPTCTRL(0));
}

int lpc43xx_usb_init(u32 dmabase, size_t dmasize)
{
    usb_t *usb = &USB;
    printf("usb_init()\n");
    if ((dmabase & 0x7FF) || (dmasize < 1024)) {
        return -1;
    }
    usb->qh = (void *) dmabase;
    usb->dtd_freelist = NULL;
    memset(usb->qh, 0, dmasize);
    usb->base = USB0_BASE;
    dmabase += 768;
    dmasize -= 768;
    while (dmasize > sizeof(usb_dtd_t)) {
        usb_dtd_t *dtd = (void *) dmabase;
        dtd->next = usb->dtd_freelist;
        usb->dtd_freelist = dtd;
        dmabase += sizeof(usb_dtd_t);
        dmasize -= sizeof(usb_dtd_t);
    }
    writel(CMD_RST, usb->base + USB_CMD);
    while (readl(usb->base + USB_CMD) & CMD_RST) ;
    printf("usb_init(): reset ok\n");
    thread_sleep(250);

    // enable USB0 PHY via CREG0
    writel(readl(0x40043004) & (~0x20), 0x40043004);

    writel(MODE_DEVICE | MODE_SLOM, usb->base + USB_MODE);

    // enable termination in OTG control (required for device mode)
    writel(OTG_OT, usb->base + USB_OTGSC);

    writel((u32) usb->qh, usb->base + USB_ENDPOINTLISTADDR);
    usb->flags |= F_LL_INIT;
    return 0;
}

static void usb_enable(usb_t *usb, int yes)
{
    if (yes) {
        writel(INTR_UE | INTR_UEE | INTR_PCE | INTR_SEE | INTR_URE,
               usb->base + USB_INTR);

        writel(CMD_RUN, usb->base + USB_CMD);
        NVIC_EnableIRQ(USB0_IRQn);
    } else {
        NVIC_DisableIRQ(USB0_IRQn);
        writel(CMD_STOP, usb->base + USB_CMD);
    }
}


void lpc43xx_USB0_IRQ(void)
{
    udc_endpoint_t *ept;
    usb_t *usb = &USB;
    int ret = 0;
    unsigned n;

    arm_cm_irq_entry();

    n = readl(usb->base + USB_STS);
    writel(n, usb->base + USB_STS);

    if (n & STS_URI) {
        // reset procedure, per databook
        // 1. clear setup token semaphores
        writel(readl(usb->base + USB_ENDPTSETUPSTAT),
               usb->base + USB_ENDPTSETUPSTAT);
        // 2. clear completion status bits
        writel(readl(usb->base + USB_ENDPTCOMPLETE),
               usb->base + USB_ENDPTCOMPLETE);
        // 3. cancel primed transfers
        while (readl(usb->base + USB_ENDPTPRIME)) ;
        writel(0xFFFFFFFF, usb->base + USB_ENDPTFLUSH);
        // 4. ensure we finished while reset still active
        if (!(readl(usb->base + USB_PORTSC1) & PORTSC1_RC)) {
            printf("usb: failed to reset in time\n");
        }
        // 5. free active DTDs
        usb->online = 0;
        usb->config_value = 0;
        notify_gadgets(usb->gadget, UDC_EVENT_OFFLINE);
        for (ept = usb->ept_list; ept; ept = ept->next) {
            if (ept->req) {
                ept->req->dtd->config = DTD_HALTED;
                handle_ept_complete(ept);
            }
        }
    }
    if (n & STS_PCI) {
        unsigned x = readl(usb->base + USB_PORTSC1);
        usb->highspeed = (x & PORTSC1_HSP) ? 1 : 0;
    }
    if (n & (STS_UI | STS_UEI)) {
        if (readl(usb->base + USB_ENDPTSETUPSTAT) & 1) {
            handle_setup(usb);
        }
        n = readl(usb->base + USB_ENDPTCOMPLETE);
        writel(n, usb->base + USB_ENDPTCOMPLETE);

        for (ept = usb->ept_list; ept; ept = ept->next) {
            if (n & ept->bit) {
                handle_ept_complete(ept);
                ret = INT_RESCHEDULE;
            }
        }
    }
    if (n & STS_SEI) {
        panic("<SEI>");
    }
    arm_cm_irq_exit(ret);
}

// ---- UDC API

int udc_init(struct udc_device *dev)
{
    USB.device = dev;
    USB.ep0out = _udc_endpoint_alloc(&USB, 0, 0, 64);
    USB.ep0in = _udc_endpoint_alloc(&USB, 0, 1, 64);
    USB.ep0req = udc_request_alloc();
    USB.ep0req->context = &USB;
    USB.flags |= F_UDC_INIT;
    return 0;
}

int udc_register_gadget(udc_gadget_t *gadget)
{
    if (USB.gadget) {
        udc_gadget_t *last = USB.gadget;
        while (last->next) {
            last = last->next;
        }
        last->next = gadget;
    } else {
        USB.gadget = gadget;
    }
    gadget->next = NULL;
    return 0;
}

void udc_ept_desc_fill(udc_endpoint_t *ept, unsigned char *data)
{
    data[0] = 7;
    data[1] = TYPE_ENDPOINT;
    data[2] = ept->num | (ept->in ? 0x80 : 0x00);
    data[3] = 0x02; // bulk -- the only kind we support
    data[4] = ept->maxpkt;
    data[5] = ept->maxpkt >> 8;
    data[6] = ept->in ? 0x00 : 0x01;
}

int udc_start(void)
{
    usb_t *usb = &USB;

    dprintf(INFO, "udc_start()\n");
    if (!(usb->flags & F_LL_INIT)) {
        panic("udc cannot start before hw init\n");
    }
    if (!usb->device) {
        panic("udc cannot start before init\n");
    }
    if (!usb->gadget) {
        panic("udc has no gadget registered\n");
    }
    udc_create_descriptors(usb->device, usb->gadget);

    usb_enable(usb, 1);
    return 0;
}

int udc_stop(void)
{
    usb_enable(&USB, 0);
    thread_sleep(10);
    return 0;
}

