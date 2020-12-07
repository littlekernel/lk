#include <app.h>
#include <assert.h>
#include <lk/console_cmd.h>
#include <lk/reg.h>
#include <platform.h>
#include <platform/bcm28xx.h>
#include <platform/bcm28xx/udelay.h>
#include <platform/interrupts.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define USB_BASE (BCM_PERIPH_BASE_VIRT + 0x980000)

#define USB_GAHBCFG   (USB_BASE + 0x0008)
#define USB_GINTSTS   (USB_BASE + 0x0014)
#define USB_GINTSTS_RXFLVL  (1 << 4)
#define USB_GINTMSK   (USB_BASE + 0x0018)
#define USB_GRXSTSP   (USB_BASE + 0x0020)
#define USB_GHWCFG1   (USB_BASE + 0x0044)
#define USB_GHWCFG2   (USB_BASE + 0x0048)
#define USB_GHWCFG3   (USB_BASE + 0x004c)
#define USB_GHWCFG4   (USB_BASE + 0x0050)
#define USB_DCFG      (USB_BASE + 0x0800)
#define USB_DIEPMSK   (USB_BASE + 0x0810)
#define USB_DAINT     (USB_BASE + 0x0818)

#define USB_DIEPCTL0  (USB_BASE + 0x0900)
#define USB_DIEPINT0  (USB_BASE + 0x0908)

#define USB_DIEPCTL1  (USB_BASE + 0x0920)
#define USB_DIEPINT1  (USB_BASE + 0x0928)

#define USB_DIEPCTL2  (USB_BASE + 0x0940)

#define USB_DOEPINT0  (USB_BASE + 0x0b08)
#define USB_DOEPINT1  (USB_BASE + 0x0b28)
#define USB_DOEPINT2  (USB_BASE + 0x0b48)
#define USB_DOEPINT3  (USB_BASE + 0x0b68)
#define USB_DOEPINT4  (USB_BASE + 0x0b88)
#define USB_DOEPINT5  (USB_BASE + 0x0ba8)
#define USB_DOEPINT6  (USB_BASE + 0x0bc8)
#define USB_DOEPINT7  (USB_BASE + 0x0be8)
#define USB_DOEPINT8  (USB_BASE + 0x0c08)
#define USB_DOEPINT9  (USB_BASE + 0x0c28)

#define USB_DFIFO0    (USB_BASE + 0x1000)
#define USB_DFIFO1    (USB_BASE + 0x2000)

#define DWC_IRQ       9

typedef struct {
  volatile uint32_t control;    //  0
  uint32_t pad1;                //  4
  volatile uint32_t interrupt;  //  8
  uint32_t pad2;                //  c
  volatile uint32_t size;       // 10
  volatile uint32_t dma;        // 14
  volatile uint32_t fifo_status;// 18   number of 32bit words that are free in the fifo
} endpoint_control;

typedef struct {
  uint8_t bmRequestType;
  uint8_t bRequest;
  uint16_t wValue;
  uint16_t wIndex;
  uint16_t wLength;
} setupData;

typedef struct {
  uint32_t cmd;
  char name[256];
} fileServerRequest;

enum usb_stream_type {
  USB_STREAM_SETUP
};

struct dwc_state_T;

typedef struct packet_queue_T {
  struct packet_queue_T *next;
  void *payload;
  int payload_size;
  int start;
  void (*cb)(struct dwc_state_T *, struct packet_queue_T *);
} packet_queue_t;

typedef struct {
  packet_queue_t *packet_queue_head;
  packet_queue_t *packet_queue_tail;
} endpoint_t;

typedef struct {
  endpoint_t in[1];
  endpoint_t out[1];
} dwc_state_t;

const char *packet_status_names[] = {
  [1] = "GOUTNAK",
  [2] = "OUT data packet received",
  [3] = "XFERCOMPL",
  [4] = "SETUPCOMPL",
  [5] = "TGLERR",
  [6] = "SETUP packet received",
  [7] = "CHLT"
};

