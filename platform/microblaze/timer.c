/*
 * Copyright (c) 2015 Travis Geiselbrecht
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
#include <reg.h>
#include <trace.h>
#include <err.h>
#include <lk/init.h>
#include <kernel/thread.h>
#include <platform.h>
#include <platform/timer.h>
#include <platform/interrupts.h>
#include <platform/debug.h>
#include <sys/types.h>
#include <target/microblaze-config.h>

#define LOCAL_TRACE 0

#define R_TCSR     0
#define R_TLR      1
#define R_TCR      2
#define R_MAX      4

#define TCSR_MDT        (1<<0)
#define TCSR_UDT        (1<<1)
#define TCSR_GENT       (1<<2)
#define TCSR_CAPT       (1<<3)
#define TCSR_ARHT       (1<<4)
#define TCSR_LOAD       (1<<5)
#define TCSR_ENIT       (1<<6)
#define TCSR_ENT        (1<<7)
#define TCSR_TINT       (1<<8)
#define TCSR_PWMA       (1<<9)
#define TCSR_ENALL      (1<<10)

#define TIMER_REG(reg) (*REG32(TIMER_BASEADDR + (reg) * 4))

static platform_timer_callback timer_cb;
static void *timer_arg;

static uint32_t ticks = 0;

status_t platform_set_periodic_timer(platform_timer_callback callback, void *arg, lk_time_t interval)
{
    LTRACEF("cb %p, arg %p, interval %u\n", callback, arg, interval);

    uint32_t count = ((uint64_t)TIMER_RATE * interval / 1000);

    LTRACEF("count 0x%x\n", count);

    timer_cb = callback;
    timer_arg = arg;

    TIMER_REG(R_TCSR) = 0;
    TIMER_REG(R_TLR) = count;
    TIMER_REG(R_TCSR) = TCSR_ENIT | TCSR_UDT | TCSR_ARHT | TCSR_ENT;

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

enum handler_return timer_irq(void *arg)
{
    LTRACE;

    TIMER_REG(R_TCSR) |= TCSR_TINT;

    ticks += 1;

    enum handler_return ret = timer_cb(timer_arg, ticks * 10);

    return ret;
}

static void timer_init(uint level)
{
    LTRACE;

    register_int_handler(TIMER_IRQ, timer_irq, NULL);
    unmask_interrupt(TIMER_IRQ);
}

LK_INIT_HOOK(timer, timer_init, LK_INIT_LEVEL_PLATFORM_EARLY + 1);


