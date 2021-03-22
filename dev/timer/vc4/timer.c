#include <assert.h>
#include <lk/err.h>
#include <lk/init.h>
#include <lk/reg.h>
#include <platform/bcm28xx.h>
#include <platform/interrupts.h>
#include <platform/timer.h>

static enum handler_return timer0_irq(void *arg);
static void vc4_timer_init(uint level);

static platform_timer_callback timer_cb = 0;;
static void *timer_arg = 0;;

LK_INIT_HOOK(vc4_timer, &vc4_timer_init, LK_INIT_LEVEL_PLATFORM);

lk_bigtime_t current_time_hires(void) {
  //TODO, deal with rollover
  return ( ((lk_bigtime_t)*REG32(ST_CHI)) << 32) | *REG32(ST_CLO);
}

lk_time_t current_time(void) {
  return current_time_hires();
}

static void vc4_timer_init(uint level) {
#if VC4_TIMER_CHANNEL == 0
  // TODO, only register the interrupt handler once
  register_int_handler(0, timer0_irq, NULL);
  unmask_interrupt(0);
#elif VC4_TIMER_CHANNEL == 1
  register_int_handler(1, timer0_irq, NULL);
  unmask_interrupt(1);
#else
#error unsupported timer channel
#endif
}

status_t platform_set_oneshot_timer (platform_timer_callback callback, void *arg, lk_time_t interval) {
  timer_cb = callback;
  timer_arg = arg;
  //printf("platform_set_oneshot_timer(..., ..., %d)\n", interval);
#if VC4_TIMER_CHANNEL == 0
  *REG32(ST_C0) = *REG32(ST_CLO) + (interval * 1000);
#elif VC4_TIMER_CHANNEL == 1
  *REG32(ST_C1) = *REG32(ST_CLO) + (interval * 1000);
#else
#error unsupported timer channel
#endif
  return NO_ERROR;
}

void platform_stop_timer(void) {
}

static enum handler_return timer0_irq(void *arg) {
  uint32_t cs = *REG32(ST_CS);
  *REG32(ST_CS) = cs; // ack the event so the irq signal clears
  assert(timer_cb);
  return timer_cb(timer_arg, current_time());
}