static int cmd_dwc_dump(int argc, const cmd_args *argv);
static void dump_endpoint(endpoint_control *ep, bool in);
static void ack_ep(endpoint_control *ep);
void ep_write_in(dwc_state_t *state, int epNr);

STATIC_COMMAND_START
STATIC_COMMAND("dwc_dump", "dump dwc registers", &cmd_dwc_dump)
STATIC_COMMAND_END(dwc);

#define dumpreg(reg) { t = *REG32(reg); printf(#reg":\t 0x%x\n", t); }
#define GET_IN(epNr) ((endpoint_control*) ((USB_BASE + 0x0900) + (epNr) * 0x20))
#define GET_OUT(epNr) ((endpoint_control*) ((USB_BASE + 0x0b00) + (epNr) * 0x20))

static uint8_t packet_buffer[4096];

static void logmsg(const char *msg) {
  printf("%4d: %s\n", *REG32(ST_CLO), msg);
}

void dwc2_ep_queue_in(dwc_state_t *state, int epNr, void *buffer, int bytes, void (*cb)(dwc_state_t *, packet_queue_t *)) {
  assert(epNr <= 0);
  packet_queue_t *pkt = malloc(sizeof(packet_queue_t));
  pkt->payload = buffer;
  pkt->payload_size = bytes;
  pkt->start = 0;
  pkt->next = NULL;
  pkt->cb = cb;

  uint32_t *data = buffer;
  for (int i=0; i<(bytes/4); i++) {
    printf("0x%x ", data[i]);
  }
  puts("");

  int do_tx = false;
  if (state->in[epNr].packet_queue_head == NULL) {
    // no pending packets, can tx right away
    do_tx = true;
    state->in[epNr].packet_queue_head = pkt;
  }
  if (state->in[epNr].packet_queue_tail) {
    state->in[epNr].packet_queue_tail->next = pkt;
  }
  state->in[epNr].packet_queue_tail = pkt;
  if (do_tx) {
    ep_write_in(state, epNr);
  }
}

void ep_write_in(dwc_state_t *state, int epNr) {
  endpoint_control *ep = GET_IN(epNr);
  while (ep->control & (1<<31))  {}
  puts("ready");
  int maxPacketSize = 64; // TODO
  packet_queue_t *pkt = state->in[epNr].packet_queue_head;
  assert(pkt);
  printf("packet(%x): 0x%x+(0x%x/0x%x)\n", (uint32_t)pkt, (uint32_t)pkt->payload, pkt->start, pkt->payload_size);
  int bytes = pkt->payload_size - pkt->start;
  if (bytes == 0) {
    puts("packet sent");
    pkt->cb(state, pkt);
    packet_queue_t *next = pkt->next;
    state->in[epNr].packet_queue_head = next;
    if (state->in[epNr].packet_queue_tail == pkt) state->in[epNr].packet_queue_tail = NULL;
    if (next) return ep_write_in(state, epNr);
    return;
  }
  assert(bytes > 0);
  int fullPackets = bytes / maxPacketSize;
  int partialPacketSize = bytes - (fullPackets*maxPacketSize);
  int packets = fullPackets + ((partialPacketSize==0) ? 0 : 1);
  int words = ROUNDUP(bytes,4)/4;
  printf("sending %d full packets and a %d byte partial, %d total, %d words\n", fullPackets, partialPacketSize, packets, words);
  if (epNr == 0) {
    packets = 1;
    bytes = MIN(maxPacketSize, bytes);
    printf("capped to 1 packet of %d bytes\n", bytes);
  }
  uint32_t x = (3 << 29) | (packets<<19) | bytes;
  printf("size is 0x%x\n", x);

  ep->size = x;
  ep->control = (1<<31) | // endpoint enable
    (1 << 26) | // clear nak
    (0 << 22) | // fifo#
    (1 << 15) | // USB active endpoint
    (0 & 0x3); // max packet size
  dump_endpoint(ep, true);
  //ack_ep(ep);

  uint32_t *packet = (uint32_t*)(pkt->payload + pkt->start);
  printf("packet: 0x%x\n", (uint32_t)packet);
  int bytes_sent = 0;
  for (int i = 0; i < MIN(maxPacketSize/4, words); i++) {
    *REG32(USB_DFIFO0) = packet[i];
    //printf("%d(0x%x): posted 0x%x to fifo\n", i, &packet[i], packet[i]);
    bytes_sent += 4;
  }
  pkt->start += bytes_sent;
  printf("%d bytes sent, start now %d\n", bytes_sent, pkt->start);
}

