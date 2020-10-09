#include <kernel/novm.h>
#include <lk/init.h>
#include <platform/bcm28xx/sdram.h>

#define UNCACHED_RAM 0xc0000000
#define MB (1024*1024)

static void autoram_dram_init(uint level) {
  sdram_init();
  uint32_t start = UNCACHED_RAM | (1 * MB);
  uint32_t length = 10 * MB;
  novm_add_arena("dram", start, length);
}
LK_INIT_HOOK(autoram, &autoram_dram_init, LK_INIT_LEVEL_PLATFORM_EARLY + 1);
