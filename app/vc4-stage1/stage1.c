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
#include <string.h>

#define UNCACHED_RAM 0xc0000000
#define MB (1024*1024)

static void stage2_dram_init(uint level) {
  sdram_init();
  uint32_t start = UNCACHED_RAM | (1 * MB);
  uint32_t length = 10 * MB;
  novm_add_arena("dram", start, length);
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
    return 0;
  }
  if (c == 'X') {
    printf("got char 0x%x\n", c);
    THREAD_LOCK(state);
    wait_queue_wake_all(waiter, false, c);
    THREAD_UNLOCK(state);
  }
  return 0;
}

static void *load_and_run_elf(elf_handle_t *stage2_elf) {
  int ret = elf_load(stage2_elf);
  if (ret) {
    printf("failed to load elf: %d\n", ret);
    return;
  }
  elf_close_handle(stage2_elf);
  void *entry = stage2_elf->entry;
  free(stage2_elf);
  return entry;
}

static void load_stage2(void) {
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
  void *entry = load_and_run_elf(stage2_elf);
  fs_close_file(stage2);
  arch_chain_load(entry, 0, 0, 0, 0);
}

struct xmodem_packet {
  uint8_t magic;
  uint8_t block_num;
  uint8_t block_num_invert;
  uint8_t payload[128];
  uint8_t checksum;
} __attribute__((packed));

//static_assert(sizeof(struct xmodem_packet) == 132, "xmodem packet malformed");

static ssize_t read_repeat(io_handle_t *in, void *buf, ssize_t len) {
  ssize_t total_read = 0;
  ssize_t ret;
  while ((ret = io_read(in, buf, len)) > 0) {
    //printf("0X%02x %d\n\n", ((uint8_t*)buf)[0], ret);
    len -= ret;
    total_read += ret;
    buf += ret;
    if (len <= 0) return total_read;
  }
  return -1;
}

static void xmodem_receive(void) {
  size_t capacity = 2 * MB;
  void *buffer = malloc(capacity);
  size_t used = 0;
  struct xmodem_packet *packet = malloc(sizeof(struct xmodem_packet));
  ssize_t ret;
  int blockNr = 1;
  bool success = false;

  io_write(&console_io, "\x15", 1);
  while ((ret = io_read(&console_io, packet, 1)) == 1) {
    if (packet->magic == 4) {
      puts("R: EOF!");
      success = true;
      break;
    }
    ret = read_repeat(&console_io, &packet->block_num, sizeof(struct xmodem_packet) - 1);
    if (ret != (sizeof(struct xmodem_packet)-1)) {
      puts("read error");
      break;
    }
    uint8_t checksum = 0;
    for (int i=0; i<128; i++) {
      checksum += packet->payload[i];
    }
    bool fail = true;
    if (packet->checksum == checksum) {
      if (packet->block_num_invert == (255 - packet->block_num)) {
        if (packet->block_num == (blockNr & 0xff)) {
          memcpy(buffer + (128 * (blockNr-1)), packet->payload, 128);
          blockNr++;
          io_write(&console_io, "\6", 1);
          fail = false;
        } else if (packet->block_num == ((blockNr - 1) & 0xff)) { // ack was lost, just re-ack
          io_write(&console_io, "\6", 1);
        } else {
          io_write(&console_io, "\x15", 1);
        }
      } else { // block_invert wrong
        io_write(&console_io, "\x15", 1);
      }
    } else { // wrong checksum
      io_write(&console_io, "\x15", 1);
    }
    if (fail) printf("got packet: %d %d %d %d/%d\n", packet->magic, packet->block_num, packet->block_num_invert, packet->checksum, checksum);
  }
  printf("final ret was %ld\n", ret);
  free(packet);
  if (success) {
    elf_handle_t *stage2_elf = malloc(sizeof(elf_handle_t));
    ret = elf_open_handle_memory(stage2_elf, buffer, blockNr*128);
    if (ret) {
      printf("failed to elf open: %d\n", ret);
      return;
    }
    void *entry = load_and_run_elf(stage2_elf);
    free(buffer);
    arch_chain_load(entry, 0, 0, 0, 0);
    return;
  }
  free(buffer);
}

static void stage2_core_entry(void *_unused) {
  spin_lock_saved_state_t state1;
  arch_interrupt_save(&state1, 0);
  printf("stage2 init\n");
  puts("press X to stop autoboot and go into xmodem mode...");
  wait_queue_init(&waiter);

  thread_t *waker = thread_create("waker", waker_entry, &waiter, DEFAULT_PRIORITY, ARCH_DEFAULT_STACK_SIZE);
  thread_resume(waker);
  arch_interrupt_restore(state1, 0);

  THREAD_LOCK(state);
  int ret = wait_queue_block(&waiter, 100000);
  THREAD_UNLOCK(state);

  printf("wait result: %d\n", ret);

  if (ret == 'X') {
    puts("going into xmodem mode");
    uint32_t rsts = *REG32(PM_RSTS);
    printf("%x\n", rsts);
    xmodem_receive();
  } else {
    load_stage2();
  }
}

static thread_t *stage2_core;

static void stage2_init(const struct app_descriptor *app) {
  stage2_core = thread_create("stage2", stage2_core_entry, 0, DEFAULT_PRIORITY, ARCH_DEFAULT_STACK_SIZE);
  thread_resume(stage2_core);
}

static void stage2_entry(const struct app_descriptor *app, void *args) {
  printf("stage2 entry\n");
}

APP_START(stage2)
  .init = stage2_init,
  .entry = stage2_entry,
APP_END
