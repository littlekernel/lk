//
// Copyright (c) 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
//
#include <assert.h>
#include <arch/interrupts.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/trace.h>
#include <platform/interrupts.h>
#include <platform/time.h>
#include <platform/timer.h>
#include <sys/types.h>

#include "platform_p.h"

#define LOCAL_TRACE 0

// sun4m timer

namespace {

// each cpu offset is 0x1000, only handle UP for now
constexpr uint64_t TIMER_PERCPU_PHYS = 0xff1300000ULL;
constexpr uint64_t TIMER_GLOBAL_PHYS = 0xff1310000ULL;

// free running timer, running in 500nS increments
constexpr uint32_t TIMER_FREQ = 2 * 1000 * 1000;
constexpr lk_time_t TIMER_PERIOD = 10;
constexpr uint32_t TIMER_TICKS_PER_PERIOD = TIMER_FREQ / (1000 / TIMER_PERIOD); // 10mS period
constexpr uint32_t TIMER_WRAP_BIT = 0x80000000u;
constexpr uint32_t TIMER_COUNTER_MASK = ~TIMER_WRAP_BIT;
constexpr uint32_t TIMER_COUNTER_SHIFT = 9;
constexpr uint32_t TIMER_COUNTER_GRANULARITY_MASK = (1u << 9) - 1;
constexpr uint32_t TIMER_TICKS_PER_US = TIMER_FREQ / (1000 * 1000);
constexpr uint32_t TIMER_TICKS_PER_MS = TIMER_FREQ / 1000;
constexpr uint32_t TIMER_MAX_TICKS = TIMER_COUNTER_MASK >> TIMER_COUNTER_SHIFT;
constexpr uint32_t TIMER_GLOBAL_FREE_RUNNING_LIMIT =
    TIMER_COUNTER_MASK & ~TIMER_COUNTER_GRANULARITY_MASK;

uint32_t timer_ticks = 0;
uint64_t global_timer_wrap_ticks = 0;
bool per_cpu_timer_oneshot = false;
platform_timer_callback timer_callback = nullptr;
void *timer_callback_arg = nullptr;

// timer registers
enum {
    // limit register, write to this to set the timer period (bits 30-9), read to clear the limit bit
    // (bit 31)
    TIMER_LIMIT_REG = 0,
    // counter register, read to get the current counter value (bits 30-0), bit 31 is the wrap bit
    TIMER_COUNTER_REG = 4,
    // limit register, write to this to set the timer period (bits 30-9), read does not clear limit bit
    TIMER_LIMIT_NO_RESET_REG = 8,
    // user start/stop register, write 1 to bit 0 to start the timer, write 1 to bit 1 to stop the
    // timer, only if configured as user
    TIMER_USER_START_STOP_REG = 0xc,
};

uint64_t read_global_timer_ticks() {
    arch_interrupt_saved_state_t state = arch_interrupt_save();

    // The global timer runs without a routed interrupt. If its limit bit latches,
    // clear it by reading the limit register and account for one wrapped interval.
    uint32_t cnt = sparc_read_physical_32(TIMER_GLOBAL_PHYS + TIMER_COUNTER_REG);
    if (cnt & TIMER_WRAP_BIT) {
        sparc_read_physical_32(TIMER_GLOBAL_PHYS + TIMER_LIMIT_REG);
        global_timer_wrap_ticks += (TIMER_GLOBAL_FREE_RUNNING_LIMIT >> TIMER_COUNTER_SHIFT);
    }

    uint64_t ticks = global_timer_wrap_ticks + ((cnt & TIMER_COUNTER_MASK) >> TIMER_COUNTER_SHIFT);

    arch_interrupt_restore(state);
    return ticks;
}

uint32_t interval_ms_to_timer_limit(lk_time_t interval) {
    uint64_t ticks = static_cast<uint64_t>(interval) * TIMER_TICKS_PER_MS;
    if (ticks == 0) {
        ticks = 1;
    }
    if (ticks > TIMER_MAX_TICKS) {
        ticks = TIMER_MAX_TICKS;
    }

    return static_cast<uint32_t>(ticks << TIMER_COUNTER_SHIFT);
}

} // anonymous namespace

