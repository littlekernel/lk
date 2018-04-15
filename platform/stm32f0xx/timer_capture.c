/*
 * Copyright (c) 2017 The Fuchsia Authors.
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

#include <platform/timer_capture.h>

#include <arch/arm/cm.h>
#include <assert.h>
#include <platform/rcc.h>

typedef enum {
    STM32_TIM_NOT_FOUND = -1,
    STM32_TIM1_INDEX = 0,
    STM32_TIM2_INDEX,
    STM32_TIM3_INDEX,
    STM32_TIM6_INDEX,
    STM32_TIM7_INDEX,
    STM32_TIM14_INDEX,
    STM32_TIM15_INDEX,
    STM32_TIM16_INDEX,
    STM32_TIM17_INDEX,
    STM32_NUM_TIMERS,
} stm32_timer_index_t;

typedef TIM_TypeDef stm32_timer_regs_t;

#define STM32_TIMER_FLAGS_32_BIT (1 << 0)
typedef struct stm32_timer_config_ {
    stm32_timer_regs_t *regs;
    uint32_t flags;
    stm32_rcc_clk_t clock;
    int irq;
} stm32_timer_config_t;

static const stm32_timer_config_t stm32_timer_config[] = {
#ifdef TIM1
    [STM32_TIM1_INDEX] = {
        .regs = TIM1,
        .flags = 0,
        .clock = STM32_RCC_CLK_TIM1,
        // TIM1 has two interrupts.  This one and TIM1_BRK_UP_TRG_COM_IRQn.
        // This will need to be supported when more than just capture is
        // supported.
        .irq = TIM1_CC_IRQn,
    },
#endif
#ifdef TIM2
    [STM32_TIM2_INDEX] = {
        .regs = TIM2,
        .flags = STM32_TIMER_FLAGS_32_BIT,
        .clock = STM32_RCC_CLK_TIM2,
        .irq = TIM2_IRQn,
    },
#endif
#ifdef TIM3
    [STM32_TIM3_INDEX] = {
        .regs = TIM3,
        .flags = 0,
        .clock = STM32_RCC_CLK_TIM3,
        .irq = TIM3_IRQn,
    },
#endif
#ifdef TIM6
    [STM32_TIM6_INDEX] = {
        .regs = TIM6,
        .flags = 0,
        .clock = STM32_RCC_CLK_TIM6,
        .irq = TIM6_IRQn,
    },
#endif
#ifdef TIM7
    [STM32_TIM7_INDEX] = {
        .regs = TIM7,
        .flags = 0,
        .clock = STM32_RCC_CLK_TIM7,
        .irq = TIM7_IRQn,
    },
#endif
#ifdef TIM14
    [STM32_TIM14_INDEX] = {
        .regs = TIM14,
        .flags = 0,
        .clock = STM32_RCC_CLK_TIM14,
        .irq = TIM14_IRQn,
    },
#endif
#ifdef TIM15
    [STM32_TIM15_INDEX] = {
        .regs = TIM15,
        .flags = 0,
        .clock = STM32_RCC_CLK_TIM15,
        .irq = TIM15_IRQn,
    },
#endif
#ifdef TIM16
    [STM32_TIM16_INDEX] = {
        .regs = TIM16,
        .flags = 0,
        .clock = STM32_RCC_CLK_TIM16,
        .irq = TIM16_IRQn,
    },
#endif
#ifdef TIM17
    [STM32_TIM17_INDEX] = {
        .regs = TIM17,
        .flags = 0,
        .clock = STM32_RCC_CLK_TIM17,
        .irq = TIM17_IRQn,
    },
#endif
};

static stm32_timer_capture_t *stm32_timer_capture_data[STM32_NUM_TIMERS];

static void stm32_timer_enable(const stm32_timer_config_t *config) {
    stm32_rcc_set_enable(config->clock, true);
}

static stm32_timer_index_t stm32_timer_get_index(int timer) {
    switch (timer) {
        case 1:
            return STM32_TIM1_INDEX;
        case 2:
            return STM32_TIM2_INDEX;
        case 3:
            return STM32_TIM3_INDEX;
        case 6:
            return STM32_TIM6_INDEX;
        case 7:
            return STM32_TIM7_INDEX;
        case 14:
            return STM32_TIM14_INDEX;
        case 15:
            return STM32_TIM15_INDEX;
        case 16:
            return STM32_TIM16_INDEX;
        case 17:
            return STM32_TIM17_INDEX;
        default:
            return STM32_TIM_NOT_FOUND;
    }
}

static const stm32_timer_config_t *stm32_timer_get_config(int timer) {
    stm32_timer_index_t index = stm32_timer_get_index(timer);
    if (index == STM32_TIM_NOT_FOUND) {
        return NULL;
    }
    return &stm32_timer_config[index];
}

static bool stm32_timer_is_32_bit(int timer) {
    switch (timer) {
        case 2:
            return true;
        default:
            return false;
    }
}

static void stm32_timer_capture_setup_chan(stm32_timer_capture_t *tc, int chan) {
    stm32_timer_regs_t *regs = tc->config->regs;
    volatile uint32_t *ccmr = chan & 0x2 ? &regs->CCMR2 : &regs->CCMR1;
    int shift = 8 * (chan & 1);

    uint32_t val = *ccmr;
    val &= 0xff << shift;
    val |= TIM_CCMR1_CC1S_0 << shift;
    *ccmr = val;

    uint32_t config = TIM_CCER_CC1E;
    uint32_t flags = tc->chan[chan].flags;
    if (flags & STM32_TIMER_CAPTURE_CHAN_FLAG_FALLING) {
        config |= TIM_CCER_CC1P;
        if (flags & STM32_TIMER_CAPTURE_CHAN_FLAG_RISING) {
            config |= TIM_CCER_CC1NP;
        }
    }

    shift = 4 * (chan & 3);
    val = regs->CCER;
    val &= 0xf << shift;
    val |= config << shift;
    regs->CCER = val;
}

static uint32_t stm32_timer_get_ccr(stm32_timer_capture_t *tc, int chan) {
    assert(0 <= chan && chan <= 3);
    switch (chan) {
        case 0:
            return tc->config->regs->CCR1;
        case 1:
            return tc->config->regs->CCR2;
        case 2:
            return tc->config->regs->CCR3;
        default:  // 3
            return tc->config->regs->CCR4;
    }
}

static uint64_t stm32_timer_inc_overflow(stm32_timer_capture_t *tc, uint64_t overflow) {
    uint64_t inc;
    if (tc->config->flags & STM32_TIMER_FLAGS_32_BIT) {
        inc = 0x100000000ULL;
    } else {
        inc = 0x10000ULL;
    }
    return overflow + inc;
}

static uint32_t stm32_timer_median_val(stm32_timer_capture_t *tc) {
    if (tc->config->flags & STM32_TIMER_FLAGS_32_BIT) {
     return 0x7fffffff;
    } else {
     return 0x7fff;
    }
}

// Assumes interrupts are disabled.
static uint64_t stm32_timer_calc_value(stm32_timer_capture_t *tc, uint32_t sr, uint32_t val) {
        uint32_t overflow = tc->overflow;
        // Since we could be processing an overflow and capture interrupts at the
        // same time, we don't know the ordering of the two.  Here we assume
        // that if the capture event occurred in the lower half of the counter
        // range, the overflow happened before the capture.
        if (sr & TIM_SR_UIF && val < stm32_timer_median_val(tc)) {
            overflow = stm32_timer_inc_overflow(tc, overflow);
        }
        return overflow | val;
}

static bool stm32_timer_capture_chan_irq(stm32_timer_capture_t *tc, uint32_t sr, int chan) {
    if (tc->chan[chan].cb == NULL) {
        return false;
    }
    if (sr & (TIM_SR_CC1IF << chan)) {
        uint32_t val = stm32_timer_get_ccr(tc, chan);
        return tc->chan[chan].cb(stm32_timer_calc_value(tc, sr, val));
    }
    return false;
}

static void stm32_timer_capture_irq(stm32_timer_index_t index) {
    arm_cm_irq_entry();
    bool resched = false;

    stm32_timer_capture_t *tc = stm32_timer_capture_data[index];
    if (tc == NULL) {
        goto out;
    }

    uint32_t sr = tc->config->regs->SR;
    uint32_t irq_ack = ~0U;

    // Since these read their corresponding CCR registers, the status flags
    // do not need to be explicitly cleared.
    resched |= stm32_timer_capture_chan_irq(tc, sr, 0);
    resched |= stm32_timer_capture_chan_irq(tc, sr, 1);
    resched |= stm32_timer_capture_chan_irq(tc, sr, 2);
    resched |= stm32_timer_capture_chan_irq(tc, sr, 3);


    // We process overflow after compares in order to handle overflow race
    // detection there.
    spin_lock_saved_state_t state;
    spin_lock_irqsave(&tc->overflow_lock, state);
    if (sr & TIM_SR_UIF) {
        tc->overflow = stm32_timer_inc_overflow(tc, tc->overflow);
        irq_ack &= ~TIM_SR_UIF;
    }
    tc->config->regs->SR = irq_ack;
    spin_unlock_irqrestore(&tc->overflow_lock, state);

out:
    arm_cm_irq_exit(resched);
}

void stm32_TIM1_IRQ(void) {
    stm32_timer_capture_irq(STM32_TIM1_INDEX);
}

void stm32_TIM2_IRQ(void) {
    stm32_timer_capture_irq(STM32_TIM2_INDEX);
}

void stm32_TIM3_IRQ(void) {
    stm32_timer_capture_irq(STM32_TIM3_INDEX);
}

void stm32_TIM6_IRQ(void) {
    stm32_timer_capture_irq(STM32_TIM6_INDEX);
}

void stm32_TIM7_IRQ(void) {
    stm32_timer_capture_irq(STM32_TIM7_INDEX);
}

void stm32_TIM14_IRQ(void) {
    stm32_timer_capture_irq(STM32_TIM14_INDEX);
}

void stm32_TIM15_IRQ(void) {
    stm32_timer_capture_irq(STM32_TIM15_INDEX);
}

void stm32_TIM16_IRQ(void) {
    stm32_timer_capture_irq(STM32_TIM16_INDEX);
}

void stm32_TIM17_IRQ(void) {
    stm32_timer_capture_irq(STM32_TIM17_INDEX);
}

status_t stm32_timer_capture_setup(stm32_timer_capture_t *tc, int timer, uint16_t prescaler) {
    tc->config = stm32_timer_get_config(timer);
    if (tc->config == NULL) {
        return ERR_NOT_FOUND;
    }

    stm32_timer_enable(tc->config);

    stm32_timer_capture_data[stm32_timer_get_index(timer)] = tc;

    tc->overflow = 0;
    spin_lock_init(&tc->overflow_lock);

    tc->config->regs->CR1 = 0;
    tc->config->regs->PSC = prescaler;

    uint32_t dier =  TIM_DIER_UIE;

    int i;
    for (i = 0; i < 4; i++) {
        if (tc->chan[i].flags & STM32_TIMER_CAPTURE_CHAN_FLAG_ENABLE) {
            dier |= TIM_DIER_CC1IE << i;
            stm32_timer_capture_setup_chan(tc, i);
        }
    }

    tc->config->regs->DIER = dier;
    tc->config->regs->CR1 |= TIM_CR1_CEN;

    NVIC_EnableIRQ(tc->config->irq);

    return NO_ERROR;
}

uint64_t stm32_timer_capture_get_counter(stm32_timer_capture_t *tc) {
    // Protect against tc->overflow being updated while we calculate the value.
    spin_lock_saved_state_t state;
    spin_lock_irqsave(&tc->overflow_lock, state);

    uint32_t cnt = tc->config->regs->CNT;
    uint32_t sr = tc->config->regs->SR;
    uint64_t value = stm32_timer_calc_value(tc, sr, cnt);

    spin_unlock_irqrestore(&tc->overflow_lock, state);
    return value;
}

void stm32_timer_init(void) {
}

void stm32_timer_early_init(void) {
}
