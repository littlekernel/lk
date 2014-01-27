#pragma once

#include <stdint.h>
#include <reg.h>

/* memory map */
#define NOR0_BASE       (0x08000000)
#define NOR1_BASE       (0x0c000000)
#define ETH_BASE        (0x1a000000)
#define SYSREG_BASE     (0x1c010000)
#define SYSCONTROL_BASE (0x1c020000)
#define UART0_BASE      (0x1c090000)
#define UART1_BASE      (0x1c0a0000)
#define UART2_BASE      (0x1c0b0000)
#define UART3_BASE      (0x1c0c0000)
#define WDOG_BASE       (0x1c0f0000)
#define POWER_BASE      (0x1c100000)
#define TIMER0_BASE     (0x1c110000)
#define TIMER1_BASE     (0x1c120000)
#define VIO_BLOCK_BASE  (0x1c130000)
#define RTC_BASE        (0x1c170000)
#define REFCLK_CNTControl  (0x2a430000)
#define REFCLK_CNTRead     (0x2a800000)
#define AP_REFCLK_CNTCTL   (0x2a810000)
#define AP_REFCLK_CNTBASE0 (0x2a820000)
#define AP_REFCLK_CNTBASE1 (0x2a830000)
#define GIC_BASE          (0x2c000000)
#define GIC_DISTRIB_BASE  (0x2c001000)
#define GIC_PROC_BASE     (0x2c002000)
#define GIC_PROC_HYP_BASE (0x2c004000)
#define GIC_HYP_BASE      (0x2c005000)
#define GIC_VCPU_BASE     (0x2c006000)

/* interrupts */
#define INT_PPI_VMAINT       (16+9)
#define INT_PPI_HYP_TIMER    (16+10)
#define INT_PPI_VIRT_TIMER   (16+11)
#define INT_PPI_SPHYS_TIMER  (16+13)
#define INT_PPI_NSPHYS_TIMER (16+14)

#define INT_TIMER0      34

#define MAX_INT 96

/* system control registers */
#define SYSREG_ID       (SYSREG_BASE + 0x00)
#define SYSREG_SWITCH   (SYSREG_BASE + 0x04)
#define SYSREG_LED      (SYSREG_BASE + 0x08)
#define SYSREG_CONFIG   (SYSREG_BASE + 0xa0)
#define SYSREG_CONTROL  (SYSREG_BASE + 0xa4)
#define SYSREG_STATUS   (SYSREG_BASE + 0xa8)

static inline void set_led(uint8_t led)
{
    *REG32(SYSREG_LED) = led;
}

static inline uint8_t read_switches(void)
{
    return *REG32(SYSREG_SWITCH) & 0xff;
}

static inline void shutdown(void)
{
    *REG32(SYSREG_CONTROL) = 0xc0800000;
    for (;;);
}

