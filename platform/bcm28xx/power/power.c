#include <platform/bcm28xx/power.h>
#include <lk/reg.h>
#include <stdio.h>
#include <platform/bcm28xx/udelay.h>
#include <platform/bcm28xx/pll.h>

void power_up_image(void) {
  puts("image domain on...");
  //dumpreg(PM_IMAGE);
  *REG32(PM_IMAGE) |= PM_PASSWORD | 0x10000; // CFG = 1
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
