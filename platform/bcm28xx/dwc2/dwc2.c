#include <app.h>
#include <assert.h>
#include <lk/console_cmd.h>
#include <lk/reg.h>
#include <platform.h>
#include <platform/bcm28xx.h>
#include <platform/bcm28xx/dwc2.h>
#include <platform/bcm28xx/pll.h>
#include <platform/bcm28xx/pm.h>
#include <platform/bcm28xx/power.h>
#include <platform/bcm28xx/udelay.h>
#include <platform/interrupts.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define logf(fmt, ...) { print_timestamp(); printf("[DWC2:%s]: "fmt, __FUNCTION__, ##__VA_ARGS__); }

static void dwc_mdio_write(uint8_t reg, uint16_t val) {
  //printf("usb MDIO 0x%x <- 0x%x\n", reg, val);
  *REG32(USB_GMDIOGEN) = 0xffffffff;
  while ((*REG32(USB_GMDIOCSR) & 0x80000000) != 0) {}

  *REG32(USB_GMDIOGEN) = (reg & 0x1f) << 18 | 0x50020000 | val;
  while ((*REG32(USB_GMDIOCSR) & 0x80000000) != 0) {}

  *REG32(USB_GMDIOGEN) = 0;
  while ((*REG32(USB_GMDIOCSR) & 0x80000000) != 0) {}
}

static uint16_t dwc_mdio_read(uint8_t reg) {
  *REG32(USB_GMDIOGEN) = 0xffffffff;
  while ((*REG32(USB_GMDIOCSR) & 0x80000000) != 0) {}

  *REG32(USB_GMDIOGEN) = (reg & 0x1f) << 18 | 0x60020000;
  while ((*REG32(USB_GMDIOCSR) & 0x80000000) != 0) {}

  *REG32(USB_GMDIOGEN) = 0;
  while ((*REG32(USB_GMDIOCSR) & 0x80000000) != 0) {}

  uint16_t val = *REG32(USB_GMDIOCSR) & 0xffff;
  //printf("usb MDIO 0x%x -> 0x%x\n", reg, val);
  return val;
}

static void dwc_configure_phy(void) {
  puts("phy configure");
  *REG32(USB_GMDIOCSR) = 0x40000;
  dwc_mdio_write(0x19, 4);
  dwc_mdio_write(0x18, 0x342);
  dwc_mdio_write(0x1d, 4);
  dwc_mdio_write(0x15, 0x111);

  int a, b;
  if (true) {
    // "fast" crystal (19.2mhz)
    a = 0x32;
    b = 1;
  } else {
    // "slow" crystal
    a = 0x140;
    b = 9;
  }
  dwc_mdio_write(0x17, b << 12 | 0x600 | a);
  while ((dwc_mdio_read(0x1b) & 0x80) == 0) {}
  *REG32(USB_GVBUSDRV) &= 0xffffff7f;

  dwc_mdio_write(0x1e, 0x8000);
  dwc_mdio_write(0x1d, 0x4000);
  dwc_mdio_write(0x19, 0xc004);
  dwc_mdio_write(0x20, 0x1c2f);
  dwc_mdio_write(0x22, 0x100);
  dwc_mdio_write(0x24, 0x10);
}

static void dwc_start(void) {
  puts("usb reset");

  // core soft reset
  *REG32(USB_GRSTCTL) = BIT(0);
  while ((*REG32(USB_GRSTCTL) & BIT(0)) != 0) {}

  // hclk soft reset
  *REG32(USB_GRSTCTL) = BIT(1);
  while ((*REG32(USB_GRSTCTL) & BIT(1)) != 0) {}

  // flush tx fifo 1
  *REG32(USB_GRSTCTL) = 0x420;
  while ((*REG32(USB_GRSTCTL) & 0x20) != 0) {}

  // flush rx fifo
  *REG32(USB_GRSTCTL) = 0x10;
  while ((*REG32(USB_GRSTCTL) & 0x10) != 0) {}

  *REG32(USB_GAHBCFG) = BIT(0);
  *REG32(USB_GUSBCFG) = 0x40402740;
  *REG32(USB_DOEPCTL0) = 0x80000000;
  *REG32(USB_DIEPCTL0) = 0x80000000;
}

