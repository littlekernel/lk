/*
 * Copyright (c) 2015 MediaTek Inc.
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <sys/types.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/reg.h>
#include <kernel/thread.h>
#include <mt_gic.h>
#include <platform/timer.h>
#include <platform/mt_typedefs.h>
#include <platform/mt_reg_base.h>
#include <platform/mt_gpt.h>
#include <platform/mt_irq.h>

#define TIMER_TICK_RATE 32768

static volatile lk_time_t ticks = 0;
static lk_time_t tick_interval;
static platform_timer_callback time_callback;
static void *callback_arg;

static enum handler_return timer_irq(void *arg) {
    ticks += tick_interval;
    return time_callback(callback_arg, ticks);
}

enum handler_return lk_scheduler(void) {
    static enum handler_return ret;

    /* ack GPT5 irq */
    DRV_WriteReg32(GPT_IRQACK_REG, 0x10);
    DRV_WriteReg32(GPT5_CON_REG, GPT_CLEAR);
    DRV_WriteReg32(GPT5_CON_REG, GPT_DISABLE);

    ret = timer_irq(0);

    /* ack GIC irq */
    mt_irq_ack(MT_GPT_IRQ_ID);

    /* enable GPT5 */
    DRV_WriteReg32(GPT5_CON_REG, GPT_ENABLE|GPT_MODE4_ONE_SHOT);

    return ret;
}

status_t platform_set_periodic_timer(platform_timer_callback callback, void *arg, lk_time_t interval) {
    time_callback = callback;
    tick_interval = interval;
    callback_arg  = arg;

    DRV_WriteReg32(GPT_IRQEN_REG, 0);
    DRV_WriteReg32(GPT_IRQACK_REG, 0x3f);

    mt_irq_set_sens(MT_GPT_IRQ_ID, MT65xx_LEVEL_SENSITIVE);
    mt_irq_set_polarity(MT_GPT_IRQ_ID, MT65xx_POLARITY_LOW);

    DRV_WriteReg32(GPT5_CON_REG, 0x02);
    DRV_WriteReg32(GPT_IRQACK_REG, 0x10);
    DRV_WriteReg32(GPT5_CLK_REG, 0x10);

    DRV_WriteReg32(GPT5_COMPARE_REG, TIMER_TICK_RATE*interval/1000);
    DRV_WriteReg32(GPT_IRQEN_REG, 0x10);

    mt_irq_unmask(MT_GPT_IRQ_ID);

    DRV_WriteReg32(GPT5_CON_REG, GPT_ENABLE|GPT_MODE4_ONE_SHOT);

    return NO_ERROR;
}

lk_time_t current_time(void) {
    return ticks;
}

lk_bigtime_t current_time_hires(void) {
    lk_bigtime_t time;

    time = (lk_bigtime_t)ticks * 1000;
    return time;
}

