#include <app.h>
#include <stdio.h>
#include <platform/bcm28xx/sdram.h>
#include <platform/bcm28xx/otp.h>

static void bootloader_init(const struct app_descriptor *app) {
  printf("bootloader init\n");
}

static void bootloader_entry(const struct app_descriptor *app, void *args) {
  printf("bootloader entry\n\n");
  otp_pretty_print();
  //sdram_init();
}

APP_START(bootloader)
  .init = bootloader_init,
  .entry = bootloader_entry,
APP_END