static void dwc_clear_fifo(void) {
  volatile uint32_t *reg = REG32(USB_DFIFO0);
  for (int i=0; i<0x1000; i++) {
    reg[i] = 0;
  }

  uint32_t config3 = *REG32(USB_GHWCFG3);
  uint32_t a = config3 >> 16;
  uint32_t b = a;
  while (b = b>>1, b != 0) {
    a |= b;
  }
  uint32_t c = a+1;
  reg = REG32(0x7e9a0000);
  for (unsigned int i=0; i<c; i++) {
    reg[i] = 0;
  }
}

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
  uint8_t bmRequestType;
  uint8_t bRequest;
  uint8_t bDescriptorIndex;
  uint8_t bDescriptorType;
  uint16_t wLanguageId;
  uint16_t wLength;
} getDescriptorRequest;

typedef struct {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint16_t bcdUSB;
  uint8_t bDeviceClass;
  uint8_t bDeviceSubClass;
  uint8_t bDeviceProtocol;
  uint8_t bMaxPacketSize0;
  uint16_t idVendor;
  uint16_t idProduct;
  uint16_t bcdDevice;
  uint8_t iManufacturer;
  uint8_t iProduct;
  uint8_t iSerialNumber;
  uint8_t bNumberConfigurations;
} deviceDescriptor;

typedef struct {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint16_t wTotalLength;
  uint8_t bNumberInterfaces;
  uint8_t bConfigurationValue;
  uint8_t iConfiguration;
  uint8_t bmAttributes;
  uint8_t bMaxPower;
} __attribute__((packed)) configurationDescriptor;

typedef struct {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint8_t bInterfaceNumber;
  uint8_t bAlternateSetting;
  uint8_t bNumEndpoints;
  uint8_t bInterfaceClass;
  uint8_t bInterfaceSubClass;
  uint8_t bInterfaceProtocol;
  uint8_t iInterface;
} __attribute__((packed)) interfaceDescriptor;

typedef struct {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint8_t bEndpointAddress;
  uint8_t bmAttributes;
  uint16_t wMaxPacketSize;
  uint8_t bInterval;
} __attribute__((packed)) endpointDescriptor;

deviceDescriptor defaultDeviceDescriptor = {
  .bLength = sizeof(deviceDescriptor),
  .bDescriptorType = 1,
  .bcdUSB = 0x0110,
  .bDeviceClass = 0,
  .bDeviceSubClass = 0,
  .bDeviceProtocol = 0,
  .bMaxPacketSize0 = 64,
  .idVendor = 0x0000,
  .idProduct = 0x0000,
  .bcdDevice = 0,
  .iManufacturer = 1,
  .iProduct = 2,
  .iSerialNumber = 0,
  .bNumberConfigurations = 1
};

configurationDescriptor defaultConfigurationDescriptor = {
  .bLength = sizeof(configurationDescriptor),
  .bDescriptorType = 2,
  .wTotalLength = 32,
  .bNumberInterfaces = 1,
  .bConfigurationValue = 1,
  .iConfiguration = 0,
  .bmAttributes = 0xc0,
  .bMaxPower = 50
};

interfaceDescriptor defaultInterfaceDescriptor = {
  .bLength = sizeof(interfaceDescriptor),
  .bDescriptorType = 4,
  .bInterfaceNumber = 0,
  .bAlternateSetting = 0,
  .bNumEndpoints = 2,
  .bInterfaceClass = 0xff,
  .bInterfaceSubClass = 0,
  .bInterfaceProtocol = 0,
  .iInterface = 0
};

endpointDescriptor ep0Descriptor = {
  .bLength = sizeof(endpointDescriptor),
  .bDescriptorType = 5,
  .bEndpointAddress = 1,
  .bmAttributes = 2,
  .wMaxPacketSize = 64,
  .bInterval = 0
};

endpointDescriptor ep1Descriptor = {
  .bLength = sizeof(endpointDescriptor),
  .bDescriptorType = 5,
  .bEndpointAddress = 0x82,
  .bmAttributes = 2,
  .wMaxPacketSize = 64,
  .bInterval = 0
};

