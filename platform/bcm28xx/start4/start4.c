#include <lk/init.h>
#include <lk/reg.h>
#include <platform/bcm28xx.h>
#include <stdint.h>
#include <stdio.h>

uint32_t bootloader_state[4] __attribute__ ((section(".text.bootloader_state"))) = { 0x41545342, 0, 0, 0 };
// bit 0 declares support for a certain cpuid on the VPU
// bit 1 is pi400 flag?
// pi400 booting over usb-dev requires both bit0 and bit1
uint32_t firmware_rev[4] __attribute__ ((section(".text.firmware_rev"))) = { 0x3, 0, 0, 0 };

typedef struct {
  char revision[41];
  char type[8];
  char date[16];
  char time[12];
  uint32_t padding;
  uint32_t something;
  uint32_t serial;
  uint32_t something2;
} bver_state;
// the new tryboot flag may appear in BVER

typedef struct {
  uint32_t macend;    // 0
  uint32_t macstart;  // 4
  uint8_t mac[6];     // 10
  uint8_t pad[2];     // 12
  uint32_t thing1;    // 14
  uint16_t thing2;    // 18
  uint8_t pad2[2];    // 20
  char serial_str[12];// 22
  uint8_t myip[4];    // 34
  uint32_t pad3[2];   // 38
  uint8_t tftp_ip[4]; // 40
  char prefix[128];   // 44
} bstn_state;

typedef struct {
  void *vl805hub_addr;
  uint32_t vl805hub_size;
  void *vl805mcu_addr;
  uint32_t vl805mcu_size;
  uint32_t bitfield; // bit1 means vl805 eeprom is present
  void *buffer;
  uint32_t usb_handle;
} busb_state;

typedef struct {
  uint32_t a;
  uint32_t b;
  uint32_t c;
} bstm_state;

static void print_bver(bver_state *bver) {
  printf("\trevision: %s, type: %s, %s %s, 0x%x, serial: 0x%x something 0x%x\n", bver->revision, bver->type, bver->date, bver->time, bver->something, bver->serial, bver->something2);
}

static void print_bstn(bstn_state *bstn) {
  printf("\tmac1: %08x%08x\n", bstn->macstart, bstn->macend);
  printf("\tmac2: %02x:%02x:%02x:%02x:%02x:%02x\n", bstn->mac[0], bstn->mac[1], bstn->mac[2], bstn->mac[3], bstn->mac[4], bstn->mac[5]);
  printf("\tserial1: %s\n", bstn->serial_str);
  printf("\tmyip: %d.%d.%d.%d\n", bstn->myip[3], bstn->myip[2], bstn->myip[1], bstn->myip[0]);
  printf("\ttftp_server: %d.%d.%d.%d\n", bstn->tftp_ip[3], bstn->tftp_ip[2], bstn->tftp_ip[1], bstn->tftp_ip[0]);
  printf("\ttftp_prefix: %s\n", bstn->prefix);
}

static void print_busb(busb_state *busb) {
  printf("\tvl805hub: 0x%x + %d\n", busb->vl805hub_addr, busb->vl805hub_size);
  printf("\tvl805mcu: 0x%x + %d\n", busb->vl805mcu_addr, busb->vl805mcu_size);
  printf("\tbitfield: 0x%x\n", busb->bitfield);
  printf("\tbuffer: 0x%x, handle: 0x%x\n", busb->buffer, busb->usb_handle);
}

static void print_bstm(bstm_state *bstm) {
  printf("\tA: 0x%x, B: 0x%x, C: 0x%x\n", bstm->a, bstm->b, bstm->c);
  if (bstm->b == 0) puts("\tSD");
  if (bstm->b == 1) puts("\tNET");
  if (bstm->b == 2) puts("\tUSB-MSD");
  if (bstm->b == 3) puts("\tUSB-DEV");
  if (bstm->b == 4) puts("\tUSB-BCM-MSD");
}

static char guard(char c) {
  if ((c >= ' ') && (c <= '~')) {
    return c;
  } else {
    return '.';
  }
}

static void print_word(int i, uint32_t w) {
  char a,b,c,d;
  a = w & 0xff;
  b = (w >> 8) & 0xff;
  c = (w >> 16) & 0xff;
  d = (w >> 24) & 0xff;
  a = guard(a);
  b = guard(b);
  c = guard(c);
  d = guard(d);
  printf("%d: 0x%x (%c%c%c%c)\n", i, w, a,b,c,d);
}

static void start4_print_state(uint level) {
  printf("ST_CLO: %d\n", *REG32(ST_CLO));
  printf("bootloader state 0x%x 0x%x 0x%x 0x%x\n", bootloader_state[0], bootloader_state[1], bootloader_state[2], bootloader_state[3]);
  uint32_t *state = bootloader_state[1];
  if (state) {
    puts("some state exists");
    while (true) {
      char a,b,c,d;
      a = state[0] & 0xff;
      b = (state[0] >> 8) & 0xff;
      c = (state[0] >> 16) & 0xff;
      d = (state[0] >> 24) & 0xff;
      printf("TAG 0x%x(%c%c%c%c), 0x%x 0x%x\n", state[0], a,b,c,d, state[1], state[2]);
      switch (state[0]) {
      case 0x52455642: // BVER
        print_bver(&state[3]);
        break;
      case 0x4e545342: // BSTN
        print_bstn(&state[3]);
        break;
      case 0x42535542: // BUSB
        print_busb(&state[3]);
        break;
      case 0x4d545342: // BSTM
        print_bstm(&state[3]);
        break;
      default: {
        uint32_t size = state[2];
        if (size == 0) size = state[1];
        for (int i=3; i < (size/4); i++) {
          print_word(i, state[i]);
        }
      }
      }
      if (state[2] == 0) break;
      state += (state[2]/4);
    }
  }
}

LK_INIT_HOOK(start4, &start4_print_state, LK_INIT_LEVEL_APPS);
