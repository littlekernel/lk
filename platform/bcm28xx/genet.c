// based on drivers/net/ethernet/broadcom/genet/bcmgenet.c from linux
// for PHY control, look at the cmd_bits variable in drivers/net/ethernet/broadcom/genet/bcmmii.c

#include <lk/reg.h>
#include <stdio.h>
#include <lk/console_cmd.h>
#include <platform/bcm28xx.h>
#include <lk/debug.h>

static int cmd_genet_dump(int argc, const cmd_args *argv);

STATIC_COMMAND_START
STATIC_COMMAND("dump_genet", "print genet information", &cmd_genet_dump)
STATIC_COMMAND_END(genet);

#define SYS_REV_CTRL (GENET_BASE + 0x0)

static int cmd_genet_dump(int argc, const cmd_args *argv) {
  uint32_t reg = *REG32(SYS_REV_CTRL);
  uint8_t major = (reg >> 24 & 0x0f);
  if (major == 6) major = 5;
  else if (major == 5) major = 4;
  else if (major == 0) major = 1;

  dprintf(INFO, "found GENET controller version %d\n", major);
  return 0;
}
