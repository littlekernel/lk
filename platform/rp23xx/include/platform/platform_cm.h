#pragma once

#define __CM33_REV             0x0100U
#define __NVIC_PRIO_BITS       4
#define __Vendor_SysTickConfig 0
#define __VTOR_PRESENT         1
#define __MPU_PRESENT          1
#define __FPU_PRESENT          1
#define __FPU_DP               0
#define __DSP_PRESENT          1
#define __SAUREGION_PRESENT    1

typedef enum {
  Reset_IRQn                = -15,
  NonMaskableInt_IRQn       = -14,
  HardFault_IRQn            = -13,
  MemoryManagement_IRQn     = -12,
  BusFault_IRQn             = -11,
  UsageFault_IRQn           = -10,
  SecureFault_IRQn          =  -9,
  SVCall_IRQn               =  -5,
  DebugMonitor_IRQn         =  -4,
  PendSV_IRQn               =  -2,
  SysTick_IRQn              =  -1,
#define RP23XX_IRQ(name,num) name##_IRQn = num,
#include <platform/irqinfo.h>
#undef RP23XX_IRQ
} IRQn_Type;
