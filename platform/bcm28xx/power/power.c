#include <platform/bcm28xx/power.h>
#include <lk/reg.h>
#include <stdio.h>
#include <platform/bcm28xx/udelay.h>
#include <platform/bcm28xx/pll.h>
#include <lk/console_cmd.h>

static int cmd_pm_dump(int argc, const cmd_args *argv);
static int cmd_pm_usb_on(int argc, const cmd_args *argv);

STATIC_COMMAND_START
STATIC_COMMAND("pm_dump_all", "dump power domain states", &cmd_pm_dump)
STATIC_COMMAND("pm_usb_on", "enable usb power domain", &cmd_pm_usb_on)
STATIC_COMMAND_END(pm);

void power_up_image(void) {
  puts("image domain on...");
  //dumpreg(PM_IMAGE);
  *REG32(PM_IMAGE) |= PM_PASSWORD | 0x10000 | BIT(6); // CFG = 1
#if 0
  printf("PM_IMAGE: 0x%x\n", *REG32(PM_IMAGE));
  *REG32(PM_IMAGE) |= PM_PASSWORD | 1; // POWUP = 1
  for (int i=0; i<10; i++) {
    if (*REG32(PM_IMAGE) & 2) { // if power ok
      break;
    }
    udelay(1);
    if (i == 9) puts("PM_IMAGE timeout");
  }
  printf("PM_IMAGE: 0x%x\n", *REG32(PM_IMAGE));
  *REG32(PM_IMAGE) = PM_PASSWORD | (*REG32(PM_IMAGE) & ~1); // POWUP = 0

  *REG32(PM_IMAGE) |= PM_PASSWORD | 0x30000; // CFG = 3
  *REG32(PM_IMAGE) |= PM_PASSWORD | 1; // POWUP = 1
  for (int i=0; i<10; i++) {
    if (*REG32(PM_IMAGE) & 2) { // if power ok
      break;
    }
    udelay(1);
    if (i == 9) puts("PM_IMAGE timeout");
  }
  printf("PM_IMAGE: 0x%x\n", *REG32(PM_IMAGE));
  *REG32(PM_IMAGE) |= PM_PASSWORD | 0x40;
#endif
  //dumpreg(PM_IMAGE);
  power_domain_on(REG32(PM_IMAGE));
  //dumpreg(PM_IMAGE);
}

void power_up_usb(void) {
  // TODO, move power domain code into its own driver
  power_up_image();

  puts("usb power on...");
  *REG32(PM_USB) = PM_PASSWORD | 1;
  udelay(600);

  //dumpreg(CM_PERIICTL);
  *REG32(CM_PERIIDIV) = CM_PASSWORD | 0x1000;
  *REG32(CM_PERIICTL) |= CM_PASSWORD | 0x40;
  //dumpreg(CM_PERIICTL);
  *REG32(CM_PERIICTL) = (*REG32(CM_PERIICTL) & 0xffffffbf) | CM_PASSWORD;
  //dumpreg(CM_PERIICTL);
  //dumpreg(CM_PERIIDIV);

  *REG32(PM_IMAGE) |= PM_PASSWORD | PM_V3DRSTN;
  *REG32(CM_PERIICTL) |= CM_PASSWORD | 0x40;
}

void power_domain_on(volatile uint32_t *reg) {
  /* If it was already powered on by the fw, leave it that way. */
  if (*REG32(reg) & PM_POWUP) {
    puts("already on");
    return;
  }
  /* Enable power */
  *REG32(reg) |= PM_PASSWORD | PM_POWUP;

  while (!(*REG32(reg) & PM_POWOK)) {
    udelay(1); // TODO, add timeout
  }

  /* Disable electrical isolation */
  *REG32(reg) |= PM_PASSWORD | PM_ISPOW;

  /* Repair memory */
  *REG32(reg) |= PM_PASSWORD | PM_MEMREP;
  while (!(*REG32(reg) & PM_MRDONE)) {
    udelay(1); // TODO, add timeout
  }
  /* Disable functional isolation */
  *REG32(reg) |= PM_PASSWORD | PM_ISFUNC;
}

static void dump_power_domain(const char *name, uint32_t pmreg) {
  volatile uint32_t *reg = REG32(pmreg);
  uint32_t v = *reg;
  printf("%8s: 0x%x == 0x%08x  ", name, pmreg, v);
  printf("%2s  %2s %2s", v & PM_POWUP ? "UP":"", v & PM_POWOK ? "OK":"", v & PM_ENABLE ? "EN":"");
  if ((pmreg == PM_IMAGE) && (v & BIT(7))) printf("  H264RSTN");
  puts("");
}

static int cmd_pm_dump(int argc, const cmd_args *argv) {
  dump_power_domain("PM_USB",   PM_USB);
  dump_power_domain("PM_SMPS",  PM_SMPS);
  dump_power_domain("PM_IMAGE", PM_IMAGE);
  dump_power_domain("PM_GRAFX", PM_GRAFX);
  dump_power_domain("PM_PROC",  PM_PROC);
  return 0;
}
static int cmd_pm_usb_on(int argc, const cmd_args *argv) {
  power_up_usb();
  return 0;
}