void sun4m_timer_early_init(void) {
    // Start with the per-cpu timer in free-run mode and arm it when requested.
    sparc_write_physical_32(TIMER_PERCPU_PHYS + TIMER_LIMIT_REG, 0);
    sparc_read_physical_32(TIMER_PERCPU_PHYS + TIMER_LIMIT_REG);

    // Program the global timer as a free-running source used by current_time().
    // No interrupt is routed for it; readers clear the latched wrap condition.
    sparc_write_physical_32(TIMER_GLOBAL_PHYS + TIMER_LIMIT_NO_RESET_REG,
                            TIMER_GLOBAL_FREE_RUNNING_LIMIT);
    global_timer_wrap_ticks = 0;
    sparc_read_physical_32(TIMER_GLOBAL_PHYS + TIMER_LIMIT_REG);
}

void sun4m_timer_init(void) {
    // nothing to do here
}

handler_return sun4m_timer_irq() {
    handler_return ret = INT_NO_RESCHEDULE;

    // read the current limit register and check if the wrap bit is set,
    // if so, increment the tick count and call our callback if one is registered.
    uint32_t counter = sparc_read_physical_32(TIMER_PERCPU_PHYS + TIMER_COUNTER_REG);
    // printf("limit: counter 0x%x\n", counter);
    if (counter & TIMER_WRAP_BIT) {
        // printf("tick %u\n", timer_ticks);

        // read the limit register to clear the wrap bit
        sparc_read_physical_32(TIMER_PERCPU_PHYS + TIMER_LIMIT_REG);

        if (per_cpu_timer_oneshot) {
            // Disable matching before callback so callback can safely re-arm.
            sparc_write_physical_32(TIMER_PERCPU_PHYS + TIMER_LIMIT_REG, 0);
        }

        timer_ticks++;
        if (timer_callback) {
            if (timer_callback(timer_callback_arg, current_time()) == INT_RESCHEDULE) {
                ret = INT_RESCHEDULE;
            }
        }
    }
    return ret;
}

status_t platform_set_oneshot_timer(platform_timer_callback callback, void *arg,
                                    lk_time_t interval) {
    LTRACEF("callback %p, arg %p, interval %u\n", callback, arg, interval);

    arch_interrupt_saved_state_t state = arch_interrupt_save();

    timer_callback = callback;
    timer_callback_arg = arg;
    per_cpu_timer_oneshot = true;

    sparc_write_physical_32(TIMER_PERCPU_PHYS + TIMER_LIMIT_REG,
                            interval_ms_to_timer_limit(interval));

    arch_interrupt_restore(state);
    return NO_ERROR;
}

status_t platform_set_periodic_timer(platform_timer_callback callback, void *arg,
                                     lk_time_t interval) {
    TRACEF("callback %p, arg %p, interval %u\n", callback, arg, interval);

    arch_interrupt_saved_state_t state = arch_interrupt_save();

    timer_callback = callback;
    timer_callback_arg = arg;
    per_cpu_timer_oneshot = false;

    sparc_write_physical_32(TIMER_PERCPU_PHYS + TIMER_LIMIT_REG,
                            interval_ms_to_timer_limit(interval));

    arch_interrupt_restore(state);
    return NO_ERROR;
}

void platform_stop_timer(void) {
    arch_interrupt_saved_state_t state = arch_interrupt_save();

    sparc_write_physical_32(TIMER_PERCPU_PHYS + TIMER_LIMIT_REG, 0);
    sparc_read_physical_32(TIMER_PERCPU_PHYS + TIMER_LIMIT_REG);

    arch_interrupt_restore(state);
}

lk_time_t current_time(void) {
    uint64_t ticks = read_global_timer_ticks();
    return ticks / TIMER_TICKS_PER_MS;
}

lk_bigtime_t current_time_hires(void) {
    uint64_t ticks = read_global_timer_ticks();
    return ticks / TIMER_TICKS_PER_US;
}
