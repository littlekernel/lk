#pragma once

#include <platform/bcm28xx.h>

#define SH_BASE (BCM_PERIPH_BASE_PHYS + 0x202000)

#define SH_CMD    (SH_BASE + 0x00)
#define SH_CMD_NEW_FLAG_SET       0x8000
#define SH_CMD_FAIL_FLAG_SET      0x4000
#define SH_CMD_BUSY_CMD_SET       0x0800
#define SH_CMD_NO_RESPONSE_SET    0x0400
#define SH_CMD_LONG_RESPONSE_SET  0x0200
#define SH_CMD_READ_CMD_SET       0x0040
#define SH_CMD_COMMAND_SET        0x003f
#define SH_ARG    (SH_BASE + 0x04)
#define SH_TOUT   (SH_BASE + 0x08)
#define SH_CDIV   (SH_BASE + 0x0c)
#define SH_RSP0   (SH_BASE + 0x10)
#define SH_RSP1   (SH_BASE + 0x14)
#define SH_RSP2   (SH_BASE + 0x18)
#define SH_RSP3   (SH_BASE + 0x1c)
#define SH_HSTS   (SH_BASE + 0x20)
#define SH_HSTS_DATA_FLAG_SET     0x01
#define SH_VDD    (SH_BASE + 0x30)
#define SH_VDD_POWER_ON_SET       0x01
#define SH_EDM    (SH_BASE + 0x34)
#define SH_HCFG   (SH_BASE + 0x38)
#define SH_HCFG_SLOW_CARD_SET     0x8
#define SH_HCFG_WIDE_EXT_BUS_SET  0x4
#define SH_HCFG_WIDE_INT_BUS_SET  0x2
#define SH_HBCT   (SH_BASE + 0x3c)
#define SH_DATA   (SH_BASE + 0x40)
#define SH_HBLC   (SH_BASE + 0x50)