typedef struct {
  uint32_t cmd;
  char name[256];
} fileServerRequest;

enum usb_stream_type {
  USB_STREAM_SETUP
};

struct dwc_state_t;

typedef struct packet_queue_T {
  struct packet_queue_T *next;
  void *payload;
  int payload_size;
  int start;
  void (*cb)(struct dwc_state_t *, struct packet_queue_T *);
  bool has_0byte_tail;
} packet_queue_t;

typedef struct {
  packet_queue_t *packet_queue_head;
  packet_queue_t *packet_queue_tail;
} endpoint_t;

struct dwc_state_t {
  endpoint_t in[1];
  endpoint_t out[1];
  void *mainConfigurationDescriptor;
  int mainConfigurationDescriptorSize;
};

static const wchar_t *strings[] = {
  [1] = L"string 1",
  [2] = L"second string"
};

// FIXME
static int string_lengths[] = {
  [1] = 16,
  [2] = 26
};

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
void ep_write_in(struct dwc_state_t *state, int epNr);

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

void dwc2_ep_queue_in(struct dwc_state_t *state, int epNr, void *buffer, int bytes, void (*cb)(struct dwc_state_t *, packet_queue_t *)) {
  assert(epNr <= 0);
  packet_queue_t *pkt = malloc(sizeof(packet_queue_t));
  pkt->payload = buffer;
  pkt->payload_size = bytes;
  pkt->start = 0;
  pkt->next = NULL;
  pkt->cb = cb;
  pkt->has_0byte_tail = (bytes % 64) == 0;

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

void ep_write_in(struct dwc_state_t *state, int epNr) {
  endpoint_control *ep = GET_IN(epNr);
  //printf("control: 0x%x\n", ep->control);
  while (ep->control & (1<<31))  {}
  int maxPacketSize = 64; // TODO
  packet_queue_t *pkt = state->in[epNr].packet_queue_head;
  assert(pkt);
  //printf("packet(%x): 0x%x+(0x%x/0x%x)\n", (uint32_t)pkt, (uint32_t)pkt->payload, pkt->start, pkt->payload_size);
  int bytes = pkt->payload_size - pkt->start;
  if ((bytes <= 0) && !pkt->has_0byte_tail) {
    //puts("IN packet sent");
    pkt->cb(state, pkt);
    packet_queue_t *next = pkt->next;
    state->in[epNr].packet_queue_head = next;
    if (state->in[epNr].packet_queue_tail == pkt) state->in[epNr].packet_queue_tail = NULL;
    free(pkt);
    if (next) return ep_write_in(state, epNr);
    return;
  }
  assert(bytes >= 0);
  int fullPackets = bytes / maxPacketSize;
  int partialPacketSize = bytes - (fullPackets*maxPacketSize);
  int packets = fullPackets + ((partialPacketSize==0) ? 0 : 1);
  int words = ROUNDUP(bytes,4)/4;
  //printf("sending %d full packets and a %d byte partial, %d total, %d words\n", fullPackets, partialPacketSize, packets, words);
  if (bytes == 0) pkt->has_0byte_tail = false;
  if (epNr == 0) {
    packets = 1;
    bytes = MIN(maxPacketSize, bytes);
    //printf("capped to 1 packet of %d bytes\n", bytes);
  }
  uint32_t x = (3 << 29) | (packets<<19) | bytes;

  ep->size = x;
  ep->control = (1<<31) | // endpoint enable
    (1 << 26) | // clear nak
    (0 << 22) | // fifo#
    (1 << 15) | // USB active endpoint
    (0 & 0x3); // max packet size
  //dump_endpoint(ep, true);
  //ack_ep(ep);

  uint32_t *packet = (uint32_t*)(pkt->payload + pkt->start);
  int bytes_sent = 0;
  for (int i = 0; i < MIN(maxPacketSize/4, words); i++) {
    *REG32(USB_DFIFO0) = packet[i];
    //printf("%d: posted 0x%08x to fifo\n", i, packet[i]);
    bytes_sent += 4;
  }
  pkt->start += bytes_sent;
  //printf("%d bytes sent, start now %d\n", bytes_sent, pkt->start);
}

static fileServerRequest req; // TODO, put on heap

static void handle_setup_status_out(struct dwc_state_t *state, packet_queue_t *pkt) {
  endpoint_control *ep_in = GET_IN(0);
  endpoint_control *ep_out = GET_OUT(0);
  //puts("setup data IN stage done, on to status OUT...");
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

void dwc2_out_cb_free(struct dwc_state_t *state, packet_queue_t *pkt) {
  free(pkt->payload);
  //puts("nop");
  endpoint_control *ep_out = GET_OUT(0);
  ep_out->control |= (1<<31) | // enable OUT0
      (1<<26); // clear NAK
}

void dwc2_out_cb(struct dwc_state_t *state, packet_queue_t *pkt) {
  //puts("nop");
  endpoint_control *ep_out = GET_OUT(0);
  ep_out->control |= (1<<31) | // enable OUT0
      (1<<26); // clear NAK
}

void dwc2_in_cb(struct dwc_state_t *state, packet_queue_t *pkt) {
  //puts("in done");
}

static void handle_incoming_setup(struct dwc_state_t *state, uint8_t *buf, int size) {
  setupData *s = (setupData*)buf;
  bool dump = false;
  if ((s->bmRequestType == 0x00) && (s->bRequest == 5)) { // set address
    *REG32(USB_DCFG) = ((s->wValue & 0x3f) << 4) | (*REG32(USB_DCFG) & 0xfffff80f);
    dwc2_ep_queue_in(state, 0, NULL, 0, &dwc2_in_cb);
    logf("i am device %d\n", s->wValue);
  } else if ((s->bmRequestType == 0x00) && (s->bRequest == 9)) { // set configuration
    dwc2_ep_queue_in(state, 0, NULL, 0, &dwc2_in_cb);
    logf("set configuration\n");
  } else if ((s->bmRequestType == 0xc0) && (s->wLength == 0x104)) {
    endpoint_control *ep_in = GET_IN(0);
    endpoint_control *ep_out = GET_OUT(0);
    //dump_endpoint(ep, true);
    ack_ep(ep_in);
    ack_ep(ep_out);
    memset(&req, 0, sizeof(req));
    req.cmd = 2;
    dwc2_ep_queue_in(state, 0, &req, sizeof(req), &handle_setup_status_out);
  } else if (s->bmRequestType == 0x80) { // control-in
    //puts("\n\n");
    switch (s->bRequest) {
    case 0: // GET STATUS
      puts("GET STATUS");
      break;
    case 6: { // GET DESCRIPTOR
      //puts("GET DESCRIPTOR");
      getDescriptorRequest *req = (getDescriptorRequest*)buf;
      switch (req->bDescriptorType) {
      case 1: // device descriptor
        {
          int size = sizeof(defaultDeviceDescriptor);
          if (size > req->wLength) size = req->wLength;
          dwc2_ep_queue_in(state, 0, &defaultDeviceDescriptor, size, &dwc2_out_cb);
        }
        break;
      case 2: // configuration descriptor
        {
          int size = state->mainConfigurationDescriptorSize;
          if (size > req->wLength) size = req->wLength;
          dwc2_ep_queue_in(state, 0, state->mainConfigurationDescriptor, size, &dwc2_out_cb);
        }
        break;
      case 3: // string descriptor
        switch (req->wLanguageId) { // language id
        case 0: {
          uint8_t *reply = malloc(4);
          reply[0] = 4; // length
          reply[1] = 3; // type string
          reply[2] = 0x09;
          reply[3] = 0x04; // english us
          dwc2_ep_queue_in(state, 0, reply, 4, &dwc2_out_cb_free);
          break;
        }
        case 0x0409:{
          int string_index = req->bDescriptorIndex;
          if ((string_index >= 1) && (string_index <= 2)) {
            uint8_t *reply = malloc(string_lengths[string_index] + 2);
            reply[0] = string_lengths[string_index] + 2;
            reply[1] = 3;
            memcpy(reply+2, strings[string_index], string_lengths[string_index]);
            dwc2_ep_queue_in(state, 0, reply, string_lengths[string_index] + 2, &dwc2_out_cb_free);
          }
          break;
        }
        }
        break;
      }
      break;
    }
    default:
      dump = true;
    }
  } else {
    dump = true;
  }
  if (dump) {
    printf("bmRequestType: 0x%x\n", s->bmRequestType);
    printf("bRequest: 0x%x\n", s->bRequest);
    printf("wValue: 0x%x\n", s->wValue);
    printf("wIndex: 0x%x\n", s->wIndex);
    printf("wLength: 0x%x\n", s->wLength);
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

static void dwc_check_interrupt(struct dwc_state_t *state) {
  uint32_t interrupt_status = *REG32(USB_GINTSTS);
  //printf("irq: 0x%x\n", interrupt_status & *REG32(USB_GINTMSK));
  if (interrupt_status & BIT(13)) { // enumeration done
    *REG32(USB_GINTSTS) = BIT(13);
    logf("enumeration done\n");
    GET_OUT(0)->size = (1 << 29) | // 1 setup
            (2<<18) | // packet count
            8; // bytes
    GET_OUT(0)->control = BIT(31) | BIT(26) | BIT(15); // enable and active
    //dumpreg(USB_DSTS);      // 808
    interrupt_status = *REG32(USB_GINTSTS);
  }
  if (interrupt_status & BIT(12)) { // usb reset
    *REG32(USB_GINTSTS) = BIT(12);
    interrupt_status = *REG32(USB_GINTSTS);

    for (int epNr = 0; epNr < 5; epNr++) {
      endpoint_control *ep = GET_OUT(epNr);
      ep->control = BIT(31); // enable
    }
    GET_OUT(0)->size = (1 << 29) | // 1 setup
            (2<<18) | // packet count
            8; // bytes
    GET_OUT(0)->control = BIT(31) | BIT(26) | BIT(15); // enable and active

    // flush tx fifo 1
    *REG32(USB_GRSTCTL) = 0x420;
    while ((*REG32(USB_GRSTCTL) & 0x20) != 0) {}

    // flush rx fifo
    *REG32(USB_GRSTCTL) = 0x10;
    while ((*REG32(USB_GRSTCTL) & 0x10) != 0) {}

    *REG32(USB_DAINTMSK) = BIT(16) | BIT(0); // allow irq on in0 and out0
    *REG32(USB_DOEPMSK) = BIT(3) | BIT(0); // allow irq on xfer complete and timeout
    *REG32(USB_DIEPMSK) = BIT(3) | BIT(0); // allow irq on xfer complete and timeout

    GET_IN(0)->control = BIT(30); // disable IN-0, no data to send yet
  }
  if (interrupt_status & BIT(11)) { // usb suspend
    *REG32(USB_GINTSTS) = BIT(11);
    interrupt_status = *REG32(USB_GINTSTS);
    //puts("suspend");
  }
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
  uint32_t daint = *REG32(USB_DAINT);
  endpoint_control *ep = 0;
  while (daint != 0) {
    //printf("DAINT: 0x%x\n", daint);
    for (int i=0; i<16; i++) {
      if ((daint >> i) & 1) {
        //printf("IN %d irq\n", i);
        ep = GET_IN(i);
        //dump_endpoint(ep, true);
        uint32_t irq = ep->interrupt;
        //printf("acking irq bits 0x%x\n", irq);
        ep->interrupt = irq;
        if (irq & 1) {
          ep_write_in(state, i);
        }
      }
    }
    for (int i=16; i<32; i++) {
      if ((daint >> i) & 1) {
        ep = (endpoint_control*)((USB_BASE + 0x0b00) + (i-16) * 0x20);
        //printf("OUT %d irq\n", i - 16);
        //dump_endpoint(ep, false);
        uint32_t irq = ep->interrupt;
        //printf("acking irq bits 0x%x\n", irq);
        ep->interrupt = irq;
      }
    }
    daint = *REG32(USB_DAINT);
  }
}

static int cmd_dwc_dump(int argc, const cmd_args *argv) {
  uint32_t t;
  endpoint_control *ep = 0;

  dumpreg(USB_GOTGCTL); // 0
  if (t & BIT(16)) puts("  16: B-device mode");
  if (t & BIT(18)) puts("  18: A-session valid (host only)");
  if (t & BIT(19)) puts("  19: B-session valid (device only)");
  dumpreg(USB_GAHBCFG); // 8
  dumpreg(USB_GUSBCFG); // c
  dumpreg(USB_GINTSTS); // 14
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
  dumpreg(USB_GINTMSK);   // 18
  printf("masked irq's: 0x%x\n", *REG32(USB_GINTSTS) & ~(*REG32(USB_GINTMSK)));
  dumpreg(USB_GHWCFG1);
  dumpreg(USB_GHWCFG2);
  dumpreg(USB_GHWCFG3);
  dumpreg(USB_GHWCFG4);
  if ((t >> 25) & 1) puts("  dedicated transmit fifo enabled");
  dumpreg(USB_DCFG);      // 800
  dumpreg(USB_DCTL);      // 804
  dumpreg(USB_DSTS);      // 808
  dumpreg(USB_DIEPMSK);   // 810
  dumpreg(USB_DOEPMSK);   // 814
  dumpreg(USB_DAINT);     // 818
  dumpreg(USB_DAINTMSK);  // 81c
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

static enum handler_return dwc_irq(struct dwc_state_t *state) {
  lk_bigtime_t start = current_time_hires();
  //logmsg("dwc irq");
  dwc_check_interrupt(state);
  lk_bigtime_t end = current_time_hires();
  lk_bigtime_t spent = end - start;
  //logmsg("done");
  if (spent > 20) logf("irq time: %lld\n", spent);
  return INT_NO_RESCHEDULE;
}

static struct dwc_state_t dwc_state;

static void dwc2_init(const struct app_descriptor *app) {
  uint32_t t;
  logf("dwc2_init\n");
  power_up_usb();

  memset(&dwc_state, 0, sizeof(dwc_state));
  dwc_state.mainConfigurationDescriptorSize = sizeof(defaultConfigurationDescriptor) + sizeof(defaultInterfaceDescriptor) + sizeof(ep0Descriptor) + sizeof(ep1Descriptor);
  dwc_state.mainConfigurationDescriptor = malloc(dwc_state.mainConfigurationDescriptorSize);

  int pos = 0;
  memcpy(dwc_state.mainConfigurationDescriptor + pos, &defaultConfigurationDescriptor, sizeof(defaultConfigurationDescriptor));
  pos += sizeof(defaultConfigurationDescriptor);

  memcpy(dwc_state.mainConfigurationDescriptor + pos, &defaultInterfaceDescriptor, sizeof(defaultInterfaceDescriptor));
  pos += sizeof(defaultInterfaceDescriptor);

  memcpy(dwc_state.mainConfigurationDescriptor + pos, &ep0Descriptor, sizeof(ep0Descriptor));
  pos += sizeof(ep0Descriptor);

  memcpy(dwc_state.mainConfigurationDescriptor + pos, &ep1Descriptor, sizeof(ep1Descriptor));
  pos += sizeof(ep1Descriptor);
  assert(pos == dwc_state.mainConfigurationDescriptorSize);

  register_int_handler(DWC_IRQ, dwc_irq, &dwc_state);
  unmask_interrupt(DWC_IRQ);

  dwc_configure_phy();
  dwc_clear_fifo();
  dwc_start();

  //dumpreg(USB_GAHBCFG);
  *REG32(USB_GINTMSK) =
    BIT(19) | BIT(18) | BIT(17) |
    BIT(13) | BIT(12) | BIT(11) | BIT(4)
    | BIT(2)
    ;
  *REG32(USB_DIEPMSK) = (1<<4) | (1<<3) | (1<<0);

  *REG32(USB_DCFG) = (1 << 18) | // in endpoint mismatch count
      (1 << 11) | // periodic frame interval interrupt
      3; // full speed
      //0; // high speed 2.0

  *REG32(USB_GRXFSIZ) = 0x200;

  volatile uint32_t *reg = REG32(USB_DIEPTXF1);
  int start = 0x300;
  for (int i = 0; i< 7; i++) {
    t = start | (0x100 << 16);
    start = start + 0x100;
    reg[i] = t;
  }
}

static void dwc2_entry(const struct app_descriptor *app, void *args) {
}

APP_START(dwc2)
  .init = dwc2_init,
  .entry = dwc2_entry,
APP_END