static fileServerRequest req; // TODO, put on heap

static void handle_setup_status_out(dwc_state_t *state, packet_queue_t *pkt) {
  endpoint_control *ep_in = GET_IN(0);
  endpoint_control *ep_out = GET_OUT(0);
  puts("setup data IN stage done, on to status OUT...");
  ep_in->size = (3 << 29);

  //puts("\nOUT 0");
  //dump_endpoint(ep_out, false);
  ep_out->control |= (1<<31) | // enable OUT0
      (1<<26); // clear NAK

  //puts("IN 0");
  //dump_endpoint(ep, true);
  ack_ep(ep_in);
  //udelay(5);
  //puts("\nIN 0");
  //dump_endpoint(ep, true);
  ack_ep(ep_in);
  //udelay(5);

  //puts("\nOUT 0");
  //dump_endpoint(ep_out, false);
  ack_ep(ep_out);
  //dump_endpoint(ep_out, false);
}

static void handle_incoming_setup(dwc_state_t *state, uint8_t *buf, int size) {
  setupData *s = (setupData*)buf;
  //printf("bmRequestType: 0x%x\n", s->bmRequestType);
  //printf("bRequest: 0x%x\n", s->bRequest);
  //printf("wValue: 0x%x\n", s->wValue);
  //printf("wIndex: 0x%x\n", s->wIndex);
  //printf("wLength: 0x%x\n", s->wLength);
  if ((s->bmRequestType == 0xc0) && (s->wLength == 0x104)) {
    endpoint_control *ep = GET_IN(0);
    endpoint_control *ep_out = GET_OUT(0);
    //dump_endpoint(ep, true);
    ack_ep(ep);
    ack_ep(ep_out);
    memset(&req, 0, sizeof(req));
    req.cmd = 2;
    dwc2_ep_queue_in(state, 0, &req, sizeof(req), &handle_setup_status_out);
    return;
    int fullPackets = sizeof(req) / 64;
    int partialPacket = sizeof(req) - (fullPackets*64);
    int packets = fullPackets + ((partialPacket == 0) ? 0 : 1);
    int words = ROUNDUP(sizeof(req),4)/4;
    //printf("sending %d full packets and a %d byte partial, %d total, %d words\n", fullPackets, partialPacket, packets, words);

    //uint32_t x = (3 << 29) | (packets<<19) | sizeof(req);
    //printf("size is 0x%x\n", x);

    int bytes_sent = 0;
    for (int packetNr = 0; packetNr < packets; packetNr++) {
      int bytes_this_packet = MIN(64, sizeof(req) - bytes_sent);
      //printf("bytes_this_packet: %d\n", bytes_this_packet);
      ep->size = (1<<19) | bytes_this_packet;
      ep->control = (1<<31) | // endpoint enable
        (1<<26) | // clear nak
        (0 << 22) | // fifo#
        (1 << 15) | // USB active endpoint
        (0 & 0x3); // max packet size
      // TODO, r/m/w the control and only set enable
      //dump_endpoint(ep, true);
      ack_ep(ep);

      uint32_t *packet = (uint32_t*)&req;
      int start = packetNr * (64/4);
      for (int i = start; i < MIN(start+16, words); i++) {
        *REG32(USB_DFIFO0) = packet[i];
        //printf("%d: posted 0x%x to fifo\n", i, packet[i]);
        bytes_sent += 4;
      }

      //dump_endpoint(ep, true);
      //int t_0 = *REG32(ST_CLO);
      while (ep->control & (1<<31))  {}
      ack_ep(ep);
      //int t_1 = *REG32(ST_CLO);
      //printf("waited %d usec and read %d times\n", t_1 - t_0, cycles);
    }
    //puts("setup data IN stage done, on to status OUT...");
    ep->size = (3 << 29);

    //puts("\nOUT 0");
    //dump_endpoint(ep_out, false);
    ep_out->control |= (1<<31) | // enable OUT0
        (1<<26); // clear NAK

    //puts("IN 0");
    //dump_endpoint(ep, true);
    ack_ep(ep);
    //udelay(5);
    //puts("\nIN 0");
    //dump_endpoint(ep, true);
    ack_ep(ep);
    //udelay(5);

    //puts("\nOUT 0");
    //dump_endpoint(ep_out, false);
    ack_ep(ep_out);
    //dump_endpoint(ep_out, false);
  }
}

