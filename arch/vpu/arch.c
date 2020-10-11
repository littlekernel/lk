#include <sys/types.h>
#include <stdint.h>
#include <lk/debug.h>
#include <lk/console_cmd.h>
#include <lk/reg.h>
#include <platform/bcm28xx.h>
#include <kernel/thread.h>
#include <string.h>
#include <platform/bcm28xx/pll.h>
#include <dev/gpio.h>
#include <platform/bcm28xx/udelay.h>
#include <dev/uart.h>

static int cmd_boot_other_core(int argc, const cmd_args *argv);
static int cmd_testit(int argc, const cmd_args *argv);
static int cmd_jitter(int argc, const cmd_args *argv);

static char core2_stack[512];
uint32_t core2_stack_top = 0;

STATIC_COMMAND_START
STATIC_COMMAND("boot_other_core", "boot the 2nd vpu core", &cmd_boot_other_core)
STATIC_COMMAND("testit", "do some asm tests", &cmd_testit)
//STATIC_COMMAND("jitter", "jitter test", &cmd_jitter)
STATIC_COMMAND_END(arch);

extern uint8_t _fbss;
extern uint8_t _ebss;

void zero_bss(void) {
  bzero(&_fbss, &_ebss - &_fbss);
}

void arch_early_init(void) {
  uint32_t r28, sp, sr;
  __asm__ volatile ("mov %0, r28" : "=r"(r28));
  __asm__ volatile ("mov %0, sp" : "=r"(sp));
  __asm__ volatile ("mov %0, sr" : "=r"(sr));
  //dprintf(INFO, "arch_early_init\nr28: 0x%x\nsp: 0x%x\nsr: 0x%x\n", r28, sp, sr);
}

void arch_init(void) {
  uint32_t r28, sp, cpuid;
  __asm__ volatile ("mov %0, r28" : "=r"(r28));
  __asm__ volatile ("mov %0, sp" : "=r"(sp));
  __asm__ volatile ("version %0" : "=r"(cpuid));
  dprintf(INFO, "arch_init\nr28: 0x%x\nsp: 0x%x\ncpuid: %x\n", r28, sp, cpuid);
}

void arch_idle(void) {
    asm volatile("sleep");
}

void arch_chain_load(void *entry, ulong arg0, ulong arg1, ulong arg2, ulong arg3) {
  puts("flushing uart tx and chainloading...\n");
  uart_flush_tx(0);
  __asm__ volatile ("mov r0, %0\nmov r1, %1\nmov r2, %2\nmov r3, %3\nbl %4":
      : "r"(arg0)
      , "r"(arg1)
      , "r"(arg2)
      , "r"(arg3)
      , "r"(entry));
  panic("chainload somehow returned");
}

void arch_sync_cache_range(addr_t start, size_t len) {
}

void core2_start(void);

volatile uint32_t foo;

static int cmd_boot_other_core(int argc, const cmd_args *argv) {
  core2_stack_top = (uint32_t)((core2_stack + sizeof(core2_stack)) - 4);
  //*REG32(A2W_PLLC_CORE1) = A2W_PASSWORD | 6; // 3ghz/6 == 500mhz
  *REG32(IC1_WAKEUP) = (uint32_t)(&core2_start);
  for (int i=0; i<20; i++) {
    udelay(2);
    printf("%d %d\n", i, foo);
  }
  return 0;
}

void core2_entry(void) {
  dprintf(INFO, "core2 says hello\n");
  for (int i=0; i<300; i++) {
    udelay(1);
    foo = i;
  }
  for (;;);
}

void testit(uint32_t *, uint32_t, uint32_t, uint32_t, uint32_t);

static int cmd_testit(int argc, const cmd_args *argv) {
  uint32_t target[4];
  testit(target, 0x11, 0x22, 0x33, 0x44);
  printf("%x %x %x %x\n", target[0], target[1], target[2], target[3]);
  return 0;
}

void toggle18(void);

static int cmd_jitter(int argc, const cmd_args *argv) {
  //__asm__ volatile("di");
  gpio_config(18, 1);
  for (int i=0; i < 10000000; i++) {
    toggle18();
    //gpio_set(18,1);
    //gpio_set(18,0);
  }
  //__asm__ volatile("ei");
  return 0;
}

int vc4_atomic_add(volatile int *ptr, int val) {
  // TODO
  //spin_lock_saved_state_t state;
  //arch_interrupt_save(&state, SPIN_LOCK_FLAG_INTERRUPTS);
  THREAD_LOCK(state);
  int old = *ptr;
  *ptr += val;
  //arch_interrupt_restore(&state, SPIN_LOCK_FLAG_INTERRUPTS);
  THREAD_UNLOCK(state);
  return old;
}

void arch_clean_cache_range(addr_t start, size_t len) {
  // TODO
}
