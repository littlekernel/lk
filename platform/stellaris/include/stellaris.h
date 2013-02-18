#ifndef __stellaris_h__
#define __stellaris_h__

/* an attempt at a CMSIS compatibility layer for Stellaris */

#include "../ti/inc/hw_ints.h"

typedef enum IRQn {
    // base Cortex IRQs
    NonMaskableInt_IRQn   = FAULT_NMI-16,
    MemoryManagement_IRQn = FAULT_MPU-16,
    BusFault_IRQn         = FAULT_BUS-16,
    UsageFault_IRQn       = FAULT_USAGE-16,
    SVCall_IRQn           = FAULT_SVCALL-16,
    DebugMonitor_IRQn     = FAULT_DEBUG-16,
    PendSV_IRQn           = FAULT_PENDSV-16,
    SysTick_IRQn          = FAULT_SYSTICK-16
} IRQn_Type;

// based on datasheet page 159?
#define __NVIC_PRIO_BITS 3

#endif
