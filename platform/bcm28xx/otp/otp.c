#include <stdint.h>
#include <lk/reg.h>
#include <platform/bcm28xx.h>
#include <platform/bcm28xx/otp.h>
#include <platform/bcm28xx/udelay.h>
#include <stdio.h>
#include <lk/console_cmd.h>

#define OTP_MAX_CMD_WAIT 1000

enum otp_command {
  OTP_CMD_READ = 0,
};

/* OTP_CTRL_LO Bits */
#define OTP_CMD_START 1

/* OTP Status Bits */
#ifdef RPI4
# define OTP_STAT_CMD_DONE 2
#else
# define OTP_STAT_CMD_DONE 1
#endif

static int cmd_otp_pretty(int argc, const cmd_args *argv);
static int cmd_otp_full(int argc, const cmd_args *argv);

STATIC_COMMAND_START
STATIC_COMMAND("otp_pretty_print", "pretty-print all known otp values", &cmd_otp_pretty)
STATIC_COMMAND("otp_dump_all","dump all OTP values", &cmd_otp_full)
STATIC_COMMAND_END(otp);

static inline void otp_delay(void) {
  udelay(1);
}

static void otp_open(void) {
  // TODO: Is this really needed? The code seems to work
  // without this block, at least on BCM2711 B0 silicon.
  *REG32(OTP_CONFIG) = 3;
  otp_delay();

  *REG32(OTP_CTRL_HI) = 0;
  *REG32(OTP_CTRL_LO) = 0;
  *REG32(OTP_ADDR) = 0;
  *REG32(OTP_DATA) = 0;
  *REG32(OTP_CONFIG) = 2;
}

static void otp_close(void) {
  *REG32(OTP_CTRL_HI) = 0;
  *REG32(OTP_CTRL_LO) = 0;
  *REG32(OTP_CONFIG) = 0;
}

static int otp_wait_status(uint32_t mask, int retry) {
  int i = retry;
  do {
    otp_delay();
    if (*REG32(OTP_STATUS) & mask)
      return 0;
  } while (--i);
  return -1;
}

static int otp_set_command(enum otp_command cmd) {
  *REG32(OTP_CTRL_HI) = 0;
  *REG32(OTP_CTRL_LO) = cmd << 1;
  if (otp_wait_status(OTP_STAT_CMD_DONE, OTP_MAX_CMD_WAIT))
    return -1;
  *REG32(OTP_CTRL_LO) = (cmd << 1) | OTP_CMD_START;
  return otp_wait_status(OTP_STAT_CMD_DONE, OTP_MAX_CMD_WAIT);
}

static uint32_t otp_read_open(uint8_t addr) {
  *REG32(OTP_ADDR) = addr;
  return (uint32_t) otp_set_command(OTP_CMD_READ) ?: *REG32(OTP_DATA);
}

uint32_t otp_read(uint8_t addr) {
  uint32_t val;
  otp_open();
  val = otp_read_open(addr);
  otp_close();
  return val;
}

void dump_all_otp(void) {
  printf("full otp dump\n");
  for (uint8_t addr=0; addr < 67; addr++) {
    uint32_t value = otp_read(addr);
    printf("%02d:%08x\n", addr, value);
  }
}

static int cmd_otp_full(int argc, const cmd_args *argv) {
  dump_all_otp();
  return 0;
}

void otp_pretty_print(void) {
  // https://github.com/raspberrypi/firmware/issues/974
  // addr 32 contains warrenty flag
  uint32_t bootmode = otp_read(17);
  printf("OTP17: 0x%08x\n", bootmode);
  if (bootmode & (1<< 1)) printf("  19.2mhz crystal present\n");
  if (bootmode & (1<< 3)) printf("  SDIO pullups should be on?\n");
  if (bootmode & (1<<19)) printf("  GPIO boot mode\n");
  if (bootmode & (1<<20)) printf("  GPIO boot mode bank bit is set\n");
  if (bootmode & (1<<21)) printf("  SD card boot enabled\n");
  if (bootmode & (1<<22)) printf("  bank to boot from set\n");
  if (bootmode & (1<<28)) printf("  USB device boot enabled\n");
  if (bootmode & (1<<29)) printf("  USB host boot enabled (ethernet/mass-storage)\n");
  if (bootmode != otp_read(18)) printf("WARNING: boot mode duplicate doesnt match\n");
  uint32_t serial = otp_read(28);
  printf("\nSoC serial# 0x%08x\n", serial);
  if (~serial != otp_read(29)) printf("WARNING: serial# duplicate doesnt match\n");
  uint32_t revision = otp_read(30);
  // https://www.raspberrypi.org/documentation/hardware/raspberrypi/revision-codes/README.md
  printf("\nHW revision: 0x%08x\n", revision);
  if (revision & (1 <<23)) { // new style revision
    printf("  Type: %d\n", (revision >> 4) & 0xff);
    printf("  Rev: %d\n", revision & 0xf);
    printf("  Proc: %d\n", (revision >> 12) & 0xf);
    printf("  Manufacturer: %d\n", (revision >> 16) & 0xf);
    printf("  Ram: %d\n", (revision >> 20) & 0x7);
  }
  uint32_t maclow = otp_read(64);
  uint32_t machi = otp_read(65);
  printf("\nOptional Mac Override: %08x%x\n", maclow, machi);

  uint32_t advanced_boot = otp_read(66);
  printf("\nOTP66: 0x%08x\n", advanced_boot);
  if (advanced_boot & (1<<7)) {
    printf("  ETH_CLK on GPIO%d\n", advanced_boot & 0x7f);
    if (advanced_boot & (1<<25)) printf("  ETH_CLK 24mhz\n");
    else printf("  ETH_CLK 25mhz\n");
  }
  if (advanced_boot & (1<<15)) {
    printf("  LAN_RUN on GPIO%d\n", (advanced_boot >> 8) & 0x7f);
  }
  if (advanced_boot & (1<<24)) printf("  extended USB timeout\n");
}

int cmd_otp_pretty(int argc, const cmd_args *argv) {
  otp_pretty_print();
  return 0;
}
