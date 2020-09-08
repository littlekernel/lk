#include <platform/timer.h>
#include <platform/bcm28xx.h>
#include <lk/reg.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <platform/interrupts.h>
#include <assert.h>

static enum handler_return timer0_irq(void *arg);

lk_bigtime_t current_time_hires(void) {
  //TODO, deal with rollover
  return ( ((lk_bigtime_t)*REG32(ST_CHI)) << 32) | *REG32(ST_CLO);
}

lk_time_t current_time(void) {
  return current_time_hires();
}

static platform_timer_callback timer_cb = 0;;
static void *timer_arg = 0;;

status_t platform_set_oneshot_timer (platform_timer_callback callback, void *arg, lk_time_t interval) {
  timer_cb = callback;
  timer_arg = arg;
  *REG32(ST_C0) = *REG32(ST_CLO) + (interval * 1000);
  register_int_handler(0, timer0_irq, NULL);
  unmask_interrupt(0);

  return NO_ERROR;
}

static enum handler_return timer0_irq(void *arg) {
  uint32_t cs = *REG32(ST_CS);
  *REG32(ST_CS) = cs; // ack the event so the irq signal clears
  assert(timer_cb);
  return timer_cb(timer_arg, current_time());
}

void platform_stop_timer(void) {
}
