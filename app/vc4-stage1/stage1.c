#include <app.h>
#include <kernel/novm.h>
#include <kernel/thread.h>
#include <kernel/wait.h>
#include <lib/elf.h>
#include <lib/fs.h>
#include <lib/io.h>
#include <lib/partition.h>
#include <lk/debug.h>
#include <lk/init.h>
#include <lk/reg.h>
#include <platform.h>
#include <platform/bcm28xx/pm.h>
#include <platform/bcm28xx/sdhost_impl.h>
#include <platform/bcm28xx/sdram.h>

static void stage2_dram_init(uint level) {
  sdram_init();
  // add 1mb of heap, 1mb from start of ram
  novm_add_arena("dram", 0xc0000000 + (1024*1024), 1024*1024);
}

LK_INIT_HOOK(stage1, &stage2_dram_init, LK_INIT_LEVEL_PLATFORM_EARLY + 1);

static ssize_t fs_read_wrapper(struct elf_handle *handle, void *buf, uint64_t offset, size_t len) {
  return fs_read_file(handle->read_hook_arg, buf, offset, len);
}

static wait_queue_t waiter;

static int waker_entry(wait_queue_t *waiter) {
  char c;
  int ret = platform_dgetc(&c, true);
  if (ret) {
    puts("failed to getc\n");
    thread_exit(0);
  }
  if (c == 'X') {
    THREAD_LOCK(state);
    wait_queue_wake_all(waiter, false, c);
    THREAD_UNLOCK(state);
  }
  thread_exit(0);
}

static void load_stage2() {
  int ret;

  bdev_t *sd = rpi_sdhost_init();
  partition_publish("sdhost", 0);
  //fs_mount("/boot", "fat32", "sdhostp0");
  ret = fs_mount("/root", "ext2", "sdhostp1");
  if (ret) {
    printf("mount failure: %d\n", ret);
    return;
  }
  filehandle *stage2;
  ret = fs_open_file("/root/lk.elf", &stage2);
  if (ret) {
    printf("open failed: %d\n", ret);
    return;
  }
  elf_handle_t *stage2_elf = malloc(sizeof(elf_handle_t));
  ret = elf_open_handle(stage2_elf, fs_read_wrapper, stage2, false);
  if (ret) {
    printf("failed to elf open: %d\n", ret);
    return;
  }
  ret = elf_load(stage2_elf);
  if (ret) {
    printf("failed to load elf: %d\n", ret);
    return;
  }
  fs_close_file(stage2);
  elf_close_handle(stage2_elf);
  uint32_t entry = stage2_elf->entry;
  free(stage2_elf);
  arch_chain_load(entry, 0, 0, 0, 0);
}

static void stage2_init(const struct app_descriptor *app) {
  printf("stage2 init\n");
  puts("press X to stop autoboot and go into xmodem mode...");
  wait_queue_init(&waiter);

  thread_t *waker = thread_create("waker", waker_entry, &waiter, DEFAULT_PRIORITY, ARCH_DEFAULT_STACK_SIZE);
  thread_resume(waker);

  THREAD_LOCK(state);
  int ret = wait_queue_block(&waiter, 10000);
  THREAD_UNLOCK(state);

  printf("wait result: %d\n", ret);

  if (ret == 'X') {
    puts("going into xmodem mode");
    uint32_t rsts = *REG32(PM_RSTS);
    printf("%x\n", rsts);
    platform_halt(HALT_ACTION_REBOOT, HALT_REASON_SW_RESET);
  } else {
    load_stage2();
  }
}

static void stage2_entry(const struct app_descriptor *app, void *args) {
  printf("stage2 entry\n");
}

APP_START(stage2)
  .init = stage2_init,
  .entry = stage2_entry,
APP_END
