#pragma once

#define __CM0PLUS_REV          0x0001U
#define __NVIC_PRIO_BITS       2
#define __Vendor_SysTickConfig 0
#define __VTOR_PRESENT         1
#define __MPU_PRESENT          1
#define __FPU_PRESENT          0

typedef enum {
  Reset_IRQn                = -15,
  NonMaskableInt_IRQn       = -14,
  HardFault_IRQn            = -13,
  SVCall_IRQn               =  -5,
  PendSV_IRQn               =  -2,
  SysTick_IRQn              =  -1,
#define RP20XX_IRQ(name,num) name##_IRQn = num,
#include <platform/irqinfo.h>
#undef RP20XX_IRQ
} IRQn_Type;