static void dump_endpoint(endpoint_control *ep, bool in) {
  if (ep->control) {
    uint32_t ctl = ep->control;
    printf("     CTL: 0x%08x\n", ctl);
    if ((ctl >> 15) & 1) puts("       Active");
    if ((ctl >> 17) & 1) puts("       NAK'ing");
    if ((ctl >> 31) & 1) puts("       Enabled");
    uint32_t irq = ep->interrupt;
    printf("     INT: 0x%08x\n", irq);
    if (irq & 1)          puts("       XFERCOMPL");
    if ((irq >> 4) & 1)   puts("       INTKNTXFEMP IN Token Received When TxFIFO is Empty");
    if ((irq >> 6) & 1)   puts("       INEPNAKEFF  IN Endpoint NAK Effective");
    if ((irq >> 13) & 1)  puts("       NAKINTRPT   NAK Interrupt");
    if (in) {
      if ((irq >> 3) & 1) puts("       TIMEOUT");
      if ((irq >> 7) & 1) puts("       TX FIFO empty");
    } else {
      if ((irq >> 3) & 1) puts("       SETUP");
    }
    printf("    TSIZ: 0x%08x\n", ep->size);
    printf("  TXFSTS: 0x%08x\n", ep->fifo_status);
  }
}

static void ack_ep(endpoint_control *ep) {
  uint32_t irq = ep->interrupt;
  ep->interrupt = irq;
  //printf("ACK'd interrupt 0x%x\n", irq);
}

static void dwc_check_interrupt(dwc_state_t *state) {
  uint32_t interrupt_status = *REG32(USB_GINTSTS);
  while (interrupt_status & USB_GINTSTS_RXFLVL) {
    uint32_t receive_status = *REG32(USB_GRXSTSP);
    //printf("USB_GRXSTSP:\t 0x%x\n", receive_status);
    //printf("  CHEPNUM:    %d\n", receive_status & 0xf);
    int packet_size = (receive_status >> 4) & 0x7ff;
    //printf("  BCNT:       %d\n", packet_size);
    //printf("  DPID:       %d\n", (receive_status >> 15) & 0x3);
    int packet_status = (receive_status >> 17) & 0xf;
    //printf("  PKTSTS:     %d %s\n", packet_status, packet_status_names[packet_status]);
    //printf("  FN:         %d\n", (receive_status >> 21) & 0xf);

    int words = ROUNDUP(packet_size, 4) / 4;
    uint32_t *dest = (uint32_t*)packet_buffer;
    for (int i=0; i<words; i++) {
      dest[i] = *REG32(USB_DFIFO0);
    }
    //for (int i=0; i < packet_size; i++) {
    //  printf("%02x ", packet_buffer[i]);
    //}
    //puts("");
    if (packet_status == 4) {
      handle_incoming_setup(state, packet_buffer, packet_size);
    }

    interrupt_status = *REG32(USB_GINTSTS);
  }
  logmsg(".");
  uint32_t daint = *REG32(USB_DAINT);
  endpoint_control *ep = 0;
  while (daint != 0) {
    printf("DAINT: 0x%x\n", daint);
    for (int i=0; i<16; i++) {
      if ((daint >> i) & 1) {
        printf("IN %d irq\n", i);
        ep = GET_IN(i);
        dump_endpoint(ep, true);
        uint32_t irq = ep->interrupt;
        printf("acking irq bits 0x%x\n", irq);
        ep->interrupt = irq;
        if (irq & 1) {
          ep_write_in(state, i);
        }
      }
    }
    for (int i=16; i<32; i++) {
      if ((daint >> i) & 1) {
        ep = (endpoint_control*)((USB_BASE + 0x0b00) + (i-16) * 0x20);
        printf("OUT %d irq\n", i - 16);
        dump_endpoint(ep, false);
        uint32_t irq = ep->interrupt;
        printf("acking irq bits 0x%x\n", irq);
        ep->interrupt = irq;
      }
    }
    daint = *REG32(USB_DAINT);
  }
}

