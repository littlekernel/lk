/*
 * Copyright (c) 2015 Stefan Kristiansson
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <trace.h>
#include <err.h>
#include <kernel/thread.h>
#include <platform/timer.h>
#include <platform/debug.h>
#include <sys/types.h>
#include <arch/or1k.h>

#define LOCAL_TRACE 0

static platform_timer_callback timer_cb;
static void *timer_arg;

static uint32_t timer_freq;

static volatile uint64_t ticks = 0;

status_t platform_set_periodic_timer(platform_timer_callback callback, void *arg, lk_time_t interval)
{
    LTRACEF("cb %p, arg %p, interval %ld\n", callback, arg, interval);

    uint32_t ttmr = (uint64_t)timer_freq * interval / 1000;
    LTRACEF("count 0x%x\n", ttmr);

    timer_cb = callback;
    timer_arg = arg;

    /* disable timer before doing changes */
    mtspr(OR1K_SPR_TICK_TTMR_ADDR, 0);
    /* reset timer counter */
    mtspr(OR1K_SPR_TICK_TTCR_ADDR, 0);
    /* enable timer with given interval in 'restart' mode */
    ttmr = OR1K_SPR_TICK_TTMR_MODE_SET(ttmr | OR1K_SPR_TICK_TTMR_IE_MASK,
                                       OR1K_SPR_TICK_TTMR_MODE_RESTART);
    mtspr(OR1K_SPR_TICK_TTMR_ADDR, ttmr);

    return NO_ERROR;
}

lk_bigtime_t current_time_hires(void)
{
    return (lk_bigtime_t)ticks * 10000;
}

lk_time_t current_time(void)
{
    return (lk_time_t)ticks * 10;
}

enum handler_return platform_tick(void)
{
    ticks++;

    /* clear pending interrupt flag */
    mtspr(OR1K_SPR_TICK_TTMR_ADDR,
          mfspr(OR1K_SPR_TICK_TTMR_ADDR) & ~(OR1K_SPR_TICK_TTMR_IP_MASK));

    return timer_cb(timer_arg, ticks * 10);
}

void or1k_ticktimer_init(uint32_t freq)
{
    timer_freq = freq;
    /* disable timer */
    mtspr(OR1K_SPR_TICK_TTMR_ADDR, 0);
    /* reset timer counter */
    mtspr(OR1K_SPR_TICK_TTCR_ADDR, 0);
    /* clear pending interrupt flag */
    mtspr(OR1K_SPR_TICK_TTMR_ADDR,
          mfspr(OR1K_SPR_TICK_TTMR_ADDR) & ~(OR1K_SPR_TICK_TTMR_IP_MASK));
}
