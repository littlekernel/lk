/*
 * Copyright (c) 2017 The Fuchsia Authors.
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lk/err.h>
#include <kernel/spinlock.h>
#include <stdbool.h>
#include <stdint.h>
#include <stm32f0xx.h>

#define STM32_TIMER_CAPTURE_CHAN_FLAG_ENABLE  (1 << 0)
#define STM32_TIMER_CAPTURE_CHAN_FLAG_RISING  (1 << 1)
#define STM32_TIMER_CAPTURE_CHAN_FLAG_FALLING (1 << 2)

typedef struct {
    uint32_t flags;
    bool (*cb)(uint64_t val);
} stm32_timer_capture_channel_t;

typedef struct stm32_timer_config_ stm32_timer_config_t;

typedef struct {
    const stm32_timer_config_t    *config;
    stm32_timer_capture_channel_t chan[4];

    volatile uint64_t overflow;
    spin_lock_t overflow_lock;
} stm32_timer_capture_t;

// Set tc->chan[] cb and flags before calling.
//
// `prescaler` is the desired prescaling factor and must be positive. The value stored in the 
// prescaler register will be `prescaler - 1` (see ST reference manual RM0091, section 18.4.11).
status_t stm32_timer_capture_setup(stm32_timer_capture_t *tc, int timer, uint16_t prescaler);

uint64_t stm32_timer_capture_get_counter(stm32_timer_capture_t *tc);