static int cmd_dwc_dump(int argc, const cmd_args *argv) {
  uint32_t t;
  endpoint_control *ep = 0;

  dumpreg(USB_GAHBCFG);
  dumpreg(USB_GINTSTS);
  printf("  CURMOD:     %d\n", t & 1);
  printf("  MODEMIS:    %d\n", (t >> 1) & 1);
  printf("  OTGINT:     %d\n", (t >> 2) & 1);
  printf("  SOF:        %d\n", (t >> 3) & 1);
  printf("  RXFLVL:     %d\n", (t >> 4) & 1);
  printf("  NPTXFEMP:   %d\n", (t >> 5) & 1);
  printf("  GINNAKEFF:  %d\n", (t >> 6) & 1);
  printf("  GOUTNAKEFF: %d\n", (t >> 7) & 1);
  printf("  unused:     %d\n", (t >> 8) & 0x3);
  printf("  ERLYSUSP:   %d\n", (t >> 10) & 1);
  printf("  USBSUSP:    %d\n", (t >> 11) & 1);
  printf("  USBRST:     %d\n", (t >> 12) & 1);
  printf("  ENUMDONE:   %d\n", (t >> 13) & 1);
  printf("  ISOOUTDROP: %d\n", (t >> 14) & 1);
  printf("  EOPF:       %d\n", (t >> 15) & 1);
  printf("  unparsed:   %d\n", (t >> 16) & 0xffff);
  dumpreg(USB_GINTMSK);
  dumpreg(USB_GHWCFG1);
  dumpreg(USB_GHWCFG2);
  dumpreg(USB_GHWCFG3);
  dumpreg(USB_GHWCFG4);
  dumpreg(USB_DCFG);      // 800
  dumpreg(USB_DIEPMSK);   // 810
  dumpreg(USB_DAINT);
  for (int i=0; i<3; i++) {
    printf("IN%d\n", i);
    ep = (endpoint_control*)((USB_BASE + 0x0900) + i * 0x20);
    dump_endpoint(ep, true);
  }
  for (int i=0; i<3; i++) {
    printf("OUT%d\n", i);
    ep = (endpoint_control*)((USB_BASE + 0x0b00) + i * 0x20);
    dump_endpoint(ep, false);
  }
  return 0;
}

static enum handler_return dwc_irq(dwc_state_t *state) {
  lk_bigtime_t start = current_time_hires();
  logmsg("dwc irq");
  dwc_check_interrupt(state);
  lk_bigtime_t end = current_time_hires();
  lk_bigtime_t spent = end - start;
  logmsg("done");
  printf("irq time: %lld\n", spent);
  return INT_NO_RESCHEDULE;
}

static dwc_state_t dwc_state;

static void dwc2_init(const struct app_descriptor *app) {
  memset(&dwc_state, 0, sizeof(dwc_state));
  register_int_handler(DWC_IRQ, dwc_irq, &dwc_state);
  unmask_interrupt(DWC_IRQ);

  *REG32(USB_GINTMSK) = (1<<4);
  *REG32(USB_DIEPMSK) = (1<<4) | (1<<3) | (1<<0);
}

static void dwc2_entry(const struct app_descriptor *app, void *args) {
}

APP_START(dwc2)
  .init = dwc2_init,
  .entry = dwc2_entry,
APP_END
