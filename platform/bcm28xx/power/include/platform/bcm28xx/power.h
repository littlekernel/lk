#pragma once

#include <stdint.h>
#include <platform/bcm28xx.h>

#define PM_BASE (BCM_PERIPH_BASE_VIRT + 0x100000)
#define PM_PASSWORD 0x5a000000

#define PM_AUDIO                (PM_BASE + 0x004)

#define PM_RSTC                 (PM_BASE + 0x1c)
#define PM_RSTC_WRCFG_CLR       0xffffffcf // mask to keep everything but the watchdog config
// https://github.com/raspberrypi/linux/issues/932
// docs for RSTS
// definitions from broadcom/bcm2708_chip/cpr_powman.h
// comments from github issue
#define PM_RSTS                 (PM_BASE + 0x20)
#define PM_RSTS_HADPOR_SET      0x00001000        // bit 12 had power-on reset
#define PM_RSTS_HADSRH_SET      0x00000400        // bit 10 had software hard reset
#define PM_RSTS_HADSRF_SET      0x00000200        // bit  9 had software full reset
#define PM_RSTS_HADSRQ_SET      0x00000100        // bit  8 had software quick reset
#define PM_RSTS_HADWRH_SET      0x00000040        // bit  6 had watchdog hard reset
#define PM_RSTS_HADWRF_SET      0x00000020        // bit  5 had watchdog full reset
#define PM_RSTS_HADWRQ_SET      0x00000010        // bit  4 had watchdog quick reset
#define PM_RSTS_HADDRH_SET      0x00000004        // bit  2 had debugger hard reset
#define PM_RSTS_HADDRF_SET      0x00000002        // bit  1 had debugger full reset
#define PM_RSTS_HADDRQ_SET      0x00000001        // bit  0 had debugger quick reset
#define PM_WDOG                 (PM_BASE + 0x24)
#define PM_WDOG_MASK            0x00000fff
#define PM_USB                  (PM_BASE + 0x5c)
#define PM_SMPS                 (PM_BASE + 0x6c)
#define PM_SPAREW               (PM_BASE + 0x74)
#define PM_IMAGE                (PM_BASE + 0x108)
#define PM_GRAFX                (PM_BASE + 0x10c)
#define PM_PROC                 (PM_BASE + 0x110)

#define PM_ENABLE   BV(12)
#define PM_V3DRSTN  BV(6)
#define PM_ISFUNC   BV(5)
#define PM_MRDONE   BV(4)
#define PM_MEMREP   BV(3)
#define PM_ISPOW    BV(2)
#define PM_POWOK    BV(1)
#define PM_POWUP    BV(0)

void power_up_image(void);
void power_up_usb(void);
void power_domain_on(volatile uint32_t *reg, uint32_t rstn);
void power_arm_start(void);
