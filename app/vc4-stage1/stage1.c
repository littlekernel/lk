#include <app.h>
#include <lk/debug.h>
#include <platform/bcm28xx/sdhost_impl.h>
#include <lib/partition.h>
#include <lib/fs.h>
#include <platform/bcm28xx/sdram.h>
#include <kernel/novm.h>
#include <lk/init.h>

static void stage2_dram_init(uint level) {
  sdram_init();
  // add 1mb of heap, 1mb from start of ram
  novm_add_arena("dram", 0xc0000000 + (1024*1024), 1024*1024);
}

LK_INIT_HOOK(stage1, &stage2_dram_init, LK_INIT_LEVEL_PLATFORM_EARLY + 1);

static void stage2_init(const struct app_descriptor *app) {
  printf("stage2 init\n");
  bdev_t *sd = rpi_sdhost_init();
  partition_publish("sdhost", 0);
  //fs_mount("/boot", "fat32", "sdhostp0");
  int ret = fs_mount("/root", "ext2", "sdhostp1");
  printf("mount status %d\n", ret);
}

static void stage2_entry(const struct app_descriptor *app, void *args) {
  printf("stage2 entry\n");
}

APP_START(stage2)
  .init = stage2_init,
  .entry = stage2_entry,
APP_END
