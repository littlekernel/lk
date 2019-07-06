/*
 * Copyright (c) 2015 Brian Swetland
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#pragma once

#define __CM4_REV       1
#define __MPU_PRESENT       1
#define __NVIC_PRIO_BITS    3
#define __Vendor_SysTickConfig  0
#define __FPU_PRESENT       1

#define DEFIRQ(x) x##_IRQn,
typedef enum {
    Reset_IRQn = -15,
    NonMaskableInt_IRQn = -14,
    HardFault_IRQn = -13,
    MemoryManagement_IRQn = -12,
    BusFault_IRQn = -11,
    UsageFault_IRQn = -10,
    SVCall_IRQn = -5,
    DebugMonitor_IRQn = -4,
    PendSV_IRQn = -2,
    SysTick_IRQn = -1,
#include <platform/defirq.h>
} IRQn_Type;
#undef DEFIRQ

