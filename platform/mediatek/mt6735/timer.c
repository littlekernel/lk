/*
 * Copyright (c) 2008 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <sys/types.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/reg.h>
#include <platform/timer.h>
#include <platform/mt_typedefs.h>
#include <platform/mt_reg_base.h>
#include <platform/mt_gpt.h>
#include <platform/mt_irq.h>

#include <kernel/thread.h>

static volatile lk_time_t ticks = 0;
static lk_time_t tick_interval;

static platform_timer_callback time_callback;
static void *callback_arg;

#define AP_PERI_GLOBALCON_PDN0 (PERICFG_BASE+0x10)

#define TIMER_TICK_RATE 32768

static enum handler_return timer_irq(void *arg) {
    ticks += tick_interval;
    return time_callback(callback_arg, ticks);
}

void lk_scheduler(void) {
    //static enum handler_return ret;

    /* ack GPT5 irq */
    DRV_WriteReg32(GPT_IRQACK_REG, 0x10);
    DRV_WriteReg32(GPT5_CON_REG, GPT_CLEAR);
    DRV_WriteReg32(GPT5_CON_REG, GPT_DISABLE);

    timer_irq(0);

    /*
     * CAUTION! The de-assert signal to GIC might delay serveral clocks.
     * Here must have enough delay to make sure the GPT signal had arrived GIC.
     */
    /* ack GIC irq */
    mt_irq_ack(MT_GPT_IRQ_ID);

    /* enable GPT5 */
    DRV_WriteReg32(GPT5_CON_REG, GPT_ENABLE|GPT_MODE4_ONE_SHOT);

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

static void gpt_power_on (bool bPowerOn) {
    if (!bPowerOn) {
        DRV_SetReg32(AP_PERI_GLOBALCON_PDN0, 1<<13);
    } else {
        DRV_ClrReg32(AP_PERI_GLOBALCON_PDN0, 1<<13);
    }
}

void platform_early_init_timer(void) {
    //dprintf(SPEW, "platform_early_init_timer\n");

    gpt_power_on(1);
}
