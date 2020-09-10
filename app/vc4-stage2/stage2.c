#include <app.h>
#include <lk/debug.h>
#include <platform/bcm28xx/sdhost_impl.h>
#include <lib/partition.h>
#include <lib/fs.h>

static void stage2_init(const struct app_descriptor *app) {
  printf("stage2 init\n");
  bdev_t *sd = rpi_sdhost_init();
  partition_publish("sdhost", 0);
  //fs_mount("/boot", "fat32", "sdhostp0");
  fs_mount("/root", "ext2", "sdhostp0");
}

static void stage2_entry(const struct app_descriptor *app, void *args) {
  printf("stage2 entry\n");
}

APP_START(stage2)
  .init = stage2_init,
  .entry = stage2_entry,
APP_END
