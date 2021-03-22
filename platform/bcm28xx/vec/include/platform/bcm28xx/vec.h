#pragma once

#include <platform/bcm28xx.h>
#include <lk/bits.h>

#define VEC_BASE (BCM_PERIPH_BASE_VIRT + 0x806000)

#define VEC_WSE_RESET (VEC_BASE + 0x0c0)
#define VEC_WSE_CONTROL (VEC_BASE + 0x0c4)
#define VEC_CONFIG0     (VEC_BASE + 0x104)
#define VEC_CONFIG0_NTSC_STD              0
#define VEC_CONFIG0_PAL_BDGHI_STD         1
#define VEC_CONFIG0_PDEN          BV(6)
#define VEC_SCHPH       (VEC_BASE + 0x108)
#define VEC_SOFT_RESET (VEC_BASE + 0x10c)
#define VEC_CLMP0_START (VEC_BASE + 0x144)
#define VEC_CLMP0_END  (VEC_BASE + 0x148)
#define VEC_FREQ3_2     (VEC_BASE + 0x180)
#define VEC_FREQ1_0     (VEC_BASE + 0x184)
#define VEC_CONFIG1     (VEC_BASE + 0x188)
#define VEC_CONFIG1_CUSTOM_FREQ           BV(0)
#define VEC_CONFIG1_C_CVBS_CVBS           (7 << 10)
#define VEC_CONFIG2    (VEC_BASE + 0x18c)
#define VEC_CONFIG2_UV_DIG_DIS            BV(6)
#define VEC_CONFIG2_RGB_DIG_DIS           BV(5)
#define VEC_CONFIG3    (VEC_BASE + 0x1a0)
#define VEC_CONFIG3_HORIZ_LEN_STD (0 << 0)
#define VEC_MASK0       (VEC_BASE + 0x204)
#define VEC_CFG          (VEC_BASE + 0x208)
#define VEC_CFG_VEC_EN                    BV(3)
#define VEC_DAC_CONFIG (VEC_BASE + 0x210)
#define VEC_DAC_CONFIG_LDO_BIAS_CTRL(x)   ((x) << 24)
#define VEC_DAC_CONFIG_DRIVER_CTRL(x)     ((x) << 16)
#define VEC_DAC_CONFIG_DAC_CTRL(x)        (x)
#define VEC_DAC_MISC    (VEC_BASE + 0x214)
#define VEC_DAC_MISC_DAC_RST_N            BV(0)
#define VEC_DAC_MISC_VID_ACT              BV(8)
