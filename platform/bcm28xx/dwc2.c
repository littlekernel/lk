#include <app.h>
#include <kernel/timer.h>
#include <lk/console_cmd.h>
#include <lk/reg.h>
#include <platform/bcm28xx.h>
#include <platform/bcm28xx/udelay.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define USB_BASE (BCM_PERIPH_BASE_VIRT + 0x980000)

#define USB_GAHBCFG   (USB_BASE + 0x0008)
#define USB_GINTSTS   (USB_BASE + 0x0014)
#define USB_GINTSTS_RXFLVL  (1 << 4)
#define USB_GRXSTSP   (USB_BASE + 0x0020)
#define USB_DCFG      (USB_BASE + 0x0800)
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

timer_t dwc2_poll;

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

STATIC_COMMAND_START
STATIC_COMMAND("dwc_dump", "dump dwc registers", &cmd_dwc_dump)
STATIC_COMMAND_END(dwc);

#define dumpreg(reg) { t = *REG32(reg); printf(#reg":\t 0x%x\n", t); }
#define GET_IN(epNr) ((endpoint_control*) ((USB_BASE + 0x0900) + (epNr) * 0x20))

static uint8_t packet_buffer[4096];

void ep_write_in_reply(int epNr, void *buffer, int bytes) {
  int maxPacketSize = 64;
  int fullPackets = bytes / maxPacketSize;
  int partialPacketSize = bytes - (fullPackets*maxPacketSize);
  int packets = fullPackets + ((partialPacketSize==0) ? 0 : 1);
  int words = ROUNDUP(bytes,4)/4;
  printf("sending %d full packets and a %d byte partial, %d total, %d words\n", fullPackets, partialPacketSize, packets, words);
  uint32_t x = (3 << 29) | (packets<<19) | bytes;
  printf("size is 0x%x\n", x);

  endpoint_control *ep = GET_IN(epNr);
  ep->size = x;
  ep->control = (1<<31) | // endpoint enable
    (1 << 26) | // clear nak
    (0 << 22) | // fifo#
    (1 << 15) | // USB active endpoint
    (0 & 0x3); // max packet size
  dump_endpoint(ep, true);
  ack_ep(ep);

  *REG32(USB_DFIFO0) = *((uint32_t*)buffer);
}

static void handle_incoming_setup(uint8_t *buf, int size) {
  setupData *s = (setupData*)buf;
  printf("bmRequestType: 0x%x\n", s->bmRequestType);
  printf("bRequest: 0x%x\n", s->bRequest);
  printf("wValue: 0x%x\n", s->wValue);
  printf("wIndex: 0x%x\n", s->wIndex);
  printf("wLength: 0x%x\n", s->wLength);
  if ((s->bmRequestType == 0xc0) && (s->wLength == 0x4)) {
    uint32_t a = 0x42;
    ep_write_in_reply(0, &a, 4);
    return;
  }
  if ((s->bmRequestType == 0xc0) && (s->wLength == 0x104)) {
    endpoint_control *ep = GET_IN(0);
    dump_endpoint(ep, true);
    ack_ep(ep);
    fileServerRequest req;
    memset(&req, 0, sizeof(req));
    req.cmd = 2;
    int fullPackets = sizeof(req) / 64;
    int partialPacket = sizeof(req) - (fullPackets*64);
    int packets = fullPackets + ((partialPacket == 0) ? 0 : 1);
    int words = ROUNDUP(sizeof(req),4)/4;
    printf("sending %d full packets and a %d byte partial, %d total, %d words\n", fullPackets, partialPacket, packets, words);

    uint32_t x = (3 << 29) | (packets<<19) | sizeof(req);
    printf("size is 0x%x\n", x);
    ep->size = x;

    for (int packetNr = 0; packetNr < packets; packetNr++) {
      ep->control = (1<<31) | // endpoint enable
        (1<<26) | // clear nak
        (1 << 22) | // fifo#
        (1 << 15) | // USB active endpoint
        (0 & 0x3); // max packet size
      dump_endpoint(ep, true);
      ack_ep(ep);

      uint32_t *packet = (uint32_t*)&req;
      int start = packetNr * (64/4);
      for (int i = start; i < MIN(start+16, words); i++) {
        *REG32(USB_DFIFO1) = packet[i];
        printf("%d: posted 0x%x to fifo\n", i, packet[i]);
      }

      dump_endpoint(ep, true);
      ack_ep(ep);
      udelay(5);
    }
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
  printf("ACK'd interrupt 0x%x\n", irq);
}

static void dwc_check_interrupt(void) {
  uint32_t interrupt_status = *REG32(USB_GINTSTS);
  while (interrupt_status & USB_GINTSTS_RXFLVL) {
    uint32_t receive_status = *REG32(USB_GRXSTSP);
    printf("USB_GRXSTSP:\t 0x%x\n", receive_status);
    printf("  CHEPNUM:    %d\n", receive_status & 0xf);
    int packet_size = (receive_status >> 4) & 0x7ff;
    printf("  BCNT:       %d\n", packet_size);
    printf("  DPID:       %d\n", (receive_status >> 15) & 0x3);
    int packet_status = (receive_status >> 17) & 0xf;
    printf("  PKTSTS:     %d %s\n", packet_status, packet_status_names[packet_status]);
    printf("  FN:         %d\n", (receive_status >> 21) & 0xf);

    int words = ROUNDUP(packet_size, 4) / 4;
    uint32_t *dest = (uint32_t*)packet_buffer;
    for (int i=0; i<words; i++) {
      dest[i] = *REG32(USB_DFIFO0);
    }
    for (int i=0; i < packet_size; i++) {
      printf("%02x ", packet_buffer[i]);
    }
    puts("");
    if (packet_status == 4) {
      handle_incoming_setup(packet_buffer, packet_size);
    }

    interrupt_status = *REG32(USB_GINTSTS);
  }
  uint32_t daint = *REG32(USB_DAINT);
  endpoint_control *ep = 0;
  if (daint != 0) {
    for (int i=0; i<16; i++) {
      if ((daint >> i) & 1) {
        printf("IN %d irq\n", i);
        ep = GET_IN(i);
        dump_endpoint(ep, true);
        uint32_t irq = ep->interrupt;
        printf("acking irq bits 0x%x\n", irq);
        ep->interrupt = irq;
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
  dumpreg(USB_DCFG);
  dumpreg(USB_DAINT);
  for (int i=0; i<16; i++) {
    printf("IN%d\n", i);
    ep = (endpoint_control*)((USB_BASE + 0x0900) + i * 0x20);
    dump_endpoint(ep, true);
  }
  for (int i=0; i<16; i++) {
    printf("OUT%d\n", i);
    ep = (endpoint_control*)((USB_BASE + 0x0b00) + i * 0x20);
    dump_endpoint(ep, false);
  }
  return 0;
}

static enum handler_return dwc2_poller(struct timer *unused1, unsigned int unused2,  void *unused3) {
  dwc_check_interrupt();
  return INT_NO_RESCHEDULE;
}

static void dwc2_init(const struct app_descriptor *app) {
  timer_initialize(&dwc2_poll);
}

static void dwc2_entry(const struct app_descriptor *app, void *args) {
  timer_set_periodic(&dwc2_poll, 100, dwc2_poller, NULL);
}

APP_START(dwc2)
  .init = dwc2_init,
  .entry = dwc2_entry,
APP_END
