/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

/*
 * CMSIS core configuration for the MPS2/MPS3 FPGA images. There is no vendor
 * device package for these boards, so describe the cores by hand.
 *
 * The FPGA images implement the architectural minimum of three priority bits.
 * qemu implements all eight, but using three keeps us within what the real
 * boards do and matches the fallback in arch/arm/cm.h.
 */
#define __NVIC_PRIO_BITS       3
#define __Vendor_SysTickConfig 0
#define __VTOR_PRESENT         1
#define __MPU_PRESENT          1

#if ARM_CPU_CORTEX_M3
#define __CM3_REV 0x0201U
#define __FPU_PRESENT 0
#elif ARM_CPU_CORTEX_M4
#define __CM4_REV 0x0001U
#define __FPU_PRESENT 1
#elif ARM_CPU_CORTEX_M7
#define __CM7_REV 0x0100U
#define __FPU_PRESENT 1
#define __FPU_DP 0
/* arch/arm/arm-m/cache.c reaches for the SCB cache ops, which CMSIS only
 * declares when the core is described as having caches. */
#define __ICACHE_PRESENT 1
#define __DCACHE_PRESENT 1
#elif ARM_CPU_CORTEX_M33
#define __CM33_REV 0x0000U
#define __FPU_PRESENT 1
#define __DSP_PRESENT 1
#define __SAUREGION_PRESENT 0
#elif ARM_CPU_CORTEX_M55
#define __CM55_REV 0x0001U
#define __FPU_PRESENT 1
#define __FPU_DP 1
#define __DSP_PRESENT 1
#define __MVE_PRESENT 1
#define __SAUREGION_PRESENT 0
#define __ICACHE_PRESENT 1
#define __DCACHE_PRESENT 1
#else
#error "unknown cortex-m core for the mps2 platform"
#endif

/*
 * Board interrupt numbers as the cpu sees them, from the qemu board models in
 * hw/arm/mps2.c and hw/arm/mps2-tz.c, along with the number of external
 * interrupt lines the NVIC is configured with.
 *
 * These are macros as well as enum values below because the vector table is
 * laid out around them at preprocessing time. Only the interrupts the
 * platform actually drives are named.
 */
#if MPS2_MACHINE_AN385 || MPS2_MACHINE_AN386 || MPS2_MACHINE_AN500
#define MPS2_UART0_RX_IRQ 0
#define MPS2_UART0_TX_IRQ 1
#define MPS2_NUM_IRQS     32
#elif MPS2_MACHINE_AN505
/* the SSE takes the low 32 interrupts, board devices start above them */
#define MPS2_UART0_RX_IRQ 32
#define MPS2_UART0_TX_IRQ 33
#define MPS2_NUM_IRQS     124
#elif MPS2_MACHINE_AN547
#define MPS2_UART0_RX_IRQ 33
#define MPS2_UART0_TX_IRQ 34
#define MPS2_NUM_IRQS     128
#else
#error "unknown mps2 machine"
#endif

/*
 * Interrupt numbers. The negative entries are the architectural exceptions,
 * the positive ones the board interrupts named above.
 */
typedef enum {
    Reset_IRQn            = -15,
    NonMaskableInt_IRQn   = -14,
    HardFault_IRQn        = -13,
    MemoryManagement_IRQn = -12,
    BusFault_IRQn         = -11,
    UsageFault_IRQn       = -10,
#if ARM_ISA_ARMV8M
    SecureFault_IRQn      =  -8,
#endif
    SVCall_IRQn           =  -5,
    DebugMonitor_IRQn     =  -4,
    PendSV_IRQn           =  -2,
    SysTick_IRQn          =  -1,

    UARTRX0_IRQn          = MPS2_UART0_RX_IRQ,
    UARTTX0_IRQn          = MPS2_UART0_TX_IRQ,
} IRQn_Type;
