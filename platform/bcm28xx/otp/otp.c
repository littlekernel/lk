#include <stdint.h>
#include <inttypes.h>
#include <lk/reg.h>
#include <platform/bcm28xx.h>
#include <platform/bcm28xx/otp.h>
#include <platform/bcm28xx/udelay.h>
#include <stdio.h>
#include <lk/console_cmd.h>

#define OTP_MAX_CMD_WAIT 1000
#define OTP_MAX_PROG_WAIT 32000
#define OTP_MAX_WRITE_RETRIES 16

enum otp_command {
  OTP_CMD_READ = 0,
#ifndef RPI4
  OTP_CMD_PROG_ENABLE  = 0x1,
  OTP_CMD_PROG_DISABLE = 0x2,
#else
  OTP_CMD_PROG_ENABLE  = 0x2,
  OTP_CMD_PROG_DISABLE = 0x3,
#endif
  OTP_CMD_PROGRAM_WORD = 0xa,
};

/* OTP_CTRL_LO Bits */
#define OTP_CMD_START 1
#ifndef RPI4
# define OTP_STAT_PROG_OK 4
# define OTP_STAT_PROG_ENABLE 0x1000
#else
# define OTP_STAT_PROG_ENABLE 4
#endif

/* OTP Status Bits */
#ifdef RPI4
# define OTP_STAT_CMD_DONE 2
#else
# define OTP_STAT_CMD_DONE 1
#endif

#define OTP_PROG_EN_SEQ { 0xf, 0x4, 0x8, 0xd };

static int cmd_otp_pretty(int argc, const cmd_args *argv);
static int cmd_otp_full(int argc, const cmd_args *argv);
static int cmd_otp_write(int argc, const cmd_args *argv);

STATIC_COMMAND_START
STATIC_COMMAND("otp_pretty_print", "pretty-print all known otp values", &cmd_otp_pretty)
STATIC_COMMAND("otp_dump_all","dump all OTP values", &cmd_otp_full)
STATIC_COMMAND("otp_write","write new OTP value", &cmd_otp_write)
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

static int otp_set_command(uint32_t ctrlhi, uint32_t cmd) {
  *REG32(OTP_CTRL_HI) = ctrlhi;
  *REG32(OTP_CTRL_LO) = cmd << 1;
  if (otp_wait_status(OTP_STAT_CMD_DONE, OTP_MAX_CMD_WAIT))
    return -1;
  *REG32(OTP_CTRL_LO) = (cmd << 1) | OTP_CMD_START;
  return otp_wait_status(OTP_STAT_CMD_DONE, OTP_MAX_CMD_WAIT);
}

static uint32_t otp_read_open(uint8_t addr) {
  *REG32(OTP_ADDR) = addr;
  return (uint32_t) otp_set_command(0, OTP_CMD_READ) ?: *REG32(OTP_DATA);
}

uint32_t otp_read(uint8_t addr) {
  uint32_t val;
  otp_open();
  val = otp_read_open(addr);
  otp_close();
  return val;
}

static int otp_enable_program(void)
{
  static const uint32_t seq[] = OTP_PROG_EN_SEQ;
  unsigned i;

  for (i = 0; i < countof(seq); ++i) {
#ifndef RPI4
    *REG32(OTP_BITSEL) = seq[i];
#else
    *REG32(OTP_DATA) = seq[i];
#endif
    if (otp_set_command(0, OTP_CMD_PROG_ENABLE))
      return -1;
  }
  return otp_wait_status(OTP_STAT_PROG_ENABLE, OTP_MAX_PROG_WAIT);
}

static int otp_disable_program(void)
{
  return otp_set_command(0, OTP_CMD_PROG_DISABLE);
}

#ifndef RPI4

static int otp_program_bit(uint8_t addr, uint8_t bit)
{
  uint32_t cmd;
  int err;
  int i;

  *REG32(OTP_ADDR) = addr;
  *REG32(OTP_BITSEL) = bit;

  if (addr < 8)
    cmd = 0x38000a;
  else if (addr < 77)
    cmd = 0x58000a;
  else
    cmd = 0x78000a;
  err = -1;
  for (i = 0; i < OTP_MAX_WRITE_RETRIES; ++i) {
    otp_set_command(0x1420000, cmd);
    if (!otp_set_command(0x1484, 0x88003) &&
        (*REG32(OTP_STATUS) & OTP_STAT_PROG_OK)) {
      err = 0;
      break;
    }
  }
  for (i = 0; i < OTP_MAX_WRITE_RETRIES; ++i) {
    otp_set_command(0x9420000, cmd);
    if (!otp_set_command(0x8001484, 0x88003) &&
        (*REG32(OTP_STATUS) & OTP_STAT_PROG_OK)) {
      err = 0;
      break;
    }
  }
  return err;
}

static int otp_write_enabled(uint8_t addr, uint32_t newval)
{
  int bitnum, err;
  uint32_t oldval = otp_read_open(addr);
  if (oldval == 0xffffffffL)    // This check guards against read failures
    return 0;

  err = 0;
  for (bitnum = 0; bitnum < 32; ++bitnum) {
    uint32_t bitmask = (uint32_t)1 << bitnum;
    if ((oldval & bitmask) != 0 || (newval & bitmask) == 0)
      continue;
    if (otp_program_bit(addr, bitnum) < 0)
      ++err;
  }
  return err;
}

#else  /* RPI4 */

static int otp_write_enabled(uint8_t addr, uint32_t newval)
{
  uint32_t oldval = otp_read_open(addr);
  if (oldval == 0xffffffffL)    // This check guards against read failures
    return 0;
  *REG32(OTP_DATA) = newval | oldval;
  otp_delay();
  *REG32(OTP_ADDR) = addr;
  otp_delay();
  return otp_set_command(0, OTP_CMD_PROGRAM_WORD);
}

#endif  /* RPI4 */

static int otp_write_open(uint8_t addr, uint32_t val)
{
  int err;
  if (otp_enable_program())
    return OTP_ERR_ENABLE;
  err = otp_write_enabled(addr, val);
  if (err)
    err = OTP_ERR_PROGRAM;
  if (otp_disable_program())
    err = err ?: OTP_ERR_DISABLE;
  return err;
}

int otp_write(uint8_t addr, uint32_t val)
{
  int err;
  otp_open();
  err = otp_write_open(addr, val);
  otp_close();
  return err;
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

static int cmd_otp_write(int argc, const cmd_args *argv) {
  uint32_t addr, val;
  int err;
  if (argc != 3) {
    printf("usage: otp_write 36 0x20\n");
    return -1;
  }
  addr = argv[1].u;
  val = argv[2].u;
  otp_open();
  printf("old value: 0x%08"PRIx32"\n", otp_read_open(addr));
  err = otp_write_open(addr, val);
  switch (err) {
    case OTP_ERR_ENABLE:
      printf("cannot %s OTP programming\n", "enable");
      break;
    case OTP_ERR_PROGRAM:
      printf("programming command failed\n");
      break;
    case OTP_ERR_DISABLE:
      printf("cannot %s OTP programming\n", "disable");
      break;
#ifndef RPI4
    case 0:
      break;
    default:
      printf("%d bits failed\n", err);
#endif
  }
  printf("new value: 0x%08"PRIx32"\n", otp_read_open(addr));
  otp_close();
  return 0;
}
